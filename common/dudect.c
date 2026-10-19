#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dudect.h"
#include "xoshiro.h"
#include "variants.h"
#include "platform.h"

/* statistics must not use fused multiply-add, so that the host recompute
 * (analysis/recompute.py) gives bit-identical t-values; builds also pass
 * -ffp-contract=off for GCC, which ignores this pragma */
#if defined(__clang__)
#pragma STDC FP_CONTRACT OFF
#endif

#ifndef DUDECT_BATCH
#define DUDECT_BATCH 256
#endif
#ifndef DUDECT_CAL_MAX
#define DUDECT_CAL_MAX 10000
#endif
#ifndef COST_RUNS
#define COST_RUNS 9
#endif
#ifndef COST_BLOCKS
#define COST_BLOCKS 64
#endif
#ifndef BUILD_INFO
#define BUILD_INFO "unknown"
#endif

#define BARRIER() __asm__ volatile("" ::: "memory")

/* ------------------------------------------------------------ statistics */

static void welch_push(welch_t *w, double x, int c){
    w->n[c] += 1.0;
    double d = x - w->mean[c];
    w->mean[c] += d / w->n[c];
    w->m2[c] += d * (x - w->mean[c]);
}

double dudect_welch(const welch_t *w){
    if (w->n[0] < 2 || w->n[1] < 2) return 0.0;
    double v0 = w->m2[0] / (w->n[0] - 1), v1 = w->m2[1] / (w->n[1] - 1);
    double den = sqrt(v0 / w->n[0] + v1 / w->n[1]);
    if (den == 0.0) return 0.0;
    return (w->mean[0] - w->mean[1]) / den;
}

