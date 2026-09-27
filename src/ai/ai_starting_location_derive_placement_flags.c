// ai_starting_location_derive_placement_flags  (Ghidra: ai_starting_location_derive_placement_flags; named for this rewrite)
// address 0x436d40, size 128 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop: objdump 0x436d40..0x436dbf and the one caller 0x40cfd9, every out slot matches)
// evidence: reads a squad's ScenarioActorStartingLocation attribute bits and derives a set
// of output flags across six distinct output pointers (matching the phase-4 summary), four
// of which are caller-inherited registers/stack slots Ghidra could not resolve at all; kept
// as raw parameters by position rather than guessed names.
// register convention: Ghidra resolved param_2..param_5 as ordinary stack parameters and
// left the encounter index and two more output pointers unresolved (EAX, EDX, ESI).
//   // blam-cc: EAX -> encounter_index, EDX -> out_edx, ESI -> out_esi, stack ->
//   starting_location_index, out_a, out_b, out_c, out_d

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: EAX -> encounter_index, EDX -> out_edx, ESI -> out_esi, stack ->
//   starting_location_index, out_a, out_b, out_c, out_d
//
// UNSURE: parameter and field roles below are preserved by position from Ghidra's own
// (incomplete) decompile, not independently re-derived. out_a/out_c are byte pointers
// (param_2/param_4); out_b/out_d/out_edx/out_esi are int16 pointers (param_3/param_5/
// in_EDX/unaff_ESI).
void ai_starting_location_derive_placement_flags(datum_index encounter_index, int16_t starting_location_index,
                                                  uint8_t *out_a, int16_t *out_b, uint8_t *out_c, int16_t *out_d,
                                                  int16_t *out_edx, int16_t *out_esi)
{
    ScenarioEncounter *encounter_definition =
        &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];
    ScenarioSquad *squads = (ScenarioSquad *)encounter_definition->squads.pointer;
    int16_t category = *(int16_t *)((uint8_t *)encounter_definition + 0x28); // UNSURE: no established field at this offset

    if ((*((uint8_t *)squads + starting_location_index * 0xe8 + 0x28) & 2) != 0) {
        category = 1;
    }

    if (category == 1) {
        *out_b = 1;
        *out_esi = 2;
        *out_edx = 2;
    } else if (category == 2) {
        *out_a = 1;
        *out_d = 0;
        *out_esi = 0;
        *out_edx = 0;
        *out_c = 0;
    }
}

#if 0
Original Ghidra decompilation (0x436d40):

void FUN_00436d40(short param_1,undefined1 *param_2,undefined2 *param_3,undefined1 *param_4,
                 undefined2 *param_5)

{
  uint in_EAX;
  int iVar1;
  short sVar2;
  undefined2 *in_EDX;
  undefined2 *unaff_ESI;

  iVar1 = (in_EAX & 0xffff) * 0xb0;
  sVar2 = *(short *)(iVar1 + 0x28 + *(int *)(global_scenario + 0x430));
  if ((*(byte *)(param_1 * 0xe8 + 0x28 + *(int *)(iVar1 + *(int *)(global_scenario + 0x430) + 0x84))
      & 2) != 0) {
    sVar2 = 1;
  }
  if (sVar2 == 1) {
    *param_3 = 1;
    *unaff_ESI = 2;
    *in_EDX = 2;
  }
  else if (sVar2 == 2) {
    *param_2 = 1;
    *param_5 = 0;
    *unaff_ESI = 0;
    *in_EDX = 0;
    *param_4 = 0;
    return;
  }
  return;
}
#endif
