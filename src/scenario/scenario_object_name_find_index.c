// scenario_object_name_find_index  (Ghidra: FUN_0053ebb0, still unnamed there; name from
// out/phase2/results/scenario_00.json)
// address 0x53ebb0, size 109 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase2/results/scenario_00.json: "Linearly scans a count/array pair at
// in_ECX+0x204/+0x208 (36-byte stride) comparing each element's leading bytes against param_1
// with an unrolled byte-compare loop, returning the matching element's index or 0xffff."
// types/tags.h Scenario.object_names (+0x204/+0x208, stride 0x24 ScenarioObjectName) matches
// exactly. The inlined comparison loop (raw disassembly 0x53ebe0-0x53ec06) is a byte-for-byte
// exact-match scan with no case folding, i.e. an inlined strcmp, replaced here with strcmp.
// register convention: ECX -> scenario (Scenario *), stack -> name (char *). Returns an int16_t
// index, or -1 if no object name matches.
//   // blam-cc: ECX -> scenario, stack -> name

#include "tags.h"
#include "scenario.h"
#include "fn_scenario.h"
#include <string.h> // strcmp: an inlined byte-compare loop in the original

// blam-cc: ECX -> scenario, stack -> name
// Linear-scans scenario->object_names for an entry whose name exactly matches `name`,
// returning its index, or -1 if none does.
int16_t scenario_object_name_find_index(Scenario *scenario, char *name)
{
    ScenarioObjectName *names = (ScenarioObjectName *)scenario->object_names.pointer;
    int16_t i;

    for (i = 0; i < (int32_t)scenario->object_names.count; i++) {
        if (strcmp(names[i].name.string, name) == 0) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x53ebb0):

undefined4 FUN_0053ebb0(byte *param_1)

{
  byte bVar1;
  int iVar2;
  undefined2 uVar3;
  int in_ECX;
  int iVar4;
  byte *pbVar5;
  byte *pbVar6;
  short sVar7;
  bool bVar8;

  iVar2 = *(int *)(in_ECX + 0x204);
  sVar7 = 0;
  uVar3 = (undefined2)((uint)iVar2 >> 0x10);
  if (0 < iVar2) {
    iVar4 = 0;
    do {
      pbVar5 = (byte *)(*(int *)(in_ECX + 0x208) + iVar4 * 0x24);
      pbVar6 = param_1;
      do {
        bVar1 = *pbVar5;
        bVar8 = bVar1 < *pbVar6;
        if (bVar1 != *pbVar6) {
LAB_0053ec04:
          iVar4 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
          goto LAB_0053ec09;
        }
        if (bVar1 == 0) break;
        bVar1 = pbVar5[1];
        bVar8 = bVar1 < pbVar6[1];
        if (bVar1 != pbVar6[1]) goto LAB_0053ec04;
        pbVar5 = pbVar5 + 2;
        pbVar6 = pbVar6 + 2;
      } while (bVar1 != 0);
      iVar4 = 0;
LAB_0053ec09:
      if (iVar4 == 0) {
        return CONCAT22(uVar3,sVar7);
      }
      sVar7 = sVar7 + 1;
      iVar4 = (int)sVar7;
    } while (iVar4 < iVar2);
  }
  return CONCAT22(uVar3,0xffff);
}

Raw disassembly (0x53ebb0-0x53ec25) confirming ECX -> scenario and the object_names field
offsets/stride:

  53ebb0: mov    eax,DWORD PTR [ecx+0x204]     ; object_names.count
  ...
  53ebc0: mov    ebp,DWORD PTR [ecx+0x208]     ; object_names.pointer
  53ebd4: lea    ecx,[ecx+ecx*8]
  53ebd7: lea    ecx,[ebp+ecx*4+0x0]           ; &names[i]  (i*0x24 == i*9*4)
  53ebe0: mov    bl,BYTE PTR [ecx]             ; inlined byte-compare loop (strcmp)
  ...
  53ec18: or     ax,0xffff                     ; not found: return -1
  53ec1e: mov    ax,di                         ; found: return index
#endif
