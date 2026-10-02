// ai_reference_expand_to_platoon_range  (Ghidra: ai_reference_expand_to_platoon_range; named for this rewrite)
// address 0x432420, size 197 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: decodes the same packed reference ai_reference_parse (0x432320, this batch)
// produces: bits 0..15 encounter index, bits 30..31 a kind selector (0 plain encounter, 1
// platoon, 2 squad, per the 0x4000/0xffff8000 high halves ai_reference_parse ORs in) and, for
// a platoon or squad reference, the sub-index in bits 16..23. Resolves a squad reference to
// its owning platoon (ScenarioSquad.platoon at +0x22, matching encounter_definition_find_
// squad_index_by_name's stride) and a plain encounter reference to the full platoon range
// 0..count-1, which is why the output is a {start, end} pair rather than one index -- this
// is the platoon-level counterpart of ai_reference_squad_iterator_new/_next (0x4324f0/
// 0x4325b0, this batch), which does the same expansion one level down (squads).
// Ghidra resolves the packed reference as an ordinary parameter (EAX) and leaves the output
// pointer as an inherited register, EDX here.
//
// UNSURE: out_range->platoon_start/platoon_end at ai_globals validity gate: also checks
// ai_globals.actors_valid (0x00880354+1) and Scenario.encounters.count, exactly like
// ai_reference_squad_iterator_new; kept as raw ai_globals offsets there is no name for.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// TYPES-GAP: the 3-dword out-parameter of ai_reference_expand_to_platoon_range. No existing
// header struct matches; only this function and its callers use it.
extern Scenario *global_scenario; // 0x00746f8c
extern ai_globals *ai_globals_ptr; // 0x00880354

// Expands a packed ai reference (see ai_reference_parse) to the range of platoon indices it
// names within its encounter: the single platoon a platoon reference names, the owning
// platoon of a squad reference, or every platoon in the encounter for a plain encounter
// reference. out_range->encounter_index is set to -1 (and the rest left as found) if the
// reference, the scenario or the AI globals are not valid.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: EDX -> out_range, stack -> packed_reference
void ai_reference_expand_to_platoon_range(uint32_t packed_reference, ai_reference_platoon_range *out_range)
{
    uint32_t encounter_index = packed_reference & 0xffff;
    ScenarioEncounter *encounter_definition;
    uint32_t kind;
    uint32_t platoon_index;

    out_range->encounter_index = (int32_t)encounter_index;

    if (global_scenario == 0 || ai_globals_ptr->actors_valid == 0 ||
        (int32_t)global_scenario->encounters.count <= (int32_t)encounter_index) {
        goto fail;
    }

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index];
    kind = packed_reference >> 0x1e;

    if (kind == 0) {
        out_range->platoon_start = 0;
        out_range->platoon_end = encounter_definition->platoons.count - 1;
        return;
    }
    if (kind > 2) {
        goto fail;
    }

    platoon_index = (packed_reference >> 0x10) & 0xff;
    if (kind == 2) {
        if ((int32_t)platoon_index < encounter_definition->squads.count) {
            ScenarioSquad *squad = &((ScenarioSquad *)encounter_definition->squads.pointer)[platoon_index];
            platoon_index = squad->platoon;
        } else {
            out_range->platoon_start = -1;
            goto validate;
        }
    }
    out_range->platoon_start = (int32_t)platoon_index;

validate:
    platoon_index = out_range->platoon_start;
    if ((int32_t)platoon_index >= 0 && (int32_t)platoon_index < encounter_definition->platoons.count) {
        out_range->platoon_end = platoon_index;
        return;
    }

fail:
    out_range->encounter_index = -1;
}

#if 0
Original Ghidra decompilation (0x432420):

void FUN_00432420(uint param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint *in_EDX;
  bool bVar4;

  iVar2 = global_scenario;
  uVar1 = param_1 & 0xffff;
  bVar4 = global_scenario == 0;
  *in_EDX = uVar1;
  if (((bVar4) || (*(char *)(DAT_00880354 + 1) == '\0')) || (*(int *)(iVar2 + 0x42c) <= (int)uVar1))
  goto LAB_004324db;
  uVar3 = param_1 >> 0x1e;
  iVar2 = uVar1 * 0xb0 + *(int *)(iVar2 + 0x430);
  if (uVar3 == 0) {
    in_EDX[1] = 0;
    in_EDX[2] = *(int *)(iVar2 + 0x8c) - 1;
    return;
  }
  if (2 < uVar3) goto LAB_004324db;
  uVar1 = param_1 >> 0x10 & 0xff;
  if (uVar3 == 2) {
    if ((int)uVar1 < *(int *)(iVar2 + 0x80)) {
      uVar1 = (uint)*(short *)(uVar1 * 0xe8 + 0x22 + *(int *)(iVar2 + 0x84));
      goto LAB_004324a4;
    }
    in_EDX[1] = 0xffffffff;
  }
  else {
LAB_004324a4:
    in_EDX[1] = uVar1;
  }
  uVar1 = in_EDX[1];
  if ((-1 < (int)uVar1) && ((int)uVar1 < *(int *)(iVar2 + 0x8c))) {
    in_EDX[2] = uVar1;
    return;
  }
LAB_004324db:
  *in_EDX = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
