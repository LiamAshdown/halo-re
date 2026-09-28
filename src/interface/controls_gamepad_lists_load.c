// controls_gamepad_lists_load  (Ghidra: FUN_004b58d0, named in phase 4)
// address 0x4b58d0, size 405 bytes
// name confidence: 0.35   rewrite confidence: 0.8
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: rewritten from objdump 0x4b58d0..0x4b5a64 in the phase-4 review. Renamed from
// server_list_reset, which collided with the networking module name of 0x4b65f0
// (symbols/functions.txt); symbols/review_queue.txt had network_game_list_load. The first
// rewrite added the built-in entries to the wrong list with the wrong stride (0x90 for
// 0x240), removed the saved entries from the wrong list, dropped the ECX screen widget of
// 0x4b5560 / 0x4b55d0 and never set the focused child.
//   Zeroes both lists and counts; without a loaded profile (selected_saved_item low nibble
// set) returns 0. Otherwise every connected gamepad in the table at 0x006b1868 (stride 0x240, 0x220
// bytes copied; count int16 at 0x006b1844, named input_gamepad_count in
// FUN_004a62d0.c) is appended to controls_available_gamepads (0x4b5800), then each of the four saved
// entries of the profile (+0x1108, stride 0x220, a nonzero first word marks a used slot) is
// appended to controls_assigned_gamepads while it has room and removed from controls_available_gamepads (0x4b5850,
// matched on the 0x14 byte key). Refreshes the widgets and focuses the assigned list (node 0),
// else the available list (node 5), else node 15. Returns 1.
// Kept from the binary: the loop over the built-in table compares the index with the whole
// dword at 0x006b1844 (the bound is the int16), and an index at or past that dword reuses the
// previous copy only when the stack flag byte is already set; that byte is never initialised
// before the first iteration (modelled as 0 here).
// register convention: plain cdecl, one stack argument (the screen widget); returns AL.
// reconciled: R78 0x006b1844 input_gamepad_count(_dword) -> input.h int32_t input_device_count; the WORD readers keep their int16 width through an (int16_t) cast

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t selected_saved_item;                // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80
extern controls_gamepad_record controls_available_gamepads[8];   // 0x006b42d8
extern controls_gamepad_record controls_assigned_gamepads[4];     // 0x006b53d8
extern int32_t controls_assigned_gamepad_count;               // 0x00719448
extern int32_t controls_available_gamepad_count;             // 0x0071944c
extern int32_t input_device_count; // 0x006b1844, input.h (0..8 connected input devices)
extern uint8_t input_devices[]; // 0x006b1868, stride 0x240; UNSURE name

extern void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen); // 0x4b5560, blam-cc: EAX out, ECX screen
extern void controls_gamepad_lists_refresh(widget_instance *screen); // 0x4b55d0, blam-cc: ECX screen
extern uint8_t controls_gamepad_list_add(const controls_gamepad_record *entry, controls_gamepad_record *list); // 0x4b5800, blam-cc: EAX list
extern uint8_t controls_gamepad_list_remove(const controls_gamepad_record *entry, controls_gamepad_record *list); // 0x4b5850, blam-cc: EDI list

