// controls_gamepad_list_remove  (Ghidra: FUN_004b5850, still unnamed there; named per
// src/interface/README.md's own calling-convention table)
// address 0x4b5850, size 127 bytes
// name confidence: 0.6 (chosen, matches README)   rewrite confidence: 0.85 (verified against objdump in the phase-4 review)
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: out/phase4/interface_functions.md "Removes the entry matching the caller's key
// from whichever gamepad list is addressed by a register pointer, compacting the array
// and decrementing its count."
// register convention: EDI the list base (unaff_EDI -- never assigned in this function, so
// it must be a register argument its caller sets up, exactly like controls_gamepad_list_find's own EDX
// argument); the record to match in the one recovered stack parameter (param_1).
//   // blam-cc: list -> EDI

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

extern int32_t controls_gamepad_list_find(const controls_gamepad_record *entry, controls_gamepad_record *list); // 0x4b5760, this module

// blam-cc: list -> EDI
uint8_t controls_gamepad_list_remove(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    int32_t index = controls_gamepad_list_find(entry, list);
    int32_t *count;
    int32_t tail_count;

    if (index == -1) {
        return 0;
    }
    if (list == controls_assigned_gamepads) {
        count = &controls_assigned_gamepad_count;
    } else if (list == controls_available_gamepads) {
        count = &controls_available_gamepad_count;
    } else {
        return 0;
    }

    index = controls_gamepad_list_find(entry, list);
    if (index == -1) {
        return 0;
    }
    tail_count = (*count - 1) - index;
    (*count)--;
    if (tail_count > 0) {
        memmove(&list[index], &list[index + 1], tail_count * sizeof(controls_gamepad_record));
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b5850):

undefined1 FUN_004b5850(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  undefined1 uVar3;
  int *piVar4;
  undefined4 *unaff_EDI;

  uVar3 = 0;
  iVar1 = FUN_004b5760(param_1);
  if (iVar1 != -1) {
    if (unaff_EDI == &DAT_006b53d8) {
      piVar4 = &DAT_00719448;
    }
    else {
      if (unaff_EDI != &DAT_006b42d8) {
        return 0;
      }
      piVar4 = &DAT_0071944c;
    }
    iVar1 = FUN_004b5760(param_1);
    if (iVar1 != -1) {
      iVar2 = (*piVar4 + -1) - iVar1;
      *piVar4 = *piVar4 + -1;
      if (0 < iVar2) {
        _memmove(unaff_EDI + iVar1 * 0x88,unaff_EDI + iVar1 * 0x88 + 0x88,iVar2 * 0x220);
      }
      uVar3 = 1;
    }
  }
  return uVar3;
}
#endif
