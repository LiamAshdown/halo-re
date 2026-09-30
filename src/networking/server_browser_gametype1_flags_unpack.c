// server_browser_gametype1_flags_unpack  (Ghidra: FUN_00576890; named per this rewrite)
// address 0x576890, size 109 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary: "Decodes the compact type-1 game flags
// code produced by server_browser_gametype1_flags_pack back into booleans and a value pair."
// register convention: code in EAX (in_EAX), destination struct in ECX (in_ECX).
// blam-cc: EAX -> code, ECX -> out
// UNSURE: the destination's trailing byte pair (a small "min/max"-shaped value, always zero for
// code 0 and for the unmapped default case) does not simply mirror
// server_browser_gametype1_flags_pack's own int32 time_limit field -- it looks like a rendered
// value distinct from the packed representation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"


// blam-cc: EAX -> code, ECX -> out
// Decodes the compact type-1 code produced by server_browser_gametype1_flags_pack back into
// booleans and a low/high value pair.
void server_browser_gametype1_flags_unpack(uint32_t code, server_browser_gametype1_decoded *out)
{
    out->flags[0] = (uint8_t)(code >> 3) & 1;
    out->flags[1] = (uint8_t)(code >> 4) & 1;
    out->flags[2] = (uint8_t)(code >> 5) & 1;
    out->flags[3] = (uint8_t)(code >> 6) & 1;
    switch ((code >> 7) & 7) {
    case 1: out->low = 8; out->high = 7; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 2: out->low = 0x10; out->high = 0xe; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 3: out->low = 0x18; out->high = 0x15; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 4: out->low = 0x28; out->high = 0x23; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 5: out->low = 0x50; out->high = 0x46; out->unknown_06 = 0; out->unknown_07 = 0; break;
    default: out->low = 0; out->high = 0; out->unknown_06 = 0; out->unknown_07 = 0; break;
    }
}

#if 0
Original Ghidra decompilation (0x576890):

void FUN_00576890(void)

{
  uint in_EAX;
  byte *in_ECX;

  *in_ECX = (byte)(in_EAX >> 3) & 1;
  in_ECX[1] = (byte)(in_EAX >> 4) & 1;
  in_ECX[2] = (byte)(in_EAX >> 5) & 1;
  in_ECX[3] = (byte)(in_EAX >> 6) & 1;
  switch(in_EAX >> 7 & 7) {
  default:
    in_ECX[4] = 0;
    in_ECX[5] = 0;
    in_ECX[6] = 0;
    in_ECX[7] = 0;
    return;
  case 1:
    in_ECX[4] = 8;
    in_ECX[5] = 7;
    in_ECX[6] = 0;
    in_ECX[7] = 0;
    return;
  case 2:
    in_ECX[4] = 0x10;
    in_ECX[5] = 0xe;
    in_ECX[6] = 0;
    in_ECX[7] = 0;
    return;
  case 3:
    in_ECX[4] = 0x18;
    in_ECX[5] = 0x15;
    in_ECX[6] = 0;
    in_ECX[7] = 0;
    return;
  case 4:
    in_ECX[4] = 0x28;
    in_ECX[5] = 0x23;
    in_ECX[6] = 0;
    in_ECX[7] = 0;
    return;
  case 5:
    in_ECX[4] = 0x50;
    in_ECX[5] = 0x46;
    in_ECX[6] = 0;
    in_ECX[7] = 0;
    return;
  }
}
#endif
