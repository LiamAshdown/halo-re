// tea_encrypt_buffer  (game library code; no C existed)
// address 0x618250, size 93 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618250..0x6182ac: nothing for fewer than 8 bytes; every full 8-byte block from
//   the start through tea_encrypt_block, then, when the length is not a multiple of 8 (signed remainder), the LAST 8
//   bytes again (overlapping the final full block). The inverse of tea_decrypt_buffer 0x618350.
// blam-cc: cdecl

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void tea_encrypt_block(uint32_t *block, const uint32_t *key); // 0x6181c0

void tea_encrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    int32_t blocks;
    uint8_t *block;

    if (length < 8) {
        return;
    }
    block = data;
    for (blocks = length / 8; blocks > 0; blocks--) {
        tea_encrypt_block((uint32_t *)block, key);
        block += 8;
    }
    if (length % 8 != 0) {
        tea_encrypt_block((uint32_t *)(data + length - 8), key);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