static int cmp_u32(const void *a, const void *b){
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

void dudect_init(dudect_state_t *s, uint32_t *cal, uint32_t ncal){
    memset(s, 0, sizeof *s);
    qsort(cal, ncal, sizeof cal[0], cmp_u32);
    double frac = 1.0;
    for (int k = 0; k < DUDECT_NCROP; k++) {
        frac *= 0.5;                               /* p_k = 1 - 0.5^(k+1) */
        uint32_t idx = (uint32_t)((1.0 - frac) * (double)ncal);
        if (idx >= ncal) idx = ncal - 1;
        s->thr[k] = cal[idx];
    }
    s->xmin[0] = s->xmin[1] = 0xFFFFFFFFu;
}

void dudect_push(dudect_state_t *s, uint32_t x, int c){
    double xd = (double)x;
    if (x < s->xmin[c]) s->xmin[c] = x;
    if (x > s->xmax[c]) s->xmax[c] = x;
    welch_push(&s->t[0], xd, c);
    for (int k = 0; k < DUDECT_NCROP; k++)
        if (x < s->thr[k]) welch_push(&s->t[1 + k], xd, c);
    welch_t *w0 = &s->t[0];
    if (w0->n[0] + w0->n[1] > DUDECT_ENOUGH) {
        double y = xd - w0->mean[c];
        welch_push(&s->t[DUDECT_NCROP + 1], y * y, c);
    }
}

double dudect_max_t(const dudect_state_t *s, int *which){
    double best = 0.0; *which = -1;
    for (int i = 0; i < DUDECT_NTESTS; i++) {
        const welch_t *w = &s->t[i];
        if (w->n[0] + w->n[1] <= DUDECT_ENOUGH) continue;
        double t = fabs(dudect_welch(w));
        if (*which < 0 || t > best) { best = t; *which = i; }
    }
    return best;
}

/* -------------------------------------------------------- number output */

/* fixed-point text, so no float printf support is needed on the MCU
 * (and no %llu, which newlib-nano lacks) */
static char *fmt_d(char *buf, double v, int dec){
    if (v != v) { strcpy(buf, "nan"); return buf; }
    int neg = v < 0; if (neg) v = -v;
    unsigned long sc = 1; for (int i = 0; i < dec; i++) sc *= 10;
    if (v >= 4.0e9) { strcpy(buf, neg ? "-big" : "big"); return buf; }
    unsigned long ip = (unsigned long)v;
    unsigned long fp = (unsigned long)((v - (double)ip) * (double)sc + 0.5);
    if (fp >= sc) { ip++; fp -= sc; }
    if (dec > 0) sprintf(buf, "%s%lu.%0*lu", neg ? "-" : "", ip, dec, fp);
    else sprintf(buf, "%s%lu", neg ? "-" : "", ip);
    return buf;
}
static char *fmt_hex_d(char *buf, double v){
    uint64_t u; memcpy(&u, &v, 8);
    sprintf(buf, "%08lx%08lx", (unsigned long)(u >> 32), (unsigned long)(u & 0xffffffffUL));
    return buf;
}

/* --------------------------------------------------------------- driver */

static const uint16_t FIXED_KEY[8] = {          /* TV3 key 000102..0E0F */
    0x0E0F, 0x0C0D, 0x0A0B, 0x0809, 0x0607, 0x0405, 0x0203, 0x0001 };
static const uint64_t FIXED_PT = 0x0123456789ABCDEFULL;   /* TV3 plaintext */

static uint8_t  b_cls[DUDECT_BATCH];
static uint64_t b_pt[DUDECT_BATCH];
static uint16_t b_key[DUDECT_BATCH][8];
static uint32_t b_d[DUDECT_BATCH];
static uint32_t cal_buf[DUDECT_CAL_MAX];
static dudect_state_t st;
static volatile uint64_t sink;

static void prepare(xoshiro128_t *r, int test, uint32_t n){
    for (uint32_t i = 0; i < n; i++) {
        int c = (int)(xoshiro128_next(r) & 1);
        b_cls[i] = (uint8_t)c;
        if (test == TEST_A) {
            b_pt[i] = c ? xoshiro128_next64(r) : FIXED_PT;
        } else {
            b_pt[i] = FIXED_PT;
            for (int j = 0; j < 8; j++)
                b_key[i][j] = c ? (uint16_t)xoshiro128_next(r) : FIXED_KEY[j];
        }
    }
}

static void measure(const dipper_variant_t *V, int test, const uint64_t *rk_fixed, uint32_t n){
    uint64_t acc = 0;
    uint64_t rk[DIPPER_ROUNDS];
    for (uint32_t i = 0; i < n; i++) {
        uint64_t p = b_pt[i];
        plat_irq_off();
        BARRIER();
        plat_t t0 = plat_now();
        BARRIER();
        if (test == TEST_A) {
            acc ^= V->enc(p, rk_fixed);
        } else {
            V->expand128(b_key[i], rk);
            acc ^= V->enc(p, rk);
        }
        BARRIER();
        plat_t t1 = plat_now();
        BARRIER();
        plat_irq_on();
        b_d[i] = plat_delta(t0, t1);
    }
    sink ^= acc;
}

static int is_pow2(uint32_t x){ return x && !(x & (x - 1)); }

static void print_status(const char *tag, const char *vn, int test, uint32_t total){
    char line[200], a[32], b[32], h[24];
    int which;
    double m = dudect_max_t(&st, &which);
    double t0 = dudect_welch(&st.t[0]);
    snprintf(line, sizeof line, "%s v=%s t=%c N=%lu maxt=%s at=%d t0=%s hex=%s",
             tag, vn, test == TEST_A ? 'A' : 'B', (unsigned long)total,
             fmt_d(a, m, 4), which, fmt_d(b, t0, 4), fmt_hex_d(h, m));
    plat_print(line);
}

static void run_one(const dudect_cfg_t *cfg, int vi, int test){
    const dipper_variant_t *V = &dipper_variants[vi];
    char line[200];
    xoshiro128_t rng;
    xoshiro128_seed(&rng, cfg->seed ^ (uint32_t)(vi * 0x9E3779B9u) ^ (uint32_t)(test * 0x85EBCA6Bu));
    uint64_t rk_fixed[DIPPER_ROUNDS];
    V->expand128(FIXED_KEY, rk_fixed);
    uint32_t ncal = cfg->ncal > DUDECT_CAL_MAX ? DUDECT_CAL_MAX : cfg->ncal;

    snprintf(line, sizeof line, "#run v=%s t=%c seed=%lu n=%lu ncal=%lu nraw=%lu batch=%d",
             V->name, test == TEST_A ? 'A' : 'B', (unsigned long)cfg->seed,
             (unsigned long)cfg->n, (unsigned long)ncal, (unsigned long)cfg->nraw, DUDECT_BATCH);
    plat_print(line);

    /* warm-up batch (first call builds the fast tables), discarded */
    prepare(&rng, test, DUDECT_BATCH);
    measure(V, test, rk_fixed, DUDECT_BATCH);

    /* calibration: thresholds for cropping, then discarded */
    for (uint32_t done = 0; done < ncal; ) {
        uint32_t m = ncal - done < DUDECT_BATCH ? ncal - done : DUDECT_BATCH;
        prepare(&rng, test, m);
        measure(V, test, rk_fixed, m);
        memcpy(&cal_buf[done], b_d, m * sizeof b_d[0]);
        done += m;
    }
    dudect_init(&st, cal_buf, ncal);
    {
        int pos = snprintf(line, sizeof line, "#thr");
        for (int k = 0; k < DUDECT_NCROP; k++)
            pos += snprintf(line + pos, sizeof line - pos, " %lu", (unsigned long)st.thr[k]);
        plat_print(line);
        snprintf(line, sizeof line, "#cal min=%lu med=%lu max=%lu",
                 (unsigned long)cal_buf[0], (unsigned long)cal_buf[ncal / 2],
                 (unsigned long)cal_buf[ncal - 1]);
        plat_print(line);
    }

    uint32_t total = 0;
    while (total < cfg->n) {
        uint32_t m = cfg->n - total < DUDECT_BATCH ? cfg->n - total : DUDECT_BATCH;
        prepare(&rng, test, m);
        measure(V, test, rk_fixed, m);
        for (uint32_t i = 0; i < m; i++) {
            dudect_push(&st, b_d[i], b_cls[i]);
            total++;
            if (total <= cfg->nraw) {
                /* raw dump: one 32-bit word per sample, class in bit 31 */
                static char raw[8 * 9 + 4];
                static int rn = 0;
                uint32_t d = b_d[i] > 0x7FFFFFFFu ? 0x7FFFFFFFu : b_d[i];
                if (rn == 0) { strcpy(raw, "#d"); }
                sprintf(raw + 2 + 9 * rn, " %08lx", (unsigned long)(d | ((uint32_t)b_cls[i] << 31)));
                rn++;
                if (rn == 8 || total == cfg->nraw || total == cfg->n) { plat_print(raw); rn = 0; }
            }
            if ((is_pow2(total) && total >= 1024) || total == cfg->nraw)
                print_status("#s", V->name, test, total);
        }
    }
    print_status("#res", V->name, test, total);
    {
        char a[32];
        int pos = snprintf(line, sizeof line, "#tl");
        for (int i = 0; i < DUDECT_NTESTS; i++)
            pos += snprintf(line + pos, sizeof line - pos, " %s/%lu", fmt_d(a, dudect_welch(&st.t[i]), 3),
                            (unsigned long)(st.t[i].n[0] + st.t[i].n[1]));
        plat_print(line);
        snprintf(line, sizeof line, "#rng v=%s t=%c min0=%lu max0=%lu min1=%lu max1=%lu n0=%lu n1=%lu",
                 V->name, test == TEST_A ? 'A' : 'B',
                 (unsigned long)st.xmin[0], (unsigned long)st.xmax[0],
                 (unsigned long)st.xmin[1], (unsigned long)st.xmax[1],
                 (unsigned long)st.t[0].n[0], (unsigned long)st.t[0].n[1]);
        plat_print(line);
    }
}

static int cmp_u32b(const void *a, const void *b){ return cmp_u32(a, b); }

static void cost(int vi){
    const dipper_variant_t *V = &dipper_variants[vi];
    uint64_t rk[DIPPER_ROUNDS];
    uint32_t enc[COST_RUNS], ks[COST_RUNS];
    char line[160];
    V->expand128(FIXED_KEY, rk);
    for (int r = -1; r < COST_RUNS; r++) {          /* r = -1: warm-up */
        uint64_t s = FIXED_PT ^ (uint64_t)(r + 1);
        plat_irq_off();
        plat_t t0 = plat_now();
        for (int i = 0; i < COST_BLOCKS; i++) s = V->enc(s, rk);
        plat_t t1 = plat_now();
        plat_irq_on();
        sink ^= s;
        uint16_t k[8];
        memcpy(k, FIXED_KEY, sizeof k);
        plat_irq_off();
        plat_t t2 = plat_now();
        for (int i = 0; i < COST_BLOCKS; i++) { V->expand128(k, rk); k[0] ^= (uint16_t)rk[27]; }
        plat_t t3 = plat_now();
        plat_irq_on();
        if (r >= 0) { enc[r] = plat_delta(t0, t1); ks[r] = plat_delta(t2, t3); }
    }
    qsort(enc, COST_RUNS, sizeof enc[0], cmp_u32b);
    qsort(ks, COST_RUNS, sizeof ks[0], cmp_u32b);
    snprintf(line, sizeof line, "#cost v=%s blocks=%d runs=%d enc_med=%lu enc_min=%lu ks_med=%lu ks_min=%lu",
             V->name, COST_BLOCKS, COST_RUNS,
             (unsigned long)enc[COST_RUNS / 2], (unsigned long)enc[0],
             (unsigned long)ks[COST_RUNS / 2], (unsigned long)ks[0]);
    plat_print(line);
}

int dudect_run_all(const dudect_cfg_t *cfg){
    char line[200];
    plat_print("#BEGIN");
    snprintf(line, sizeof line, "#meta platform=%s timer=%s build=%s", PLAT_NAME, PLAT_TIMER, BUILD_INFO);
    plat_print(line);
    int fail = dipper_selftest(1000, plat_print);
    if (fail) { plat_print("#END"); return 1; }
    for (int vi = 0; vi < dipper_n_variants; vi++)
        if (cfg->variants & (1u << vi)) cost(vi);
    for (int test = TEST_A; test <= TEST_B; test++)
        for (int vi = 0; vi < dipper_n_variants; vi++)
            if ((cfg->variants & (1u << vi)) && (cfg->tests & (1u << test)))
                run_one(cfg, vi, test);
    plat_print("#END");
    return 0;
}
