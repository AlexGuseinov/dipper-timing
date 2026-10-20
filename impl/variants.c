#include <stdio.h>
#include <string.h>
#include "variants.h"

#ifndef VARIANT_SET_PLACEMENT
/* fast and fast_const have no key schedule of their own: they use the small
 * profile's dipper_expand_* (as in the upstream repository). */
const dipper_variant_t dipper_variants[] = {
    { "small",      dipper_expand_96,    dipper_expand_128,    dipper_encrypt_block       },
    { "fast",       dipper_expand_96,    dipper_expand_128,    dipper_encrypt_block_fast  },
    { "fast_const", dipper_expand_96,    dipper_expand_128,    dipper_encrypt_block_fastc },
    { "ct",         dipper_expand_96_ct, dipper_expand_128_ct, dipper_encrypt_block_ct    },
    { "leaky",      dipper_expand_96,    dipper_expand_128,    dipper_encrypt_block_leaky },
};
#else
/* controlled placement experiment (impl/dipper_placement.c): identical code,
 * tables passed as a flash or a RAM address. expand96 is only used by the
 * 96-bit test vectors and is not timed. */
void     pl_ks_flash(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]);
void     pl_ks_ram(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]);
uint64_t pl_small_flash(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t pl_small_ram(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t pl_fast_flash(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t pl_fast_ram(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t pl_leaky1(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t pl_leaky32(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
const dipper_variant_t dipper_variants[] = {
    { "small_flash", dipper_expand_96, pl_ks_flash, pl_small_flash },
    { "small_ram",   dipper_expand_96, pl_ks_ram,   pl_small_ram   },
    { "fast_flash",  dipper_expand_96, pl_ks_flash, pl_fast_flash  },
    { "fast_ram",    dipper_expand_96, pl_ks_ram,   pl_fast_ram    },
    { "leaky1",      dipper_expand_96, pl_ks_ram,   pl_leaky1      },
    { "leaky32",     dipper_expand_96, pl_ks_ram,   pl_leaky32     },
};
#endif
const int dipper_n_variants = (int)(sizeof(dipper_variants) / sizeof(dipper_variants[0]));

static void k96(uint32_t hi32, uint64_t lo64, uint16_t k[6]){
    k[5]=(uint16_t)(hi32>>16); k[4]=(uint16_t)hi32;
    k[3]=(uint16_t)(lo64>>48); k[2]=(uint16_t)(lo64>>32);
    k[1]=(uint16_t)(lo64>>16); k[0]=(uint16_t)lo64;
}
static void k128(uint64_t hi, uint64_t lo, uint16_t k[8]){
    k[7]=(uint16_t)(hi>>48);k[6]=(uint16_t)(hi>>32);k[5]=(uint16_t)(hi>>16);k[4]=(uint16_t)hi;
    k[3]=(uint16_t)(lo>>48);k[2]=(uint16_t)(lo>>32);k[1]=(uint16_t)(lo>>16);k[0]=(uint16_t)lo;
}
/* splitmix64, only for the self-test inputs */
static uint64_t sm64(uint64_t *x){
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

int dipper_selftest(long nrand, void (*print)(const char *line)){
    static const uint8_t S[16] = {0x1,0xA,0x4,0xC,0x6,0xF,0x3,0x9,0x2,0xD,0xB,0x7,0x5,0x0,0x8,0xE};
    char line[128];
    int fail = 0;
    uint64_t rk[DIPPER_ROUNDS], rk2[DIPPER_ROUNDS];
    uint16_t k6[6], k8[8];

    for (int x = 0; x < 16; x++) fail |= (dipper_ct_sbox_nibble((uint8_t)x) != S[x]);
    snprintf(line, sizeof line, "#selftest ct_sbox_formula %s", fail ? "FAIL" : "OK");
    print(line);

    for (int v = 0; v < dipper_n_variants; v++) {
        const dipper_variant_t *V = &dipper_variants[v];
        int f = 0;
        k96(0x00112233UL, 0x445566778899AABBULL, k6); V->expand96(k6, rk);
        f |= V->enc(0x0123456789ABCDEFULL, rk) != 0x5F189202D3152B8FULL;
        k96(0, 0, k6); V->expand96(k6, rk);
        f |= V->enc(0, rk) != 0xD873355EC63B1B4BULL;
        k128(0x0001020304050607ULL, 0x08090A0B0C0D0E0FULL, k8); V->expand128(k8, rk);
        f |= V->enc(0x0123456789ABCDEFULL, rk) != 0x4B46284387969060ULL;
        k128(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, k8); V->expand128(k8, rk);
        f |= V->enc(0xFFFFFFFFFFFFFFFFULL, rk) != 0x6ABB4518063706B0ULL;
        snprintf(line, sizeof line, "#selftest %s TV1-TV4 %s", V->name, f ? "FAIL" : "OK");
        print(line);
        fail |= f;
    }

    uint64_t st = 0xD1BBE7;
    long bad = 0;
    for (long n = 0; n < nrand; n++) {
        uint64_t hi = sm64(&st), lo = sm64(&st), p = sm64(&st);
        k128(hi, lo, k8);
        dipper_expand_128(k8, rk);
        uint64_t ref = dipper_encrypt_block(p, rk);
        bad += dipper_decrypt_block(ref, rk) != p;
        for (int v = 1; v < dipper_n_variants; v++) {
            const dipper_variant_t *V = &dipper_variants[v];
            V->expand128(k8, rk2);
            bad += memcmp(rk, rk2, sizeof rk) != 0;
            bad += V->enc(p, rk2) != ref;
        }
    }
    snprintf(line, sizeof line, "#selftest random_vs_small n=%ld mismatches=%ld %s",
             nrand, bad, bad ? "FAIL" : "OK");
    print(line);
    fail |= bad != 0;
    print(fail ? "#SELFTEST FAIL" : "#SELFTEST PASS");
    return fail;
}
