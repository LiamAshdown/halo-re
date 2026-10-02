// server_browser_gametype3_flags_unpack  (Ghidra: FUN_00576a20; named per this rewrite)
// address 0x576a20, size 75 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary: "Decodes the compact type-3 code
// produced by server_browser_gametype3_flags_pack back into its booleans and numeric fields."
// Exact bit-for-bit mirror of server_browser_gametype3_flags_pack.c.
// register convention: code in EAX (in_EAX), destination struct in ECX (in_ECX).
// blam-cc: EAX -> code, ECX -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


// blam-cc: EAX -> code, ECX -> out
// Decodes the compact type-3 code produced by server_browser_gametype3_flags_pack back into its
// two booleans and five numeric fields.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void server_browser_gametype3_flags_unpack(uint32_t code, server_browser_gametype3_options *out)
{
    out->flag0 = (uint8_t)(code >> 3) & 1;
    out->flag1 = (uint8_t)(code >> 4) & 1;
    out->value_04 = (int32_t)((code >> 5) & 3);
    out->value_08 = (int32_t)((code >> 7) & 3);
    out->value_0c = (int32_t)((code >> 9) & 3);
    out->value_10 = (int32_t)((code >> 0xb) & 3);
    out->value_14 = (int32_t)((code >> 0xd) & 0x1f);
}

#if 0
Original Ghidra decompilation (0x576a20):

void FUN_00576a20(void)

{
  uint in_EAX;
  byte *in_ECX;

  *in_ECX = (byte)(in_EAX >> 3) & 1;
  in_ECX[1] = (byte)(in_EAX >> 4) & 1;
  *(uint *)(in_ECX + 4) = in_EAX >> 5 & 3;
  *(uint *)(in_ECX + 8) = in_EAX >> 7 & 3;
  *(uint *)(in_ECX + 0xc) = in_EAX >> 9 & 3;
  *(uint *)(in_ECX + 0x10) = in_EAX >> 0xb & 3;
  *(uint *)(in_ECX + 0x14) = in_EAX >> 0xd & 0x1f;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
