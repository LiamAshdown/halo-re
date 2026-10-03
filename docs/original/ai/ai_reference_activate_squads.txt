// ai_reference_activate_squads  (Ghidra: ai_reference_activate_squads; named for this rewrite)
// address 0x432b80, size 79 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: decodes a packed ai reference the same way as its siblings in this batch and
// forwards it to encounter_spawn_squads (encounter_spawn_squads, this batch) as
// (encounter_index, platoon_filter, squad_filter) -- matching that function's own
// phase-4 summary ("spawns the actors for a squad's starting locations").
// register convention: Ghidra fully resolved the parameter.
//   // blam-cc: stack -> packed_reference (EAX, per sibling convention; not independently
//   confirmed with objdump for this file)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void encounter_spawn_squads(uint32_t encounter_index, int32_t platoon_filter,
                                                int32_t squad_filter); // 0x437510, this batch

// blam-cc: stack -> packed_reference
// Unpacks a packed ai reference and forwards it to encounter_spawn_squads: a
// squad reference names one squad (squad_filter set, platoon_filter none), a platoon
// reference names one platoon (platoon_filter set, squad_filter none, matching the byte's
// sign-extending decode Ghidra shows here), and a plain encounter reference names neither
// (both none, i.e. every squad in the encounter).
void ai_reference_activate_squads(uint32_t packed_reference)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        int32_t squad_filter = (packed_reference >> 0x1e == 2) ? (int32_t)(int8_t)(packed_reference >> 0x10) : -1;

        if (packed_reference >> 0x1e == 1) {
            encounter_spawn_squads(packed_reference & 0xffff, (int32_t)(int8_t)(packed_reference >> 0x10),
                                               squad_filter);
            return;
        }
        encounter_spawn_squads(packed_reference & 0xffff, -1, squad_filter);
    }
}

#if 0
Original Ghidra decompilation (0x432b80):

void FUN_00432b80(uint param_1)

{
  uint uVar1;

  if (param_1 != 0xffffffff) {
    if (param_1 >> 0x1e == 2) {
      uVar1 = (uint)param_1._2_1_;
    }
    else {
      uVar1 = 0xffffffff;
    }
    if (param_1 >> 0x1e == 1) {
      FUN_00437510(param_1 & 0xffff,param_1._2_1_,uVar1);
      return;
    }
    FUN_00437510(param_1 & 0xffff,0xffffffff,uVar1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
