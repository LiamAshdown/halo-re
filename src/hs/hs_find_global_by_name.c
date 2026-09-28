// hs_find_global_by_name  (Ghidra: chimera__get_global_index; renamed per
// out/phase4/hs_types_notes.md -- "chimera__ prefix is a Chimera symbol name, not a Bungie
// one", the function itself is an engine builtin.)
// address 0x483480, size 145 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: scans hs_global_definitions (0x1eb entries, k_hs_builtin_global_count) then, if a
// scenario is loaded, Scenario::globals (offset 0x4a8/0x4ac, stride 0x5c), matching every use
// site that expects a packed hs_global_reference back.
// register convention: the searched name is unrecognized by Ghidra (unaff_EBX); by the
// blam-cc convention this is the fourth register slot, EBX.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"


extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern Scenario *global_scenario;          // 0x00746f8c
extern datum_index global_scenario_index;  // 0x0069e8d4

// blam-cc: searched name in EBX
// Resolves a global-variable name to a packed hs_global_reference: a builtin index with
// k_hs_global_builtin_bit set, a scenario-global index with it clear, or
// k_hs_global_reference_none if nothing matches.
hs_global_reference hs_find_global_by_name(char *name)
{
    uint16_t index;
    ScenarioGlobal *globals;
    int32_t count;
    int32_t i;

    index = 0;
    do {
        if (_stricmp(name, hs_global_definitions[index]->name) == 0) {
            return (hs_global_reference)(index | k_hs_global_builtin_bit);
        }
        index = index + 1;
    } while ((int16_t)index < k_hs_builtin_global_count);

    if (global_scenario_index != k_datum_index_none) {
        count = (int32_t)global_scenario->globals.count;
        if (0 < count) {
            globals = (ScenarioGlobal *)global_scenario->globals.pointer;
            for (i = 0; i < count; i++) {
                if (_stricmp(name, globals[i].name.string) == 0) {
                    return (hs_global_reference)(i & k_hs_global_index_mask);
                }
            }
        }
    }
    return k_hs_global_reference_none;
}

#if 0
Original Ghidra decompilation (0x483480):

ushort chimera__get_global_index(void)

{
  int iVar1;
  char *unaff_EBX;
  ushort uVar2;
  int *piVar3;

  uVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EBX,*(char **)(&PTR_PTR_0068b398)[(short)uVar2]);
    if (iVar1 == 0) {
      return uVar2 | 0x8000;
    }
    uVar2 = uVar2 + 1;
  } while ((short)uVar2 < 0x1eb);
  if (DAT_0069e8d4 != -1) {
    piVar3 = (int *)(DAT_00746f8c + 0x4a8);
    uVar2 = 0;
    if (0 < *(int *)(DAT_00746f8c + 0x4a8)) {
      iVar1 = 0;
      do {
        iVar1 = __stricmp(unaff_EBX,(char *)(iVar1 * 0x5c + *(int *)(DAT_00746f8c + 0x4ac)));
        if (iVar1 == 0) {
          return uVar2 & 0x7fff;
        }
        uVar2 = uVar2 + 1;
        iVar1 = (int)(short)uVar2;
      } while (iVar1 < *piVar3);
    }
  }
  return 0xffff;
}
#endif
