/* xoshiro128++ (Blackman, Vigna; public domain reference at prng.di.unimi.it),
 * seeded with splitmix32. Used only outside the timed region. */
#ifndef XOSHIRO_H
#define XOSHIRO_H
#include <stdint.h>
typedef struct { uint32_t s[4]; } xoshiro128_t;

static inline uint32_t xo_rotl(uint32_t x, int k){ return (x << k) | (x >> (32 - k)); }

static inline uint32_t xoshiro128_next(xoshiro128_t *r){
    uint32_t *s = r->s;
    uint32_t res = xo_rotl(s[0] + s[3], 7) + s[0];
    uint32_t t = s[1] << 9;
    s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3];
    s[2] ^= t; s[3] = xo_rotl(s[3], 11);
    return res;
}
static inline uint64_t xoshiro128_next64(xoshiro128_t *r){
    uint64_t hi = xoshiro128_next(r);
    return (hi << 32) | xoshiro128_next(r);
}
static inline void xoshiro128_seed(xoshiro128_t *r, uint32_t seed){
    for (int i = 0; i < 4; i++) {           /* splitmix32 */
        uint32_t z = (seed += 0x9E3779B9u);
        z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
        z = (z ^ (z >> 13)) * 0xC2B2AE35u;
        r->s[i] = z ^ (z >> 16);
    }
}
#endif
