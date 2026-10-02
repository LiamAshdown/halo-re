// effects_update_all  (Ghidra: FUN_00450aa0, still unnamed there; named from its own summary in
// out/phase4/effects_functions.md: "Per-frame driver that advances every active particle system
// by the given time delta" -- corrected to "effect" per the misattribution table, since it walks
// effect_data (0x0087abdc) with particle_system_update, itself renamed effect_update)
// address 0x450aa0, size 120 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: types/effects.h effect_data; src/memory/datum_next.c is byte-for-byte the tail scan
// this function inlines (see contrail_update.c for the identical simplification).
// register convention: __cdecl, delta_time on the stack (Ghidra's own recognised param_1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *effect_data; // 0x0087abdc

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630,
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern void effect_update(datum_index effect_handle, real delta_time); // 0x451a30, this module

// Per-tick driver: advances every live effect by `delta_time`.
void effects_update_all(real delta_time)
{
    datum_index effect_index = datum_next(-1, effect_data);

    while (effect_index != k_datum_index_none) {
        effect_update(effect_index, delta_time);
        effect_index = datum_next((int16_t)effect_index, effect_data);
    }
}

#if 0
Original Ghidra decompilation (0x450aa0):

void FUN_00450aa0(float param_1)

{
  short sVar1;
  uint particle_system_index;
  int iVar2;
  short *psVar3;

  particle_system_index = datum_next();
  do {
    do {
      if (particle_system_index == 0xffffffff) {
        return;
      }
      particle_system_update(particle_system_index,param_1);
      iVar2 = particle_system_index + 1;
      particle_system_index = 0xffffffff;
      sVar1 = (short)iVar2;
    } while ((sVar1 < 0) || (*(short *)(DAT_0087abdc + 0x2e) <= sVar1));
    psVar3 = (short *)((int)sVar1 * (int)*(short *)(DAT_0087abdc + 0x22) +
                      *(int *)(DAT_0087abdc + 0x34));
    do {
      if (*psVar3 != 0) {
        particle_system_index = (int)*psVar3 << 0x10 | (int)(short)iVar2;
        break;
      }
      iVar2 = iVar2 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(DAT_0087abdc + 0x22));
    } while ((short)iVar2 < *(short *)(DAT_0087abdc + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
