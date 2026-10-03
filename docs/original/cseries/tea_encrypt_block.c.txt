// tea_encrypt_block  (game library code; no C existed)
// address 0x6181c0, size 142 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6181c0..0x61824d: TEA encryption of one 8-byte block: 32 rounds, sum +=
//   0x9e3779b9; v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1); v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >>
//   5) + k3). The inverse of tea_decrypt_block 0x6182b0.
// blam-cc: cdecl

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void tea_encrypt_block(uint32_t *block, const uint32_t *key)
{
    uint32_t v0 = block[0];
    uint32_t v1 = block[1];
    uint32_t sum = 0;
    int32_t round;

    for (round = 32; round != 0; round--) {
        sum += 0x9e3779b9;
        v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
    }
    block[0] = v0;
    block[1] = v1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
