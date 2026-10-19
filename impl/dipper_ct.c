/* Dipper, constant-time variant ("ct").
 * - S-box: GIFT S-box as a Boolean formula on the four bit planes of the packed
 *   64-bit state (3 AND, 1 OR, 6 XOR, 1 NOT), no table lookup.
 *   Formula from GIFT (Banik et al., CHES 2017) / aadomn/gift SBOX macro.
 * - Bit permutation: fixed masks and shifts (impl/ct_perm.h, generated).
 * - ARX layer: fixed rotations, 16-bit additions as plain additions.
 * - Key schedule: same S-box formula; the LFSR round constant is public.
 * No secret-dependent branches or memory addresses in the C source; the
 * compiled code is checked per ISA (see docs/ct_asm_review.md). */
#include <stdint.h>
#include <string.h>
#include "ct_perm.h"
#define DIPPER_ROUNDS 28
#define M1 0x1111111111111111ULL

static inline uint64_t ct_sub(uint64_t s){
    uint64_t s0 = s & M1, s1 = (s >> 1) & M1, s2 = (s >> 2) & M1, s3 = (s >> 3) & M1, t;
    s1 ^= s0 & s2;
    s0 ^= s1 & s3;
    s2 ^= s0 | s1;
    s3 ^= s2;
    s1 ^= s3;
    s3 ^= M1;          /* NOT on the plane */
    s2 ^= s0 & s1;
    t = s0; s0 = s3; s3 = t;
    return s0 | (s1 << 1) | (s2 << 2) | (s3 << 3);
}
/* S-box on one nibble held in bits 0..3 */
static inline uint16_t ct_sub_nib(uint16_t x){
    return (uint16_t)(ct_sub((uint64_t)(x & 0xF)) & 0xF);
}
static inline uint16_t rotl16(uint16_t x,int r){return (uint16_t)((x<<r)|(x>>(16-r)));}
static inline uint16_t rotr16(uint16_t x,int r){return (uint16_t)((x>>r)|(x<<(16-r)));}

static inline uint8_t lfsr_step(uint8_t l){
    uint8_t nb = (uint8_t)(((l>>4)^(l>>1))&1);
    return (uint8_t)(((l<<1)|nb)&0x1F);
}

void dipper_expand_96_ct(const uint16_t k[6], uint64_t rk[DIPPER_ROUNDS]){
    uint16_t K[6]; memcpy(K,k,sizeof(K));
    uint8_t lfsr=0x01;
    for(int i=0;i<DIPPER_ROUNDS;i++){
        rk[i]=((uint64_t)K[3]<<48)|((uint64_t)K[2]<<32)|((uint64_t)K[1]<<16)|K[0];
        uint16_t t1=rotr16(K[1],2), t0=rotr16(K[0],12);
        uint16_t n5=K[3],n4=K[2],n3=t1,n2=t0,n1=K[5],n0=K[4];
        K[5]=n5;K[4]=n4;K[3]=n3;K[2]=n2;K[1]=n1;K[0]=n0;
        K[5]=(uint16_t)((K[5]&0x0FFF)|(ct_sub_nib((uint16_t)(K[5]>>12))<<12));
        K[4]=(uint16_t)((K[4]&0xFF0F)|(ct_sub_nib((uint16_t)(K[4]>>4))<<4));
        K[2]=(uint16_t)((K[2]&0x0FFF)|(ct_sub_nib((uint16_t)(K[2]>>12))<<12));
        K[1]=(uint16_t)((K[1]&0xFF0F)|(ct_sub_nib((uint16_t)(K[1]>>4))<<4));
        K[3]^=(uint16_t)(lfsr&0x1F);
        lfsr=lfsr_step(lfsr);
    }
}
void dipper_expand_128_ct(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]){
    uint16_t K[8]; memcpy(K,k,sizeof(K));
    uint8_t lfsr=0x01;
    for(int i=0;i<DIPPER_ROUNDS;i++){
        rk[i]=((uint64_t)K[3]<<48)|((uint64_t)K[2]<<32)|((uint64_t)K[1]<<16)|K[0];
        uint16_t t1=rotr16(K[1],2), t0=rotr16(K[0],12);
        uint16_t n7=K[3],n6=K[2],n5=K[7],n4=K[6],n3=K[5],n2=K[4],n1=t1,n0=t0;
        K[7]=n7;K[6]=n6;K[5]=n5;K[4]=n4;K[3]=n3;K[2]=n2;K[1]=n1;K[0]=n0;
        K[7]=(uint16_t)((K[7]&0x0FFF)|(ct_sub_nib((uint16_t)(K[7]>>12))<<12));
        K[3]=(uint16_t)((K[3]&0x0FFF)|(ct_sub_nib((uint16_t)(K[3]>>12))<<12));
        K[1]=(uint16_t)((K[1]&0x0FFF)|(ct_sub_nib((uint16_t)(K[1]>>12))<<12));
        K[4]^=(uint16_t)(lfsr&0x1F);
        lfsr=lfsr_step(lfsr);
    }
}
uint64_t dipper_encrypt_block_ct(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]){
    uint64_t s=p;
    for(int i=0;i<DIPPER_ROUNDS;i++){
        s ^= rk[i];
        s  = ct_sub(s);
        uint16_t A=(uint16_t)(s>>48), B=(uint16_t)(s>>32), C=(uint16_t)(s>>16), D=(uint16_t)s;
        uint16_t A1=rotl16(A,1), B1=rotl16(B,4), C1=rotl16(C,7), D1=rotl16(D,11);
        uint16_t A2=(uint16_t)(A1+B1), C2=(uint16_t)(C1+D1);
        s = ((uint64_t)A2<<48)|((uint64_t)B1<<32)|((uint64_t)C2<<16)|D1;
        s = ct_perm(s);
    }
    return s;
}
/* used only by the test programs (S-box formula check) */
uint8_t dipper_ct_sbox_nibble(uint8_t x){ return (uint8_t)ct_sub_nib(x); }
