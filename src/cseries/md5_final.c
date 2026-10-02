// md5_final  (Ghidra: FUN_0061a660; game library code, the RSA reference MD5Final)
// address 0x61a660, size 192 bytes
// name confidence: 0.85  rewrite confidence: 0.9
// evidence: objdump 0x61a660..0x61a726: the bit count is encoded little-endian, the message padded with the PADDING
//   table at 0x00683948 (0x80 then zeros) to 56 mod 64 (56 - index, or 120 - index), the 8 count bytes appended, the
//   state written out little-endian as the digest and the whole context (0x16 dwords) zeroed. The /GS cookie check
//   is the compiler's.
// blam-cc: stack -> digest, context (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

// RSA reference MD5_CTX (layout from 0x61a5a0/0x61a660: state +0x00, bit count +0x10, buffer +0x18; 0x58 bytes, which
// md5_final zeroes as 0x16 dwords)
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef struct md5_context {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
} md5_context;

extern void md5_update(md5_context *context, const uint8_t *input, uint32_t length); // 0x61a5a0
extern const uint8_t md5_padding[64]; // 0x00683948

void md5_final(uint8_t *digest, md5_context *context)
{
    uint8_t bits[8];
    uint32_t index;
    int32_t i;

    for (i = 0; i < 8; i++) {
        bits[i] = (uint8_t)(context->count[i / 4] >> ((i % 4) * 8));
    }
    index = (context->count[0] >> 3) & 0x3f;
    md5_update(context, md5_padding, index < 56 ? 56 - index : 120 - index);
    md5_update(context, bits, 8);
    for (i = 0; i < 16; i++) {
        digest[i] = (uint8_t)(context->state[i / 4] >> ((i % 4) * 8));
    }
    for (i = 0; i < (int32_t)(sizeof(md5_context) / 4); i++) {
        ((uint32_t *)context)[i] = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
