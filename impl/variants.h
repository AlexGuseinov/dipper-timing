/* Uniform interface to the five Dipper-64/128 variants under test. */
#ifndef DIPPER_VARIANTS_H
#define DIPPER_VARIANTS_H
#include <stdint.h>
#define DIPPER_ROUNDS 28

typedef void     (*dipper_expand_fn)(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]);
typedef uint64_t (*dipper_enc_fn)(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
typedef void     (*dipper_expand96_fn)(const uint16_t k[6], uint64_t rk[DIPPER_ROUNDS]);

typedef struct {
    const char        *name;
    dipper_expand96_fn expand96;   /* only for the test vectors */
    dipper_expand_fn   expand128;
    dipper_enc_fn      enc;
} dipper_variant_t;

extern const dipper_variant_t dipper_variants[];
extern const int dipper_n_variants;

/* functions from the implementation files */
void dipper_expand_96(const uint16_t k[6], uint64_t rk[DIPPER_ROUNDS]);
void dipper_expand_128(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]);
uint64_t dipper_encrypt_block(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t dipper_decrypt_block(uint64_t c, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t dipper_encrypt_block_fast(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint64_t dipper_encrypt_block_fastc(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
void dipper_expand_96_ct(const uint16_t k[6], uint64_t rk[DIPPER_ROUNDS]);
void dipper_expand_128_ct(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]);
uint64_t dipper_encrypt_block_ct(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);
uint8_t dipper_ct_sbox_nibble(uint8_t x);
uint64_t dipper_encrypt_block_leaky(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]);

/* self-test: TV1-TV4 for every variant, ct S-box formula vs table,
 * nrand random (key, plaintext) pairs of every variant vs small.
 * Returns 0 on success; prints through the given line printer. */
int dipper_selftest(long nrand, void (*print)(const char *line));
#endif
