// tea_decrypt_buffer  (Ghidra: FUN_00618350; game library code)
// address 0x618350, size 87 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: objdump 0x618350..0x6183a6: nothing for fewer than 8 bytes; when the length is not a multiple of 8 (signed
//   remainder) the LAST 8 bytes (overlapping the final full block) are decrypted first; then every full 8-byte block
//   from the start, in order, through tea_decrypt_block 0x6182b0. rasterizer_resource_file_verify_signature 0x519970
//   calls it on each shader resource file before checking its MD5.
// blam-cc: stack -> length, data, key (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_cseries.h"


void tea_decrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    int32_t blocks;

    if (length < 8) {
        return;
    }
    if (length % 8 != 0) {
        tea_decrypt_block((uint32_t *)(data + length - 8), key);
    }
    for (blocks = length / 8; blocks > 0; blocks--) {
        tea_decrypt_block((uint32_t *)data, key);
        data += 8;
    }
}
