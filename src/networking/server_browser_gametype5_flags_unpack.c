// server_browser_gametype5_flags_unpack  (Ghidra: FUN_00576990; named per this rewrite)
// address 0x576990, size 48 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary: "Decodes the compact type-5 code
// produced by server_browser_gametype5_flags_pack back into two clamped numeric fields."
// register convention: code in ECX (in_ECX), destination two-int32 struct in EDX (in_EDX).
// blam-cc: ECX -> code, EDX -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: ECX -> code, EDX -> out
// Decodes the compact type-5 code produced by server_browser_gametype5_flags_pack back into two
// numeric fields, clamped to 0..2 (a decoded value of 3 is dropped to 0).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void server_browser_gametype5_flags_unpack(uint32_t code, int32_t *out)
{
    uint32_t a = (code >> 3) & 3;
    uint32_t b = (code >> 5) & 3;
    out[0] = (a <= 2) ? (int32_t)a : 0;
    out[1] = (b <= 2) ? (int32_t)b : 0;
}

#if 0
Original Ghidra decompilation (0x576990):

void FUN_00576990(void)

{
  uint uVar1;
  uint in_ECX;
  uint uVar2;
  uint *in_EDX;

  uVar1 = in_ECX >> 3 & 3;
  uVar2 = in_ECX >> 5 & 3;
  *in_EDX = ~-(uint)(2 < uVar1) & uVar1;
  in_EDX[1] = ~-(uint)(2 < uVar2) & uVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
