// md5_hex_digest  (Ghidra: FUN_0061a730; game library code)
// address 0x61a730, size 126 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: objdump 0x61a730..0x61a7ad: MD5Init inline (count zero, state 0x67452301 0xefcdab89 0x98badcfe
//   0x10325476), md5_update(data, length) 0x61a5a0, md5_final 0x61a660, then md5_digest_to_hex 0x619c90 into out.
//   rasterizer_resource_file_verify_signature 0x519970 compares the result with the 33 bytes stored at the end of
//   each shader resource file. The /GS cookie check is the compiler's.
// blam-cc: stack -> data, length, out (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_cseries.h"

// RSA reference MD5_CTX (layout from 0x61a5a0/0x61a660: state +0x00, bit count +0x10, buffer +0x18; 0x58 bytes, which
// md5_final zeroes as 0x16 dwords)
typedef struct md5_context {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
} md5_context;

extern void md5_update(md5_context *context, const uint8_t *input, uint32_t length); // 0x61a5a0
extern void md5_final(uint8_t *digest, md5_context *context); // 0x61a660


void md5_hex_digest(const uint8_t *data, int32_t length, char *out)
{
    md5_context context;
    uint8_t digest[16];

    context.count[0] = 0;
    context.count[1] = 0;
    context.state[0] = 0x67452301;
    context.state[1] = 0xefcdab89;
    context.state[2] = 0x98badcfe;
    context.state[3] = 0x10325476;
    md5_update(&context, data, (uint32_t)length);
    md5_final(digest, &context);
    md5_digest_to_hex(digest, out);
}
