/* Dipper fast profile with the lookup tables as precomputed const arrays
 * (variant "fast_const"). The round function is copied from dipper_fast.c
 * unchanged; only the table names differ and init_fast() is gone.
 * Tables: impl/fast_const_tables.h (generated, byte-identical to init_fast()).
 * NOT constant-time: SB2C and PTC are indexed by secret data. */
#include <stdint.h>
#include "fast_const_tables.h"
#define DIPPER_ROUNDS 28
static inline uint16_t rotl16(uint16_t x,int r){r&=15;return (uint16_t)((x<<r)|(x>>(16-r)));}
static inline uint16_t rotr16(uint16_t x,int r){r&=15;return (uint16_t)((x>>r)|(x<<(16-r)));}
static inline uint64_t sub_fast(uint64_t s){
    uint64_t o=0;
    for(int b=0;b<8;b++) o |= (uint64_t)SB2C[(s>>(8*b))&0xFF]<<(8*b);
    return o;
}
static inline uint64_t sub_fast_inv(uint64_t s){
    uint64_t o=0;
    for(int b=0;b<8;b++) o |= (uint64_t)SB2IC[(s>>(8*b))&0xFF]<<(8*b);
    return o;
}
static inline uint64_t perm_fast(uint64_t s){
    uint64_t o=0;
    for(int j=0;j<16;j++) o |= PTC[j][(s>>(4*j))&0xF];
    return o;
}
static inline uint64_t perm_fast_inv(uint64_t s){
    uint64_t o=0;
    for(int j=0;j<16;j++) o |= PTIC[j][(s>>(4*j))&0xF];
    return o;
}
uint64_t dipper_encrypt_block_fastc(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]){
    uint64_t s=p;
    for(int i=0;i<DIPPER_ROUNDS;i++){
        s^=rk[i];
        s=sub_fast(s);
        uint16_t A=(uint16_t)(s>>48),B=(uint16_t)(s>>32),C=(uint16_t)(s>>16),D=(uint16_t)s;
        uint16_t A1=rotl16(A,1),B1=rotl16(B,4),C1=rotl16(C,7),D1=rotl16(D,11);
        uint16_t A2=(uint16_t)(A1+B1),C2=(uint16_t)(C1+D1);
        s=((uint64_t)A2<<48)|((uint64_t)B1<<32)|((uint64_t)C2<<16)|D1;
        s=perm_fast(s);
    }
    return s;
}
uint64_t dipper_decrypt_block_fastc(uint64_t c, const uint64_t rk[DIPPER_ROUNDS]){
    uint64_t s=c;
    for(int i=DIPPER_ROUNDS-1;i>=0;i--){
        s=perm_fast_inv(s);
        uint16_t A2=(uint16_t)(s>>48),B1=(uint16_t)(s>>32),C2=(uint16_t)(s>>16),D1=(uint16_t)s;
        uint16_t A1=(uint16_t)(A2-B1),C1=(uint16_t)(C2-D1);
        uint16_t A=rotr16(A1,1),B=rotr16(B1,4),C=rotr16(C1,7),D=rotr16(D1,11);
        s=((uint64_t)A<<48)|((uint64_t)B<<32)|((uint64_t)C<<16)|D;
        s=sub_fast_inv(s);
        s^=rk[i];
    }
    return s;
}
