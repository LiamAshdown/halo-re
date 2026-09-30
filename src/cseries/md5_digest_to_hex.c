// md5_digest_to_hex  (Ghidra: FUN_00619c90; game library code)
// address 0x619c90, size 48 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: objdump 0x619c90..0x619cbf: sprintf(out + 2 * i, "%02x" (0x0064e4dc), digest[i]) for the 16 bytes, so the
//   text is 32 lowercase hex digits and a terminator (the 33 bytes rasterizer_resource_file_verify_signature compares).
// blam-cc: stack -> digest, out (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_cseries.h"

extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT
extern const char md5_hex_byte_format[]; // 0x0064e4dc "%02x"

void md5_digest_to_hex(const uint8_t *digest, char *out)
{
    uint32_t i;

    for (i = 0; i < 0x10; i++) {
        sprintf(out, md5_hex_byte_format, digest[i]);
        out += 2;
    }
}
