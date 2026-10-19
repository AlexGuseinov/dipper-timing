/* =====================================================================
 * POSITIVE CONTROL -- DELIBERATELY LEAKY.  Not a real implementation.
 * =====================================================================
 * Variant "leaky": the small profile plus one branch on a secret bit
 * (bit 0 of plaintext XOR first round key) that adds a short dummy loop.
 * Its only purpose is to show that the test detects a known timing leak on
 * every platform. It gives the same ciphertexts as the small profile. */
#include <stdint.h>
#define DIPPER_ROUNDS 28
uint64_t dipper_encrypt_block(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);

#ifndef LEAKY_EXTRA_ITERS
#define LEAKY_EXTRA_ITERS 32
#endif

uint64_t dipper_encrypt_block_leaky(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]){
    if ((p ^ rk[0]) & 1) {                    /* secret-dependent branch */
        volatile uint32_t dummy = 0;
        for (int i = 0; i < LEAKY_EXTRA_ITERS; i++) dummy += (uint32_t)i;
    }
    return dipper_encrypt_block(p, rk);
}
