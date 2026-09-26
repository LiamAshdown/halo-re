// controls_gamepad_bindings_restore  (Ghidra: FUN_004b5a70, named in phase 4)
// address 0x4b5a70, size 170 bytes (0x2000 byte frame through _chkstk 0x628240)
// name confidence: 0.4   rewrite confidence: 0.75
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: rewritten from objdump 0x4b5a70..0x4b5b19 in the phase-4 review. The first
// rewrite dropped every register argument of the five saved-games calls.
//   Without a loaded profile (selected_saved_item low nibble set) returns 0. Otherwise the
// working profile at 0x00714e80 (0x1ffc bytes) is copied to the stack, 0x53b5a0 runs on the
// working profile (ECX), its four slots are reset (control_profile_reset_slot, ESI profile,
// EDX slot), and for every controls_assigned_gamepads entry that 0x53b470 accepts (both cdecl stack
// arguments: entry, profile) 0x53b700 merges the saved copy back (EAX entry; stack profile,
// saved copy). 0x53b620 (EAX profile) finishes; returns 1.
// register convention: plain cdecl, no arguments; returns AL.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t selected_saved_item;                   // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80
extern int32_t controls_assigned_gamepad_count;                  // 0x00719448
extern controls_gamepad_record controls_assigned_gamepads[4];        // 0x006b53d8

extern void control_profile_clear_device_slot_mappings(uint8_t *profile); // 0x53b5a0, blam-cc: ECX profile
extern void control_profile_reset_slot(uint8_t *profile, int32_t slot); // 0x53b2b0, blam-cc: ESI profile, EDX slot
extern uint8_t control_profile_find_or_create_gamepad_slot(const controls_gamepad_record *entry, uint8_t *profile); // 0x53b470, cdecl
extern uint8_t control_profile_copy_gamepad_bindings_by_key(const controls_gamepad_record *entry, uint8_t *profile, const uint8_t *saved_profile); // 0x53b700, blam-cc: EAX entry
extern void control_profile_reestablish_device_slot_mappings(uint8_t *profile_record); // 0x53b620, blam-cc: EAX profile_record

uint8_t controls_gamepad_bindings_restore(void)
{
    uint8_t saved_profile[0x1ffc];
    int32_t i;

    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    memcpy(saved_profile, saved_item_working_copy, sizeof(saved_profile));
    control_profile_clear_device_slot_mappings(saved_item_working_copy);
    for (i = 0; i < 4; i++) {
        control_profile_reset_slot(saved_item_working_copy, i);
    }
    for (i = 0; i < controls_assigned_gamepad_count; i++) {
        if (control_profile_find_or_create_gamepad_slot(&controls_assigned_gamepads[i], saved_item_working_copy) != 0) {
            control_profile_copy_gamepad_bindings_by_key(&controls_assigned_gamepads[i], saved_item_working_copy, saved_profile);
        }
    }
    control_profile_reestablish_device_slot_mappings(saved_item_working_copy);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b5a70):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_004b5a70(void)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x4b5a80;
  uVar2 = 0;
  if ((DAT_00714e7c & 0xf) == 0) {
    puVar4 = &DAT_00714e80;
    puVar5 = local_2008;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    FUN_0053b5a0();
    iVar3 = 0;
    do {
      control_profile_reset_slot();
      iVar3 = iVar3 + 1;
    } while (iVar3 < 4);
    iVar3 = 0;
    if (0 < DAT_00719448) {
      puVar4 = &DAT_006b53d8;
      do {
        cVar1 = FUN_0053b470(puVar4,&DAT_00714e80);
        if (cVar1 != '\0') {
          FUN_0053b700(&DAT_00714e80,local_2008);
        }
        iVar3 = iVar3 + 1;
        puVar4 = puVar4 + 0x88;
      } while (iVar3 < DAT_00719448);
    }
    FUN_0053b620();
    uVar2 = 1;
  }
  return uVar2;
}
#endif
