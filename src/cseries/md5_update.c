// md5_update  (Ghidra: FUN_0061a5a0; game library code, the RSA reference MD5Update)
// address 0x61a5a0, size 185 bytes
// name confidence: 0.85  rewrite confidence: 0.9
// evidence: objdump 0x61a5a0..0x61a658: index = (count[0] >> 3) & 0x3f; the bit count grows by length * 8 with the
//   carry and length >> 29 into count[1]; if length >= 64 - index the buffer is topped up and transformed, then every
//   further full 64 bytes of input is transformed in place; the rest is appended to the buffer.
// blam-cc: stack -> context, input, length (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// RSA reference MD5_CTX (layout from 0x61a5a0/0x61a660: state +0x00, bit count +0x10, buffer +0x18; 0x58 bytes, which
// md5_final zeroes as 0x16 dwords)
typedef struct md5_context {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
} md5_context;

extern void md5_transform(const uint8_t *block, md5_context *context); // 0x619cc0, ECX block

void md5_update(md5_context *context, const uint8_t *input, uint32_t length)
{
    uint32_t index = (context->count[0] >> 3) & 0x3f;
    uint32_t part_length = 64 - index;
    uint32_t i;
    uint32_t j;

    context->count[0] += length << 3;
    if (context->count[0] < (length << 3)) {
        context->count[1]++;
    }
    context->count[1] += length >> 29;

    if (length >= part_length) {
        for (j = 0; j < part_length; j++) {
            context->buffer[index + j] = input[j];
        }
        md5_transform(context->buffer, context);
        for (i = part_length; i + 63 < length; i += 64) {
            md5_transform(&input[i], context);
        }
        index = 0;
    } else {
        i = 0;
    }
    for (j = 0; j < length - i; j++) {
        context->buffer[index + j] = input[i + j];
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
