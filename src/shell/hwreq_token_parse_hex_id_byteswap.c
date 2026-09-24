// hwreq_token_parse_hex_id_byteswap  (Ghidra: hwreq_token_parse_hex_id_byteswap, already named)
// address 0x578f80, size 31 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: swaps the two bytes of hwreq_token_parse_hex_id's 16-bit result.
// register convention: parser passed straight through to hwreq_token_parse_hex_id.
// FIXED (register inputs, objdump): EAX carries parser (read at 0x578f80, the `call 0x578ef0`
//   into hwreq_token_parse_hex_id, whose own convention is "parser in EAX"); `parser` was
//   already a C parameter here but had no machine-checked "blam-cc" line.
//   // blam-cc: EAX -> parser

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern uint32_t hwreq_token_parse_hex_id(hwreq_parser *parser); // 0x00578ef0

// Parses a 4-hex-digit token and returns it byte-swapped, used when assembling multi-byte
// fields (such as a driver GUID) from their textual hex representation.
// blam-cc: EAX -> parser
int32_t hwreq_token_parse_hex_id_byteswap(hwreq_parser *parser)
{
    uint32_t value = hwreq_token_parse_hex_id(parser);
    if (value == 0xffffffff) {
        return -1;
    }
    return (int32_t)((value & 0xff) * 0x100 + (value >> 8 & 0xff));
}

#if 0
Original Ghidra decompilation (0x578f80):


int hwreq_token_parse_hex_id_byteswap(void)

{
  uint uVar1;
  
  uVar1 = hwreq_token_parse_hex_id();
  if (uVar1 == 0xffffffff) {
    return -1;
  }
  return (uVar1 & 0xff) * 0x100 + (uVar1 >> 8 & 0xff);
}
#endif
