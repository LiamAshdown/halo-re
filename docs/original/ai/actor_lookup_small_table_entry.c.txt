// actor_lookup_small_table_entry  (Ghidra: actor_lookup_small_table_entry, renamed)
// address 0x40e790, size 25 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: phase-4 summary "looks up an entry in a small fixed-size table by index,
// returning zero if the index is out of range"; bound-checks 0 <= index < 12.
// register convention: index in CX (Ghidra's in_CX).
// blam-cc: CX -> index
// UNSURE: the table at 0x006555a8 is not named anywhere else in types/ai.h; declared here
// as a plain int16 array of at least 12 entries.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t actor_lookup_table_006555a8[12]; // 0x006555a8, UNSURE: size only known to be >= 12

// blam-cc: CX -> index
int32_t actor_lookup_small_table_entry(int16_t index)
{
    if (-1 < index && index < 0xc) {
        return actor_lookup_table_006555a8[index];
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40e790):

undefined4 FUN_0040e790(void)

{
  undefined4 uVar1;
  short in_CX;

  uVar1 = 0;
  if ((-1 < in_CX) && (in_CX < 0xc)) {
    uVar1 = CONCAT22(in_CX >> 0xf,*(undefined2 *)(&DAT_006555a8 + in_CX * 2));
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
