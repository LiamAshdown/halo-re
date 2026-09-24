// hs_script_find_by_name  (Ghidra: hs_script_find_by_name, already named)
// address 0x4833a0, size 120 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: linear scan over Scenario::scripts (offset 0x49c/0x4a0, stride 0x5c, confirmed in
// out/phase4/hs_types_notes.md) comparing the TagString name field. The hand-written two-bytes-
// at-a-time compare only ever tests the result for equality (never uses the sign it also
// computes), which is exactly strcmp's contract, so it is rewritten as a plain strcmp.
// register convention: __cdecl, name is the recognized single stack parameter.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <string.h>

extern Scenario *global_scenario;         // 0x00746f8c
extern datum_index global_scenario_index; // 0x0069e8d4

// Returns the index of the scenario script named `name`, or -1 if no scenario is loaded or no
// script matches.
int16_t hs_script_find_by_name(char *name)
{
    ScenarioScript *scripts;
    int32_t count;
    int32_t i;

    if (global_scenario_index == k_datum_index_none) {
        return -1;
    }
    count = (int32_t)global_scenario->scripts.count;
    if (0 < count) {
        scripts = (ScenarioScript *)global_scenario->scripts.pointer;
        for (i = 0; i < count; i++) {
            if (strcmp((char *)name, scripts[i].name.string) == 0) {
                return (int16_t)i;
            }
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4833a0):

short hs_script_find_by_name(byte *param_1)

{
  byte bVar1;
  int iVar2;
  byte *pbVar3;
  byte *pbVar4;
  short sVar5;
  bool bVar6;

  if (DAT_0069e8d4 != -1) {
    sVar5 = 0;
    if (0 < *(int *)(DAT_00746f8c + 0x49c)) {
      iVar2 = 0;
      do {
        pbVar3 = (byte *)(iVar2 * 0x5c + *(int *)(DAT_00746f8c + 0x4a0));
        pbVar4 = param_1;
        do {
          bVar1 = *pbVar4;
          bVar6 = bVar1 < *pbVar3;
          if (bVar1 != *pbVar3) {
LAB_004833f6:
            iVar2 = (1 - (uint)bVar6) - (uint)(bVar6 != 0);
            goto LAB_004833fb;
          }
          if (bVar1 == 0) break;
          bVar1 = pbVar4[1];
          bVar6 = bVar1 < pbVar3[1];
          if (bVar1 != pbVar3[1]) goto LAB_004833f6;
          pbVar4 = pbVar4 + 2;
          pbVar3 = pbVar3 + 2;
        } while (bVar1 != 0);
        iVar2 = 0;
LAB_004833fb:
        if (iVar2 == 0) {
          return sVar5;
        }
        sVar5 = sVar5 + 1;
        iVar2 = (int)sVar5;
      } while (iVar2 < *(int *)(DAT_00746f8c + 0x49c));
    }
  }
  return -1;
}
#endif
