/* Controlled table-placement experiment (reviewer request).
 *
 * The same encryption and key-schedule functions read their lookup tables
 * through a pointer argument; the variants differ ONLY in the address passed:
 * a const copy (flash on MCUs) or a RAM copy with identical contents. The
 * functions are noinline, so RAM and flash variants run byte-identical code.
 *
 *   small_flash / small_ram : small profile (16-byte S-box) in rounds and key schedule
 *   fast_flash  / fast_ram  : fast profile (SB2 512 B, PT 2 KB) + key schedule S
 *   leaky1 / leaky32        : positive controls, a secret branch adding 1 or 32
 *                             dummy iterations (sensitivity check)
 *
 * Build options: -DPLACEMENT_RAMFUNC places these functions in RAM (.ramfunc),
 * to separate instruction fetches from data reads on flash. */
#include <stdint.h>
#include <string.h>
#include "fast_const_tables.h"
#define DIPPER_ROUNDS 28

#ifdef PLACEMENT_RAMFUNC
#define PFUNC __attribute__((noinline, section(".ramfunc")))
#else
#define PFUNC __attribute__((noinline))
#endif

/* flash (const) copies; 64-byte aligned so the address is known exactly */
static const uint8_t S_FLASH[16] __attribute__((aligned(64))) =
    {0x1,0xA,0x4,0xC,0x6,0xF,0x3,0x9,0x2,0xD,0xB,0x7,0x5,0x0,0x8,0xE};
/* RAM copies, filled from the const copies by placement_init() */
static uint8_t  S_RAM[16] __attribute__((aligned(64)));
static uint16_t SB2_RAM[256] __attribute__((aligned(64)));
static uint64_t PT_RAM[16][16] __attribute__((aligned(64)));
static uint8_t  P_PUB[64];                 /* public permutation indices (small) */

void placement_init(void){
    memcpy(S_RAM, S_FLASH, sizeof S_RAM);
    memcpy(SB2_RAM, SB2C, sizeof SB2_RAM);
    memcpy(PT_RAM, PTC, sizeof PT_RAM);
    for (int i = 0; i < 64; i++)
        P_PUB[i] = (uint8_t)(4*(i/16) + 16*((3*((i%16)/4) + (i%4)) % 4) + (i%4));
}

static inline uint16_t rotl16(uint16_t x,int r){ return (uint16_t)((x<<r)|(x>>(16-r))); }
static inline uint16_t rotr16(uint16_t x,int r){ return (uint16_t)((x>>r)|(x<<(16-r))); }
static inline uint8_t lfsr_step(uint8_t l){
    uint8_t nb = (uint8_t)(((l>>4)^(l>>1))&1);
    return (uint8_t)(((l<<1)|nb)&0x1F);
}

/* 128-bit key schedule of dipper.c, S-box read through the pointer S */
PFUNC void expand_128_tab(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS], const uint8_t *S){
    uint16_t K[8]; memcpy(K,k,sizeof(K));
    uint8_t lfsr=0x01;
    for(int i=0;i<DIPPER_ROUNDS;i++){
        rk[i]=((uint64_t)K[3]<<48)|((uint64_t)K[2]<<32)|((uint64_t)K[1]<<16)|K[0];
        uint16_t t1=rotr16(K[1],2), t0=rotr16(K[0],12);
        uint16_t n7=K[3],n6=K[2],n5=K[7],n4=K[6],n3=K[5],n2=K[4],n1=t1,n0=t0;
        K[7]=n7;K[6]=n6;K[5]=n5;K[4]=n4;K[3]=n3;K[2]=n2;K[1]=n1;K[0]=n0;
        K[7]=(uint16_t)((K[7]&0x0FFF)|((uint16_t)S[(K[7]>>12)&0xF]<<12));
        K[3]=(uint16_t)((K[3]&0x0FFF)|((uint16_t)S[(K[3]>>12)&0xF]<<12));
        K[1]=(uint16_t)((K[1]&0x0FFF)|((uint16_t)S[(K[1]>>12)&0xF]<<12));
        K[4]^=(uint16_t)(lfsr&0x1F);
        lfsr=lfsr_step(lfsr);
    }
}

