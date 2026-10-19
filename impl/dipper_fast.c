/* Dipper - table-optimized C implementation ("fast" profile).
 * S-box via 8x256 byte LUT (combined two nibbles): 2KB (could be 16B nibble LUT for "small").
 * Bit permutation via 16 nibble tables: PT[nib_idx][nib_val] -> pre-scattered 64-bit word. 2KB.
 * Total lookup tables ~4KB ROM. Same external API as dipper.c (suffix _fast).
 */
#include <stdint.h>
#include <string.h>
#define DIPPER_ROUNDS 28
static const uint8_t S[16]={0x1,0xA,0x4,0xC,0x6,0xF,0x3,0x9,0x2,0xD,0xB,0x7,0x5,0x0,0x8,0xE};
static const uint8_t SI[16]={0xD,0x0,0x8,0x6,0x2,0xC,0x4,0xB,0xE,0x7,0x1,0xA,0x3,0x9,0xF,0x5};
static uint16_t SB2[256], SB2I[256];      /* two-nibble S-box */
static uint64_t PT[16][16], PTI[16][16];  /* nibble-scatter permutation tables */
static int ready=0;
static void init_fast(void){
    if(ready) return;
    for(int b=0;b<256;b++){
        SB2[b]  = (uint16_t)(S[b&0xF]  | (S[(b>>4)&0xF]<<4));
        SB2I[b] = (uint16_t)(SI[b&0xF] | (SI[(b>>4)&0xF]<<4));
    }
    for(int j=0;j<16;j++) for(int v=0;v<16;v++){
        uint64_t w=0, wi=0;
        for(int k=0;k<4;k++){
            if((v>>k)&1){
                int i=4*j+k;
                int p=4*(i/16)+16*((3*((i%16)/4)+(i%4))%4)+(i%4);
                w |= 1ULL<<p;
            }
        }
        PT[j][v]=w;
        /* inverse: bits of nibble j of the OUTPUT come from which input bits */
        for(int k=0;k<4;k++){
            if((v>>k)&1){
                int o=4*j+k;
                /* find i with P(i)=o */
                for(int i=0;i<64;i++){
                    int p=4*(i/16)+16*((3*((i%16)/4)+(i%4))%4)+(i%4);
                    if(p==o){ wi |= 1ULL<<i; break; }
                }
            }
        }
        PTI[j][v]=wi;
    }
    ready=1;
}
static inline uint16_t rotl16(uint16_t x,int r){r&=15;return (uint16_t)((x<<r)|(x>>(16-r)));}
static inline uint16_t rotr16(uint16_t x,int r){r&=15;return (uint16_t)((x>>r)|(x<<(16-r)));}
static inline uint64_t sub_fast(uint64_t s){
    uint64_t o=0;
    for(int b=0;b<8;b++) o |= (uint64_t)SB2[(s>>(8*b))&0xFF]<<(8*b);
    return o;
}
static inline uint64_t sub_fast_inv(uint64_t s){
    uint64_t o=0;
    for(int b=0;b<8;b++) o |= (uint64_t)SB2I[(s>>(8*b))&0xFF]<<(8*b);
    return o;
}
static inline uint64_t perm_fast(uint64_t s){
    uint64_t o=0;
    for(int j=0;j<16;j++) o |= PT[j][(s>>(4*j))&0xF];
    return o;
}
static inline uint64_t perm_fast_inv(uint64_t s){
    uint64_t o=0;
    for(int j=0;j<16;j++) o |= PTI[j][(s>>(4*j))&0xF];
    return o;
}
uint64_t dipper_encrypt_block_fast(uint64_t p, const uint64_t rk[DIPPER_ROUNDS]){
    init_fast();
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
uint64_t dipper_decrypt_block_fast(uint64_t c, const uint64_t rk[DIPPER_ROUNDS]){
    init_fast();
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
