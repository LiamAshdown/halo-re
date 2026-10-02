// encounter_definition_find_platoon_index_by_name  (Ghidra: encounter_definition_find_platoon_index_by_name; named for this rewrite)
// address 0x4322c0, size 80 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/tags.h ScenarioEncounter.platoons (TagReflexive at +0x8c: count @0x8c,
// pointer @0x90) and ScenarioPlatoon (size 0xac, the exact stride this loop advances by).
// Identical shape to scenario_find_encounter_index_by_name (0x432200) and
// encounter_definition_find_squad_index_by_name (0x432260), both this batch.
// register convention: unaff_ESI (inherited unchanged from the caller); param_1 is a real
// stack argument.
//   // VERIFIED against disassembly (2026-09-30): count/pointer offsets, element stride, _strnicmp(.., 0x20) and the -1 return match.
//   The register-ESI first argument is the only reason a difftest that passes it on the stack would crash the rewrite.
// blam-cc: ESI -> encounter_definition, stack -> name

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


// blam-cc: ESI -> encounter_definition, stack -> name
// Linear-searches one ScenarioEncounter's platoons for one whose name matches (case-
// insensitively, up to 32 characters), returning its index or -1 if there are none or none
// match.
int32_t encounter_definition_find_platoon_index_by_name(ScenarioEncounter *encounter_definition, char *name)
{
    int32_t index;
    uint8_t *cursor;

    if (encounter_definition->platoons.count <= 0) {
        return -1;
    }

    index = 0;
    cursor = (uint8_t *)encounter_definition->platoons.pointer;
    while (_strnicmp((char *)cursor, name, 0x20) != 0) {
        index = index + 1;
        cursor += sizeof(ScenarioPlatoon);
        if (encounter_definition->platoons.count <= index) {
            return -1;
        }
    }
    return index;
}

#if 0
Original Ghidra decompilation (0x4322c0):

int FUN_004322c0(char *param_1)

{
  int iVar1;
  int iVar2;
  int unaff_ESI;
  int iVar3;

  iVar1 = -1;
  if (0 < *(int *)(unaff_ESI + 0x8c)) {
    iVar3 = 0;
    iVar1 = 0;
    while (iVar2 = __strnicmp((char *)(*(int *)(unaff_ESI + 0x90) + iVar3),param_1,0x20), iVar2 != 0
          ) {
      iVar1 = iVar1 + 1;
      iVar3 = iVar3 + 0xac;
      if (*(int *)(unaff_ESI + 0x8c) <= iVar1) {
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
