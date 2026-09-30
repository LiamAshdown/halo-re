// network_debug_fill_canary_buffer  (Ghidra: FUN_004e0790, unnamed)
// address 0x4e0790, size 117 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Writes a fixed, human-readable
// filler/canary byte pattern ('message in a bot...') into the 20-byte buffer pointed to by
// in_EAX, likely a debug placeholder value." The four dwords decode (little-endian) to the
// ASCII text "message in a bot".
// register convention: EAX = buffer (uint32_t[5]).
// blam-cc: EAX -> buffer

// VERIFIED against disassembly 0x4e0790..0x4e0805 (2026-09-30): four dwords = "message in a bot"; 5th dword never written
#include "tags.h"
#include "memory.h"

// Fills a 20-byte scratch buffer with the literal placeholder text "message in a bot".
void network_debug_fill_canary_buffer(uint32_t *buffer)
{
    buffer[0] = 0;
    buffer[0] = 0x7373656d;
    buffer[1] = 0x20656761;
    buffer[2] = 0x61206e69;
    buffer[3] = 0x746f6220;
}

#if 0
Original Ghidra decompilation (0x4e0790):

void FUN_004e0790(void)

{
  undefined4 *in_EAX;

  *in_EAX = 0;
  *in_EAX = 0x7373656d;
  in_EAX[1] = 0x20656761;
  in_EAX[2] = 0x61206e69;
  in_EAX[3] = 0x746f6220;
  return;
}
#endif
