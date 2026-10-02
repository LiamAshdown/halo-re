// tea_decrypt_block  (Ghidra: FUN_006182b0; game library code)
// address 0x6182b0, size 152 bytes
// name confidence: 0.85  rewrite confidence: 0.9
// evidence: objdump 0x6182b0..0x618347 is TEA decryption of one 8-byte block: v0/v1 from the block, k0..k3 from the
//   key, sum starting at 0xc6ef3720 and advanced by 0x61c88647 (minus the 0x9e3779b9 delta) for 32 rounds,
//   v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3), then v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1).
//   Used by tea_decrypt_buffer 0x618350 on the shader resource files (rasterizer_resource_file_verify_signature).
//   First-boot track: the standalone exe reached it while loading the effect files.
// blam-cc: stack -> block, key (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void tea_decrypt_block(uint32_t *block, const uint32_t *key)
{
    uint32_t v0 = block[0];
    uint32_t v1 = block[1];
    uint32_t sum = 0xc6ef3720;
    int32_t round;

    for (round = 0; round < 32; round++) {
        v1 -= ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
        v0 -= ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        sum += 0x61c88647;
    }
    block[0] = v0;
    block[1] = v1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
