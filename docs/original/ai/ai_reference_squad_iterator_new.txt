// ai_reference_squad_iterator_new  (Ghidra: ai_reference_squad_iterator_new; named for this rewrite)
// address 0x4324f0, size 179 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED against objdump)
// evidence: decodes a packed ai reference (see ai_reference_parse, 0x432320) the same way
// as ai_reference_expand_to_platoon_range (0x432420, this batch), but one level down: a
// squad reference becomes a single-squad range, a platoon reference becomes every squad in
// that platoon (filtered on the fly by its partner, ai_reference_squad_iterator_next @
// 0x4325b0, this batch, via ScenarioSquad.platoon), and a plain encounter reference becomes
// every squad in the encounter. The phase-4 summary ("iterator over all starting locations")
// is wrong about the level: ai_reference_squad_iterator_next returns an
// encounter_squad_state pointer (encounter.first_squad + cursor, stride 0x20), not a
// starting location.
// the packed reference is an ordinary parameter (EAX), the output pointer is inherited, ECX
// here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

// TYPES-GAP: the 5-dword state ai_reference_squad_iterator_new/_next (0x4324f0/0x4325b0)
// share. No existing header struct matches.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c
extern ai_globals *ai_globals_ptr; // 0x00880354

// Initializes out_iterator to walk every ScenarioSquad a packed ai reference names: one
// squad for a squad reference, every squad of the named platoon (filtered by
// ai_reference_squad_iterator_next) for a platoon reference, or every squad in the
// encounter for a plain encounter reference. out_iterator->encounter_index is left as -1 if
// the reference, the scenario or the AI globals are not valid.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: ECX -> out_iterator, stack -> packed_reference
void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator)
{
    uint32_t encounter_index = packed_reference & 0xffff;
    ScenarioEncounter *encounter_definition;
    uint32_t kind;

    out_iterator->encounter_index = (int32_t)encounter_index;

    if (global_scenario == 0 || ai_globals_ptr->actors_valid == 0 ||
        (int32_t)global_scenario->encounters.count <= (int32_t)encounter_index) {
        goto fail;
    }

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index];
    kind = packed_reference >> 0x1e;

    if (kind < 2) {
        out_iterator->cursor = -1;
        out_iterator->squad_start = 0;
        out_iterator->squad_end = encounter_definition->squads.count - 1;
        if (kind != 0) {
            out_iterator->platoon_filter = (int8_t)(packed_reference >> 0x10);
            return;
        }
    } else {
        uint32_t squad_index = (int8_t)(packed_reference >> 0x10);
        if (kind != 2 || (int32_t)squad_index >= encounter_definition->squads.count) {
            goto fail;
        }
        out_iterator->cursor = -1;
        out_iterator->squad_end = (int32_t)squad_index;
        out_iterator->squad_start = (int32_t)squad_index;
    }
    out_iterator->platoon_filter = -1;
    return;

fail:
    out_iterator->encounter_index = -1;
}

#if 0
Original Ghidra decompilation (0x4324f0):

void FUN_004324f0(uint param_1)

{
  uint uVar1;
  int iVar2;
  uint *in_ECX;
  uint uVar3;
  bool bVar4;

  iVar2 = global_scenario;
  uVar1 = param_1 & 0xffff;
  bVar4 = global_scenario != 0;
  *in_ECX = uVar1;
  if (((bVar4) && (*(char *)(DAT_00880354 + 1) != '\0')) && ((int)uVar1 < *(int *)(iVar2 + 0x42c)))
  {
    uVar3 = param_1 >> 0x1e;
    iVar2 = uVar1 * 0xb0 + *(int *)(iVar2 + 0x430);
    if (uVar3 < 2) {
      in_ECX[2] = 0xffffffff;
      in_ECX[3] = 0;
      in_ECX[4] = *(int *)(iVar2 + 0x80) - 1;
      if (uVar3 != 0) {
        in_ECX[1] = (uint)param_1._2_1_;
        return;
      }
    }
    else {
      if ((uVar3 != 2) ||
         (uVar1 = (uint)(short)(ushort)param_1._2_1_, *(int *)(iVar2 + 0x80) <= (int)uVar1))
      goto LAB_00432599;
      in_ECX[2] = 0xffffffff;
      in_ECX[4] = uVar1;
      in_ECX[3] = uVar1;
    }
    in_ECX[1] = 0xffffffff;
    return;
  }
LAB_00432599:
  *in_ECX = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
