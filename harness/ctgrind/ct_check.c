/* ctgrind-style check (A. Langley 2010): secret inputs are marked undefined
 * for Valgrind memcheck, which then reports every branch ("Conditional jump
 * or move depends on uninitialised value") and every memory address ("Use of
 * uninitialised value of size N") that depends on them.
 *   valgrind ./ct_check <variant> ks    secret: 128-bit master key, key schedule only
 *   valgrind ./ct_check <variant> enc   secret: plaintext and all 28 round keys
 * Variants: small fast fast_const ct leaky. Run by run_ctgrind.sh. */
#include <stdio.h>
#include <string.h>
#include <valgrind/memcheck.h>
#include "variants.h"

static volatile uint64_t sink;

int main(int argc, char **argv){
    if (argc != 3) { fprintf(stderr, "usage: %s <variant> ks|enc\n", argv[0]); return 2; }
    const dipper_variant_t *V = NULL;
    for (int i = 0; i < dipper_n_variants; i++)
        if (!strcmp(argv[1], dipper_variants[i].name)) V = &dipper_variants[i];
    if (!V) { fprintf(stderr, "unknown variant\n"); return 2; }

    uint16_t key[8] = {0x0E0F,0x0C0D,0x0A0B,0x0809,0x0607,0x0405,0x0203,0x0001};
    uint64_t rk[DIPPER_ROUNDS], pt = 0x0123456789ABCDEFULL;

    /* warm-up with public data: builds the run-time tables of "fast" */
    V->expand128(key, rk);
    sink = V->enc(pt, rk);

    if (!strcmp(argv[2], "ks")) {
        VALGRIND_MAKE_MEM_UNDEFINED(key, sizeof key);
        V->expand128(key, rk);
        VALGRIND_MAKE_MEM_DEFINED(rk, sizeof rk);    /* do not report the copy-out */
    } else {
        VALGRIND_MAKE_MEM_UNDEFINED(&pt, sizeof pt);
        VALGRIND_MAKE_MEM_UNDEFINED(rk, sizeof rk);
        uint64_t c = V->enc(pt, rk);
        VALGRIND_MAKE_MEM_DEFINED(&c, sizeof c);
        sink = c;
    }
    return 0;
}
