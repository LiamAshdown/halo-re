// effect_try_and_get  (Ghidra: particle_system_try_and_get; RENAMED per
// out/phase4/effects_types_notes.md's misattribution table: "0x450630 particle_system_try_and_get
// -> effect_try_and_get" -- the stride (0xfc) and count field (effect_data +0x20) match the effe
// table, not pctl)
// address 0x450630, size 67 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/effects.h effect_data (0x0087abdc); src/memory/datum_get.c is the exact same
// validate-index-and-salt routine, called here directly instead of re-inlining it.
// register convention: effect handle in EDX (in_EDX).
//   // blam-cc: EDX -> effect_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *effect_data; // 0x0087abdc

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680,
    // blam-cc: EDX -> handle, ESI -> array

// Resolves an effect handle to its record, or NULL if stale or out of range.
effect *effect_try_and_get(datum_index effect_index)
{
    return (effect *)datum_get(effect_index, effect_data);
}

#if 0
Original Ghidra decompilation (0x450630):

short * particle_system_try_and_get(void)

{
  short *psVar1;
  short *psVar2;
  short sVar3;
  int in_EDX;
  short sVar4;

  psVar1 = (short *)0x0;
  if (((in_EDX != -1) && (sVar3 = (short)in_EDX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087abdc + 0x20))) {
    psVar2 = (short *)((int)*(short *)(DAT_0087abdc + 0x22) * (int)sVar3 +
                      *(int *)(DAT_0087abdc + 0x34));
    sVar3 = *psVar2;
    if ((sVar3 != 0) && ((sVar4 = (short)((uint)in_EDX >> 0x10), sVar4 == 0 || (sVar3 == sVar4)))) {
      psVar1 = psVar2;
    }
  }
  return psVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
