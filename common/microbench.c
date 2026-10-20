/* Lookup-latency microbenchmark (reviewer request, nRF52840 mechanism).
 *
 * For a 4 KB table in flash (const) and an identical copy in RAM:
 *  - pair test: after a reset sequence, time two byte loads R[a] then R[b]
 *    for a, b in 0..31 (two 16-byte lines); min and max over 8 repetitions;
 *  - stride test: time R[0] then R[d] for d = 1, 2, 4, ..., 2048.
 * The reset sequence reads 16 bytes spaced 64 B apart in the upper half of the
 * table, to replace the content of small read buffers and caches (<= 1 KB).
 * Output lines: #mbp region=<flash|ram> a=<a> min=<32 values> max=<32 values>
 *               #mbs region=<..> d=<d> min=<v> max=<v>
 * Interrupts are off around every timed pair; timer overhead is included and
 * is the same for all cells. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"

#define MB_SIZE 4096
#define MB_REPS 8

static const uint8_t MB_FLASH[MB_SIZE] __attribute__((aligned(64))) = {
#define B4(i) (uint8_t)((i)*37+11), (uint8_t)((i)*37+48), (uint8_t)((i)*37+85), (uint8_t)((i)*37+122)
#define B16(i) B4(4*(i)), B4(4*(i)+1), B4(4*(i)+2), B4(4*(i)+3)
#define B64(i) B16(4*(i)), B16(4*(i)+1), B16(4*(i)+2), B16(4*(i)+3)
#define B256(i) B64(4*(i)), B64(4*(i)+1), B64(4*(i)+2), B64(4*(i)+3)
    B256(0), B256(1), B256(2), B256(3), B256(4), B256(5), B256(6), B256(7),
    B256(8), B256(9), B256(10), B256(11), B256(12), B256(13), B256(14), B256(15)
};
static uint8_t MB_RAM[MB_SIZE] __attribute__((aligned(64)));
static volatile uint32_t mb_sink;

static inline void mb_reset(const volatile uint8_t *R){
    uint32_t acc = 0;
    for (int i = 0; i < 16; i++) acc += R[2048 + 64 * i];
    mb_sink += acc;
}

static uint32_t mb_pair(const volatile uint8_t *R, int a, int b){
    mb_reset(R);
    plat_irq_off();
    __asm__ volatile("" ::: "memory");
    plat_t t0 = plat_now();
    uint32_t x = R[a];
    uint32_t y = R[b];
    plat_t t1 = plat_now();
    __asm__ volatile("" ::: "memory");
    plat_irq_on();
    mb_sink += x + y;
    return plat_delta(t0, t1);
}

static void mb_region(const char *name, const volatile uint8_t *R){
    char line[400];
    for (int a = 0; a < 32; a++) {
        uint32_t mn[32], mx[32];
        for (int b = 0; b < 32; b++) { mn[b] = 0xFFFFFFFFu; mx[b] = 0; }
        for (int r = 0; r < MB_REPS; r++)
            for (int b = 0; b < 32; b++) {
                uint32_t d = mb_pair(R, a, b);
                if (d < mn[b]) mn[b] = d;
                if (d > mx[b]) mx[b] = d;
            }
        int pos = snprintf(line, sizeof line, "#mbp region=%s a=%d min=", name, a);
        for (int b = 0; b < 32; b++) pos += snprintf(line + pos, sizeof line - pos, "%s%lu", b ? "," : "", (unsigned long)mn[b]);
        pos += snprintf(line + pos, sizeof line - pos, " max=");
        for (int b = 0; b < 32; b++) pos += snprintf(line + pos, sizeof line - pos, "%s%lu", b ? "," : "", (unsigned long)mx[b]);
        plat_print(line);
    }
    for (int d = 1; d <= 2048; d *= 2) {
        uint32_t mn = 0xFFFFFFFFu, mx = 0;
        for (int r = 0; r < MB_REPS; r++) {
            uint32_t v = mb_pair(R, 0, d);
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        snprintf(line, sizeof line, "#mbs region=%s d=%d min=%lu max=%lu", name, d, (unsigned long)mn, (unsigned long)mx);
        plat_print(line);
    }
}

void microbench_run(void){
    char line[160];
    memcpy(MB_RAM, MB_FLASH, MB_SIZE);
    snprintf(line, sizeof line, "#mb flash_base=%p ram_base=%p reps=%d", (const void *)MB_FLASH, (void *)MB_RAM, MB_REPS);
    plat_print(line);
    mb_region("flash", MB_FLASH);
    mb_region("ram", MB_RAM);
}
