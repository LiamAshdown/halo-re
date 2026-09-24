// controls_gamepad_list_add  (Ghidra: FUN_004b5800, still unnamed there; named per
// src/interface/README.md's own calling-convention table)
// address 0x4b5800, size 80 bytes
// name confidence: 0.6 (chosen, matches README)   rewrite confidence: 0.85 (verified against objdump in the phase-4 review)
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: out/phase4/interface_functions.md "Appends a 0x88-dword entry to whichever of
// the two gamepad lists is addressed by a register pointer, if that list has not
// reached its capacity (4 or 8), returning success."; types/interface.h controls_gamepad_record.
// register convention: EAX the list base; the record to copy in the one recovered stack
// parameter (param_1).
//   // blam-cc: list -> EAX

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

// blam-cc: list -> EAX
uint8_t controls_gamepad_list_add(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    int32_t capacity;
    int32_t *count;

    if (list == controls_assigned_gamepads) {
        capacity = 4;
        count = &controls_assigned_gamepad_count;
    } else if (list == controls_available_gamepads) {
        capacity = 8;
        count = &controls_available_gamepad_count;
    } else {
        return 0;
    }

    if (*count < capacity) {
        list[*count] = *entry;
        (*count)++;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4b5800):

undefined4 FUN_004b5800(undefined4 *param_1)

{
  undefined4 *in_EAX;
  undefined4 uVar1;
  int iVar2;
  int *piVar3;
  undefined4 *puVar4;

  uVar1 = 0;
  if (in_EAX == &DAT_006b53d8) {
    iVar2 = 4;
    piVar3 = &DAT_00719448;
  }
  else {
    if (in_EAX != &DAT_006b42d8) {
      return 0;
    }
    iVar2 = 8;
    piVar3 = &DAT_0071944c;
  }
  if (*piVar3 < iVar2) {
    puVar4 = in_EAX + *piVar3 * 0x88;
    for (iVar2 = 0x88; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *param_1;
      param_1 = param_1 + 1;
      puVar4 = puVar4 + 1;
    }
    *piVar3 = *piVar3 + 1;
    uVar1 = 1;
  }
  return uVar1;
}
#endif
