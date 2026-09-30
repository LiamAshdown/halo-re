// server_browser_gametype5_flags_pack  (Ghidra: FUN_00576960; named per this rewrite)
// address 0x576960, size 41 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary: "Encodes two small numeric fields into a
// compact code tagged with type id 5."
// register convention: two-int32 struct pointer in EDX (in_EDX).
// blam-cc: EDX -> values

// VERIFIED against disassembly 0x576960..0x576989 (2026-09-30): bits: (v0&3)<<3|5 when v0<=2 (signed), then v1 field at bits 5-6 when v1<=2
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// blam-cc: EDX -> values
// Encodes two small (0..2) numeric fields into a compact code tagged with type id 5 (the low 3
// bits); a field outside 0..2 is simply omitted from the code.
uint32_t server_browser_gametype5_flags_pack(int32_t *values)
{
    uint32_t code = 5;
    if (values[0] < 3) {
        code = ((uint32_t)values[0] & 3) << 3 | 5;
    }
    if (values[1] < 3) {
        code = code ^ (((uint32_t)values[1] & 3) << 5);
    }
    return code;
}

#if 0
Original Ghidra decompilation (0x576960):

uint FUN_00576960(void)

{
  uint uVar1;
  uint *in_EDX;

  uVar1 = 5;
  if ((int)*in_EDX < 3) {
    uVar1 = (*in_EDX & 3) << 3 | 5;
  }
  if ((int)in_EDX[1] < 3) {
    uVar1 = uVar1 ^ (in_EDX[1] & 3) << 5;
  }
  return uVar1;
}
#endif