/* small-profile encryption of dipper.c, S-box read through the pointer S */
PFUNC uint64_t enc_small_tab(uint64_t s, const uint64_t rk[DIPPER_ROUNDS], const uint8_t *S){
    for(int i=0;i<DIPPER_ROUNDS;i++){
        s ^= rk[i];
        uint64_t o=0;
        for(int j=0;j<16;j++) o |= (uint64_t)S[(s>>(4*j))&0xF]<<(4*j);
        s = o;
        uint16_t A=(uint16_t)(s>>48), B=(uint16_t)(s>>32), C=(uint16_t)(s>>16), D=(uint16_t)s;
        uint16_t A1=rotl16(A,1), B1=rotl16(B,4), C1=rotl16(C,7), D1=rotl16(D,11);
        uint16_t A2=(uint16_t)(A1+B1), C2=(uint16_t)(C1+D1);
        s = ((uint64_t)A2<<48)|((uint64_t)B1<<32)|((uint64_t)C2<<16)|D1;
        o=0;
        for(int b=0;b<64;b++) o |= ((s>>b)&1ULL)<<P_PUB[b];
        s = o;
    }
    return s;
}

/* fast-profile encryption of dipper_fast.c, tables read through pointers */
PFUNC uint64_t enc_fast_tab(uint64_t s, const uint64_t rk[DIPPER_ROUNDS],
                            const uint16_t *SB2, const uint64_t (*PT)[16]){
    for(int i=0;i<DIPPER_ROUNDS;i++){
        s^=rk[i];
        uint64_t o=0;
        for(int b=0;b<8;b++) o |= (uint64_t)SB2[(s>>(8*b))&0xFF]<<(8*b);
        s=o;
        uint16_t A=(uint16_t)(s>>48),B=(uint16_t)(s>>32),C=(uint16_t)(s>>16),D=(uint16_t)s;
        uint16_t A1=rotl16(A,1),B1=rotl16(B,4),C1=rotl16(C,7),D1=rotl16(D,11);
        uint16_t A2=(uint16_t)(A1+B1),C2=(uint16_t)(C1+D1);
        s=((uint64_t)A2<<48)|((uint64_t)B1<<32)|((uint64_t)C2<<16)|D1;
        o=0;
        for(int j=0;j<16;j++) o |= PT[j][(s>>(4*j))&0xF];
        s=o;
    }
    return s;
}

/* positive control with a configurable amount of extra work */
PFUNC uint64_t enc_leaky_n(uint64_t p, const uint64_t rk[DIPPER_ROUNDS], int iters){
    if ((p ^ rk[0]) & 1) {                    /* secret-dependent branch */
        volatile uint32_t dummy = 0;
        for (int i = 0; i < iters; i++) dummy += (uint32_t)i;
    }
    return enc_small_tab(p, rk, S_RAM);
}

/* wrappers with the common variant signature; only the address differs */
void     pl_ks_flash(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS]){ expand_128_tab(k, rk, S_FLASH); }
void     pl_ks_ram(const uint16_t k[8], uint64_t rk[DIPPER_ROUNDS])  { expand_128_tab(k, rk, S_RAM); }
uint64_t pl_small_flash(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]){ return enc_small_tab(p, rk, S_FLASH); }
uint64_t pl_small_ram(uint64_t p, const uint64_t rk[DIPPER_ROUNDS])  { return enc_small_tab(p, rk, S_RAM); }
uint64_t pl_fast_flash(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]) { return enc_fast_tab(p, rk, SB2C, PTC); }
uint64_t pl_fast_ram(uint64_t p, const uint64_t rk[DIPPER_ROUNDS])   { return enc_fast_tab(p, rk, SB2_RAM, PT_RAM); }
uint64_t pl_leaky1(uint64_t p, const uint64_t rk[DIPPER_ROUNDS])     { return enc_leaky_n(p, rk, 1); }
uint64_t pl_leaky32(uint64_t p, const uint64_t rk[DIPPER_ROUNDS])    { return enc_leaky_n(p, rk, 32); }

/* addresses for the log */
const void *pl_addr_s_flash(void){ return S_FLASH; }
const void *pl_addr_s_ram(void){ return S_RAM; }
const void *pl_addr_pt_flash(void){ return PTC; }
const void *pl_addr_pt_ram(void){ return PT_RAM; }
const void *pl_addr_code(void){ return (const void *)enc_small_tab; }
