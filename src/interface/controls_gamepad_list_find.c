// controls_gamepad_list_find  (Ghidra: FUN_004b5760, still unnamed there; named per
// src/interface/README.md's own calling-convention table)
// address 0x4b5760, size 146 bytes
// name confidence: 0.6 (chosen, matches README)   rewrite confidence: 0.85 (verified against objdump in the phase-4 review)
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: out/phase4/interface_functions.md; types/interface.h controls_gamepad_record
// (0x220 bytes, 0x14 byte key at +0x20c); the two lists controls_available_gamepads[8] /
// controls_assigned_gamepads[4] and their counts.
// register convention: EDX the list base (controls_assigned_gamepads or controls_available_gamepads); the record
// to match in the one recovered stack parameter (param_1).
//   // blam-cc: list -> EDX
// reconciled: R20 controls_gamepad_record.device_key[5] -> input_guid product_guid (+0x20c, device_key[0..3]) and int32_t product_instance (+0x21c, device_key[4])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern controls_gamepad_record controls_available_gamepads[8]; // 0x006b42d8
extern controls_gamepad_record controls_assigned_gamepads[4];   // 0x006b53d8
extern int32_t controls_assigned_gamepad_count;             // 0x00719448
extern int32_t controls_available_gamepad_count;           // 0x0071944c

// blam-cc: list -> EDX
int32_t controls_gamepad_list_find(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    int32_t count;
    int i;

    if (list == controls_assigned_gamepads) {
        count = controls_assigned_gamepad_count;
    } else if (list == controls_available_gamepads) {
        count = controls_available_gamepad_count;
    } else {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (memcmp(&list[i].product_guid, &entry->product_guid, sizeof(input_guid) + sizeof(int32_t)) /* the five-dword key */ == 0) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4b5760):

int FUN_004b5760(int param_1)

{
  int *piVar1;
  int iVar2;
  undefined4 *in_EDX;
  int *piVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  bool bVar7;

  if (in_EDX == &DAT_006b53d8) {
    piVar1 = &DAT_00719448;
  }
  else {
    if (in_EDX != &DAT_006b42d8) {
      return -1;
    }
    piVar1 = &DAT_0071944c;
  }
  iVar4 = 0;
  if (0 < *piVar1) {
    piVar3 = in_EDX + 0x83;
    do {
      if (piVar3[4] == *(int *)(param_1 + 0x21c)) {
        iVar2 = 4;
        bVar7 = true;
        piVar5 = piVar3;
        piVar6 = (int *)(param_1 + 0x20c);
        do {
          if (iVar2 == 0) break;
          iVar2 = iVar2 + -1;
          bVar7 = *piVar5 == *piVar6;
          piVar5 = piVar5 + 1;
          piVar6 = piVar6 + 1;
        } while (bVar7);
        if (bVar7) {
          return iVar4;
        }
      }
      iVar4 = iVar4 + 1;
      piVar3 = piVar3 + 0x88;
    } while (iVar4 < *piVar1);
  }
  return -1;
}
#endif
