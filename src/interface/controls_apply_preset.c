// controls_apply_preset  (Ghidra: FUN_004b4c50, named in phase 4)
// address 0x4b4c50, size 414 bytes (0x1ffc byte stack frame through _chkstk 0x628240)
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4b4c50..0x4b4ded in the phase-4 review. The first rewrite
// played sound 0 (it is 2, AX), copied the default profile from the wrong offsets and dropped
// the EDI profile of 0x53b500. In the list mode (0x00719445) the two preset spinners (children
// of type 2 below the menu) get selection 2. Otherwise, for a real profile (selected saved
// item low nibble 0), the preset spinner selection picks: 0 the default input profile (the
// device_defaults tag found by GUID 0x0065b8e0, input_device_default_profile_tag_find; its
// digital bindings (+0x134, 0xda bytes) and analog block (+0x20e, 7 dwords) are copied into
// the working profile at 0x00714fb4 / 0x0071508e, or the bindings are reset without one), 1
// and 2..6 preset 0..4 through 0x53b500 (EDI profile 0x00714e80, stack preset). The sound is
// widget_play_sound_effect(2) on every path.
// register convention: plain cdecl, one stack argument.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t controls_menu_list_mode;          // 0x00719445, UNSURE name
extern int32_t selected_saved_item;              // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80
extern uint8_t control_keyboard_scan_table[0xda]; // 0x00714fb4 (working profile +0x134)
extern uint32_t control_mouse_button_scan_table[7]; // 0x0071508e (working profile +0x20e)
extern uint32_t input_default_profile_guid[4];   // 0x0065b8e0

extern int32_t input_device_default_profile_tag_find(input_guid guid, uint8_t *out_profile); // 0x490110, the GUID passed by value
extern void control_profile_reset_digital_bindings(uint8_t *profile); // 0x539ff0, blam-cc: EDX profile
extern void control_profile_reset_analog_bindings(uint8_t *profile);  // 0x53a0d0, blam-cc: ECX profile
extern uint8_t control_profile_finalize_slot(uint8_t *profile, int32_t preset_index); // 0x53b500, applies a controls preset, blam-cc: EDI profile
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

static widget_instance *controls_find_child_of_type(widget_instance *child, int16_t type)
{
    while (child != 0 && child->widget_type != type) {
        child = child->next_sibling;
    }
    return child;
}

uint8_t controls_apply_preset(widget_instance *widget)
{
    uint8_t result = 0;

    if (controls_menu_list_mode != 0) {
        widget_instance *menu = widget->parent->parent->first_child;
        controls_find_child_of_type(menu->first_child, 2)->selection_index = 2;
        controls_find_child_of_type(menu->next_sibling->first_child, 2)->selection_index = 2;
        result = 1;
    } else if ((selected_saved_item & 0xf) == 0) {
        int32_t selection = widget->parent->parent->first_child->first_child->next_sibling->selection_index;

        if (selection == 1 || selection >= 2) {
            if (selection == 1) {
                selection = 2;
            }
            selection -= 2;
            if (selection >= 0 && selection <= 4) {
                result = control_profile_finalize_slot(saved_item_working_copy, selection);
                widget_play_sound_effect(2);
                return result;
            }
        } else {
            uint8_t profile[0x1ffc - 0x10]; // the rest of the 0x1ffc byte frame
            input_guid guid;

            memcpy(&guid, input_default_profile_guid, sizeof(guid));
            if (input_device_default_profile_tag_find(guid, profile) != -1) {
                memcpy(control_keyboard_scan_table, profile + 0x134, 0xda);
                memcpy(control_mouse_button_scan_table, profile + 0x20e, sizeof(control_mouse_button_scan_table));
            } else {
                control_profile_reset_digital_bindings(saved_item_working_copy);
                control_profile_reset_analog_bindings(saved_item_working_copy);
            }
            result = 1;
        }
    }
    widget_play_sound_effect(2);
    return result;
}

#if 0
Original Ghidra decompilation (0x4b4c50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 FUN_004b4c50(int param_1)

{
  int iVar1;
  undefined1 uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined1 local_1ffc [308];
  undefined4 local_1ec8 [54];
  undefined4 local_1dee;
  undefined4 local_1dea;
  undefined4 local_1de6;
  undefined4 local_1de2;
  undefined4 local_1dde;
  undefined4 local_1dda;
  undefined4 local_1dd6;
  undefined4 uStack_4;

  uStack_4 = 0x4b4c5a;
  uVar2 = 0;
  if (DAT_00719445 == '\0') {
    if ((DAT_00714e7c & 0xf) != 0) goto LAB_004b4dda;
    iVar3 = (int)*(short *)(*(int *)(*(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x30) + 0x30) +
                                                      0x34) + 0x34) + 0x2c) + 0x40);
    if (iVar3 == 1) {
      iVar3 = 2;
    }
    else if (iVar3 < 2) {
      iVar3 = input_device_default_profile_tag_find
                        (0x23a8e6bc,0x4ef8ce3b,0x6b656b81,0x73d56786,local_1ffc);
      if (iVar3 == -1) {
        control_profile_reset_digital_bindings();
        control_profile_reset_analog_bindings();
      }
      else {
        puVar4 = local_1ec8;
        puVar5 = &DAT_00714fb4;
        for (iVar3 = 0x36; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar5 = *puVar4;
          puVar4 = puVar4 + 1;
          puVar5 = puVar5 + 1;
        }
        *(undefined2 *)puVar5 = *(undefined2 *)puVar4;
        _DAT_0071508e = local_1dee;
        _DAT_00715092 = local_1dea;
        _DAT_00715096 = local_1de6;
        _DAT_0071509a = local_1de2;
        _DAT_0071509e = local_1dde;
        _DAT_007150a2 = local_1dda;
        _DAT_007150a6 = local_1dd6;
      }
      goto LAB_004b4dd8;
    }
    iVar3 = iVar3 + -2;
    if ((-1 < iVar3) && (iVar3 < 5)) {
      uVar2 = FUN_0053b500(iVar3);
      widget_play_sound_effect();
      return uVar2;
    }
  }
  else {
    iVar3 = *(int *)(*(int *)(*(int *)(param_1 + 0x30) + 0x30) + 0x34);
    for (iVar1 = *(int *)(iVar3 + 0x34); (iVar1 != 0 && (*(short *)(iVar1 + 0xe) != 2));
        iVar1 = *(int *)(iVar1 + 0x2c)) {
    }
    *(undefined2 *)(iVar1 + 0x40) = 2;
    for (iVar3 = *(int *)(*(int *)(iVar3 + 0x2c) + 0x34);
        (iVar3 != 0 && (*(short *)(iVar3 + 0xe) != 2)); iVar3 = *(int *)(iVar3 + 0x2c)) {
    }
    *(undefined2 *)(iVar3 + 0x40) = 2;
LAB_004b4dd8:
    uVar2 = 1;
  }
LAB_004b4dda:
  widget_play_sound_effect();
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