uint8_t controls_gamepad_lists_load(widget_instance *screen)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : (uint8_t *)0;
    widget_instance *nodes[17];
    controls_gamepad_record entry;
    uint8_t have_entry = 0; // UNSURE: uninitialised stack byte in the binary
    int32_t count;
    int32_t i;

    memset(controls_assigned_gamepads, 0, sizeof(controls_assigned_gamepads));
    memset(controls_available_gamepads, 0, sizeof(controls_available_gamepads));
    controls_assigned_gamepad_count = 0;
    controls_available_gamepad_count = 0;
    if (profile == 0) {
        return 0;
    }

    controls_gamepad_widget_nodes_collect(nodes, screen);
    count = (int16_t)input_device_count;
    for (i = 0; i < count; i++) {
        if ((int16_t)i < input_device_count) {
            memcpy(&entry, input_devices + (int16_t)i * 0x240, sizeof(entry));
            have_entry = 1;
        } else if (!have_entry) {
            continue;
        }
        controls_gamepad_list_add(&entry, controls_available_gamepads);
    }

    for (i = 0; i < 4; i++) {
        const controls_gamepad_record *saved = (const controls_gamepad_record *)(profile + 0x1108) + i;

        if (*(const uint16_t *)saved == 0) {
            continue;
        }
        entry = *saved;
        if (controls_assigned_gamepad_count < 4) {
            controls_assigned_gamepads[controls_assigned_gamepad_count] = *saved;
            controls_assigned_gamepad_count++;
        }
        controls_gamepad_list_remove(&entry, controls_available_gamepads);
    }

    controls_gamepad_lists_refresh(screen);
    if (controls_assigned_gamepad_count > 0) {
        screen->focused_child = nodes[0];
    } else if (controls_available_gamepad_count > 0) {
        screen->focused_child = nodes[5];
    } else {
        screen->focused_child = nodes[15];
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b58d0):

undefined4 FUN_004b58d0(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short *psVar5;
  undefined4 *puVar6;
  short *psVar7;
  undefined4 *puVar8;
  char local_279;
  int local_278;
  undefined4 local_270;
  undefined4 local_25c;
  undefined4 local_234;
  undefined4 local_228 [137];

  uVar1 = ~-(uint)((DAT_00714e7c & 0xf) != 0) & 0x714e80;
  puVar6 = &DAT_006b53d8;
  for (iVar2 = 0x220; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  iVar4 = 0;
  puVar6 = &DAT_006b42d8;
  for (iVar2 = 0x440; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  DAT_00719448 = 0;
  DAT_0071944c = 0;
  if (uVar1 == 0) {
    return 0;
  }
  FUN_004b5560();
  iVar2 = (int)(short)DAT_006b1844;
  if (0 < iVar2) {
    do {
      if ((short)iVar4 < DAT_006b1844) {
        puVar6 = &DAT_006b1868 + (short)iVar4 * 0x90;
        puVar8 = local_228;
        for (iVar3 = 0x88; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar8 = *puVar6;
          puVar6 = puVar6 + 1;
          puVar8 = puVar8 + 1;
        }
        local_279 = '\x01';
LAB_004b5977:
        FUN_004b5800(local_228);
      }
      else if (local_279 != '\0') goto LAB_004b5977;
      iVar4 = iVar4 + 1;
    } while (iVar4 < iVar2);
  }
  local_278 = 0;
  psVar5 = (short *)(uVar1 + 0x1108);
  do {
    if (((-1 < local_278) && (local_278 < 4)) && (*psVar5 != 0)) {
      psVar7 = psVar5;
      puVar6 = local_228;
      for (iVar2 = 0x88; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar6 = *(undefined4 *)psVar7;
        psVar7 = psVar7 + 2;
        puVar6 = puVar6 + 1;
      }
      if (DAT_00719448 < 4) {
        iVar2 = DAT_00719448 * 0x88;
        DAT_00719448 = DAT_00719448 + 1;
        psVar7 = psVar5;
        puVar6 = &DAT_006b53d8 + iVar2;
        for (iVar4 = 0x88; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar6 = *(undefined4 *)psVar7;
          psVar7 = psVar7 + 2;
          puVar6 = puVar6 + 1;
        }
      }
      FUN_004b5850(local_228);
    }
    local_278 = local_278 + 1;
    psVar5 = psVar5 + 0x110;
  } while (local_278 < 4);
  FUN_004b55d0();
  if (DAT_00719448 < 1) {
    if (DAT_0071944c < 1) {
      *(undefined4 *)(param_1 + 0x38) = local_234;
      return 1;
    }
    *(undefined4 *)(param_1 + 0x38) = local_25c;
    return 1;
  }
  *(undefined4 *)(param_1 + 0x38) = local_270;
  return 1;
}
#endif
