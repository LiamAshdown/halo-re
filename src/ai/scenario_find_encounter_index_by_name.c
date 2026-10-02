// scenario_find_encounter_index_by_name  (Ghidra: scenario_find_encounter_index_by_name; named for this rewrite)
// address 0x432200, size 80 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/tags.h Scenario.encounters (TagReflexive at +0x42c: count @0x42c, pointer
// @0x430) and ScenarioEncounter (size 0xb0, the exact stride this loop advances by). The
// phase-4 summary ("looks up a squad definition's index... within the encounter's squad
// table") is one hierarchy level off: this operates on Scenario.encounters directly, not on
// a squad table (see encounter_definition_find_squad_index_by_name @0x432260, this batch,
// for the function that actually does that).
// register convention: unaff_ESI (inherited unchanged from the caller, i.e. a genuine
// implicit parameter Ghidra could not attach to the signature); param_1 is a real stack
// argument.
//   // VERIFIED against disassembly (2026-09-30): count/pointer offsets, element stride, _strnicmp(.., 0x20) and the -1 return match.
//   The register-ESI first argument is the only reason a difftest that passes it on the stack would crash the rewrite.
// blam-cc: ESI -> scenario, stack -> name

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>


// blam-cc: ESI -> scenario, stack -> name
// Linear-searches Scenario.encounters for one whose name matches (case-insensitively, up to
// 32 characters), returning its index or -1 if there are no encounters or none match.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t scenario_find_encounter_index_by_name(Scenario *scenario, char *name)
{
    int32_t index;
    uint8_t *cursor;

    if (scenario->encounters.count <= 0) {
        return -1;
    }

    index = 0;
    cursor = (uint8_t *)scenario->encounters.pointer;
    while (_strnicmp((char *)cursor, name, 0x20) != 0) {
        index = index + 1;
        cursor += sizeof(ScenarioEncounter);
        if (scenario->encounters.count <= index) {
            return -1;
        }
    }
    return index;
}

#if 0
Original Ghidra decompilation (0x432200):

int FUN_00432200(char *param_1)

{
  int iVar1;
  int iVar2;
  int unaff_ESI;
  int iVar3;

  iVar1 = -1;
  if (0 < *(int *)(unaff_ESI + 0x42c)) {
    iVar3 = 0;
    iVar1 = 0;
    while (iVar2 = __strnicmp((char *)(*(int *)(unaff_ESI + 0x430) + iVar3),param_1,0x20),
          iVar2 != 0) {
      iVar1 = iVar1 + 1;
      iVar3 = iVar3 + 0xb0;
      if (*(int *)(unaff_ESI + 0x42c) <= iVar1) {
        return -1;
      }
    }
  }
  return iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
