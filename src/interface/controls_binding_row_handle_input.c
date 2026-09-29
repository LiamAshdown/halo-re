// controls_binding_row_handle_input  (Ghidra: FUN_004b4f30, named in phase 4)
// address 0x4b4f30, size 943 bytes (jump table 0x4b52e0 on the focus result + 1)
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4b4f30..0x4b52de in the phase-4 review (the first rewrite
// dropped every register argument: heap_reallocate, the action names in EDI/EBX, the device
// and action of 0x4b4df0/0x4b4e20, the binding records of 0x53ad00/0x53ae10/0x48be50). The
// per-frame update of the controls setup screen (argument: the screen widget):
//   dims the two headers and the footer while a binding is being captured (0x006953e8 is the
//   row being captured, -1 for none), shows the device label name of the device spinner
//   (device = spinner selection, +1 when not 0, kept at 0x006953ec), refreshes the eight rows
//   of the current page (controls_binding_list_refresh_rows), points the extended
//   description at the focused part (0 or 1 header, 2 row, 3 row while capturing);
//   while capturing and the input capture (0x00712542 bit 3, not state 1) has recorded a
//   control at 0x007127c4 ({int16 kind 1 keyboard / 2 mouse / 3 gamepad, int16 index, int16
//   subtype, int16 control}): keyboard control 0 cancels (sound 3), key 0x1d clears the
//   action (keyboard, then mouse too for device 0; sound 2); a keyboard key while a gamepad
//   device is shown, a mouse control other than a button (subtype 0) or axis 2, or a gamepad
//   other than the shown one are ignored; a reserved key sounds 4. Otherwise the action
//   (page * 8 + row) is looked up (input_action_name_to_index, 0x7fff sounds 4), refused when
//   its column is unbindable (0x4b4df0, sound 4), the old binding of the device and the
//   recorded control's old action are cleared (except a gamepad action with column bit 2),
//   the control is bound (0x53ae10, ESI record, stack action) and announced (0x48be50), sound
//   2. Any finished capture clears the three capture highlights (+0x54) of the row, input
//   capture bit 3 and the 0xa0 dword buffer at 0x00712544, and ends the capture; every
//   handled or ignored control is removed from 0x007127c4.
// register convention: plain cdecl, one stack argument; returns 1 in AL.

#include <string.h>
#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern heap *widget_memory_pool;                          // 0x006926c4
extern int32_t controls_capture_row; // 0x006953e8, -1 when no binding is being captured
extern int32_t controls_selected_device;                  // 0x006953ec
extern controls_device_label controls_device_labels[0x10]; // 0x006932e8
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE name
extern int16_t controls_captured_binding[6];              // 0x007127c4..0x007127cf, UNSURE name
extern uint32_t controls_input_capture_buffer[0xa0];      // 0x00712544, UNSURE name
extern uint8_t controls_action_table[][0x18];             // 0x00692fe8

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern int32_t controls_binding_list_refresh_rows(widget_instance *widget, int32_t page); // 0x4b4790, blam-cc: EAX widget
extern uint8_t controls_key_is_bindable(int32_t control); // 0x4b43c0, blam-cc: EDX control (returns 0 for a reserved one)
extern uint8_t controls_enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name,
                                                          uint8_t accept_reserved_on_retry); // 0x4b43e0, blam-cc: EAX device, ECX record, EDI action_name
extern uint8_t controls_binding_clear(int32_t action_index, int32_t device); // 0x4b4e20, blam-cc: EAX action_index
extern uint8_t controls_action_column_is_bindable(int32_t slot, int32_t action_index); // 0x4b4df0, blam-cc: ECX slot, EDX action_index
extern int16_t input_action_name_to_index(const char *action_name); // 0x48fe60, blam-cc: EBX action_name
extern void control_profile_clear_binding(const int16_t *record); // 0x53ad00, blam-cc: ESI record
extern void control_profile_set_binding(const int16_t *record, int32_t action); // 0x53ae10, blam-cc: ESI record
extern void input_last_used_binding_copy(const int16_t *record, int32_t action); // 0x48be50, blam-cc: EAX record, ECX action

static void controls_set_dimmed(widget_instance *widget, uint8_t dimmed)
{
    widget->hidden = dimmed;
    widget->scale = dimmed ? 0.333f : 1.0f; // 0x3eaa7efa
}

uint8_t controls_binding_row_handle_input(widget_instance *screen)
{
    static const uint16_t empty_text[1] = {0}; // 0x00660c34
    widget_instance *header = screen->first_child;
    widget_instance *spinner;
    widget_instance *footer;
    int32_t device;
    int32_t page;
    int32_t focus;
    int32_t part;
    int32_t i;

    controls_set_dimmed(header, controls_capture_row != -1);
    spinner = header->first_child->next_sibling;
    device = spinner->selection_index;
    if (device >= 1) {
        device++;
    }
    controls_selected_device = device;
    spinner->list_render_data = heap_reallocate(spinner->list_render_data, 0x80, widget_memory_pool);
    if (spinner->list_render_data != 0) {
        int32_t label = spinner->selection_index;
        wcsncpy((wchar_t *)spinner->list_render_data,
                (const wchar_t *)(label >= 0 && label < 0x10 ? controls_device_labels[label].name : empty_text), 0x3f);
        ((uint16_t *)spinner->list_render_data)[0x3f] = 0;
    }

    header = header->next_sibling;
    controls_set_dimmed(header, controls_capture_row != -1);
    page = header->first_child->next_sibling->selection_index;
    focus = controls_binding_list_refresh_rows(screen, page);
    part = -1;
    switch (focus + 1) {
    case 1: case 2: part = focus; break;                              // a header
    case 3: part = (controls_capture_row != -1) + 2; break;           // a row
    }
    if (part != -1) {
        screen->extended_description->selection_index = (int16_t)part;
        screen->extended_description->state = 1;
    } else {
        screen->extended_description->state = 0;
    }
    footer = header->next_sibling;
    for (i = 0; i < 8; i++) {
        footer = footer->next_sibling;
    }
    controls_set_dimmed(footer, controls_capture_row != -1);
    controls_set_dimmed(footer->first_child->next_sibling, controls_selected_device < 2);

    if (controls_capture_row == -1 || controls_input_capture_flags == 1 || (controls_input_capture_flags & 8) == 0 ||
        controls_captured_binding[0] == 0) {
        return 1;
    }

    {
        int16_t record[6];
        int32_t action_index = controls_capture_row + page * 8;
        int16_t kind;
        int16_t sound;

        memcpy(record, controls_captured_binding, sizeof(record));
        kind = record[0];
        if (kind == 1) { // keyboard
            if (record[3] == 0) {
                sound = 3; // cancel
                goto finish_capture;
            }
            if (record[3] == 0x1d) {
                controls_binding_clear(action_index, device);
                if (device == 0) {
                    controls_binding_clear(action_index, 1);
                }
                sound = 2;
                goto finish_capture;
            }
            if (device != 0) {
                goto drop_control;
            }
            if (controls_key_is_bindable(record[3]) == 0) {
                widget_play_sound_effect(4);
                goto drop_control;
            }
        } else if (kind == 2) { // mouse
            if (device != 0) {
                goto drop_control;
            }
            if (record[2] != 0 && !(record[2] == 1 && record[3] == 2)) {
                goto drop_control;
            }
        } else if (kind == 3) { // gamepad
            if (record[1] != device - 2) {
                goto drop_control;
            }
        } else {
            goto drop_control;
        }

        {
            const char *action_name = (const char *)controls_action_table[action_index];
            int16_t action = input_action_name_to_index(action_name);

            if (action == 0x7fff || controls_action_column_is_bindable(kind == 2 ? 1 : device, action_index) == 0) {
                sound = 4;
                goto finish_capture;
            }
            if (kind != 3 || (controls_action_table[action_index][0x14] & 4) == 0) {
                int16_t previous[6];
                if (controls_enumerate_next_assignable_action(kind == 2 ? 1 : device, previous, action_name, 0) != 0) {
                    control_profile_clear_binding(previous);
                }
                control_profile_clear_binding(record);
            }
            control_profile_set_binding(record, action);
            input_last_used_binding_copy(record, action);
            sound = 2;
        }

finish_capture:
        widget_play_sound_effect(sound);
        {
            widget_instance *row = screen->first_child->next_sibling->next_sibling;
            widget_instance *cell;

            for (i = controls_capture_row; i != 0; i--) {
                row = row->next_sibling;
            }
            cell = row->first_child->next_sibling;
            *(uint8_t *)&cell->selection_direction = 0;
            cell = cell->next_sibling->first_child;
            *(uint8_t *)&cell->selection_direction = 0;
            *(uint8_t *)&cell->next_sibling->selection_direction = 0;
        }
        controls_input_capture_flags &= 0xf7;
        memset(controls_input_capture_buffer, 0, sizeof(controls_input_capture_buffer));
        controls_capture_row = -1;
drop_control:
        memset(controls_captured_binding, 0, sizeof(controls_captured_binding));
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b4f30):


undefined4 FUN_004b4f30(int param_1)

{
  int iVar1;
  short sVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  wchar_t *_Dest;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  wchar_t *_Source;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  short sStack_16;
  
  iVar9 = *(int *)(param_1 + 0x34);
  if (DAT_006953e8 == -1) {
    *(undefined1 *)(iVar9 + 0x12) = 0;
    *(undefined4 *)(iVar9 + 0x24) = 0x3f800000;
  }
  else {
    *(undefined1 *)(iVar9 + 0x12) = 1;
    *(undefined4 *)(iVar9 + 0x24) = 0x3eaa7efa;
  }
  iVar1 = *(int *)(*(int *)(iVar9 + 0x34) + 0x2c);
  iVar5 = (int)*(short *)(iVar1 + 0x40);
  if (0 < iVar5) {
    iVar5 = iVar5 + 1;
  }
  DAT_006953ec = iVar5;
  _Dest = (wchar_t *)heap_reallocate(0x80);
  *(wchar_t **)(iVar1 + 0x50) = _Dest;
  if (_Dest != (wchar_t *)0x0) {
    iVar8 = (int)*(short *)(iVar1 + 0x40);
    if ((iVar8 < 0) || (0xf < iVar8)) {
      _Source = L"";
    }
    else {
      _Source = (wchar_t *)(&DAT_006932e8 + iVar8 * 0x84);
    }
    _wcsncpy(_Dest,_Source,0x3f);
    *(undefined2 *)(*(int *)(iVar1 + 0x50) + 0x7e) = 0;
  }
  iVar9 = *(int *)(iVar9 + 0x2c);
  if (DAT_006953e8 == -1) {
    *(undefined1 *)(iVar9 + 0x12) = 0;
    *(undefined4 *)(iVar9 + 0x24) = 0x3f800000;
  }
  else {
    *(undefined1 *)(iVar9 + 0x12) = 1;
    *(undefined4 *)(iVar9 + 0x24) = 0x3eaa7efa;
  }
  iVar10 = (int)*(short *)(*(int *)(*(int *)(iVar9 + 0x34) + 0x2c) + 0x40);
  iVar6 = FUN_004b4790(iVar10);
  iVar1 = DAT_006953e8;
  iVar8 = -1;
  switch(iVar6) {
  case 0:
  case 1:
    iVar8 = iVar6;
    break;
  case 2:
    iVar8 = (DAT_006953e8 != -1) + 2;
    break;
  case -1:
    iVar8 = -1;
  }
  iVar6 = *(int *)(param_1 + 0x4c);
  if (iVar8 == -1) {
    *(undefined1 *)(iVar6 + 0x10) = 0;
  }
  else {
    *(short *)(iVar6 + 0x40) = (short)iVar8;
    *(undefined1 *)(iVar6 + 0x10) = 1;
  }
  iVar9 = *(int *)(iVar9 + 0x2c);
  iVar8 = 8;
  do {
    iVar8 = iVar8 + -1;
    iVar9 = *(int *)(iVar9 + 0x2c);
  } while (iVar8 != 0);
  if (iVar1 == -1) {
    *(undefined1 *)(iVar9 + 0x12) = 0;
    *(undefined4 *)(iVar9 + 0x24) = 0x3f800000;
  }
  else {
    *(undefined1 *)(iVar9 + 0x12) = 1;
    *(undefined4 *)(iVar9 + 0x24) = 0x3eaa7efa;
  }
  iVar9 = *(int *)(*(int *)(iVar9 + 0x34) + 0x2c);
  if (DAT_006953ec < 2) {
    *(undefined1 *)(iVar9 + 0x12) = 1;
    *(undefined4 *)(iVar9 + 0x24) = 0x3eaa7efa;
  }
  else {
    *(undefined1 *)(iVar9 + 0x12) = 0;
    *(undefined4 *)(iVar9 + 0x24) = 0x3f800000;
  }
  if (iVar1 == -1) {
    return 1;
  }
  if (DAT_00712542 == 1) {
    return 1;
  }
  if ((DAT_00712542 & 8) == 0) {
    return 1;
  }
  sVar2 = (short)DAT_007127c4;
  if ((short)DAT_007127c4 == 0) {
    return 1;
  }
  sVar4 = (short)((uint)DAT_007127c8 >> 0x10);
  if ((short)DAT_007127c4 == 1) {
    if (sVar4 == 0) goto LAB_004b5266;
    if (sVar4 == 0x1d) {
      FUN_004b4e20(iVar5);
      if (iVar5 == 0) {
        FUN_004b4e20(1);
      }
      goto LAB_004b5266;
    }
    if (iVar5 != 0) {
      DAT_007127c4 = 0;
      DAT_007127c8 = 0;
      DAT_007127cc = 0;
      return 1;
    }
    cVar3 = FUN_004b43c0();
    if (cVar3 == '\0') {
      widget_play_sound_effect();
      DAT_007127c4 = 0;
      DAT_007127c8 = 0;
      DAT_007127cc = 0;
      return 1;
    }
  }
  else if ((short)DAT_007127c4 == 2) {
    if (iVar5 != 0) {
      DAT_007127c4 = 0;
      DAT_007127c8 = 0;
      DAT_007127cc = 0;
      return 1;
    }
    if ((short)DAT_007127c8 != 0) {
      if ((short)DAT_007127c8 != 1) {
        DAT_007127c4 = 0;
        DAT_007127c8 = 0;
        DAT_007127cc = 0;
        return 1;
      }
      if (sVar4 != 2) {
        DAT_007127c4 = 0;
        DAT_007127c8 = 0;
        DAT_007127cc = 0;
        return 1;
      }
    }
  }
  else {
    if ((short)DAT_007127c4 != 3) {
      DAT_007127c4 = 0;
      DAT_007127c8 = 0;
      DAT_007127cc = 0;
      return 1;
    }
    sStack_16 = (short)((uint)DAT_007127c4 >> 0x10);
    if ((int)sStack_16 != iVar5 + -2) {
      DAT_007127c4 = 0;
      DAT_007127c8 = 0;
      DAT_007127cc = 0;
      return 1;
    }
  }
  uVar7 = input_action_name_to_index();
  if (((short)uVar7 != 0x7fff) && (cVar3 = FUN_004b4df0(), cVar3 != '\0')) {
    if ((sVar2 != 3) || (((&DAT_00692ffc)[(iVar1 + iVar10 * 8) * 0x18] & 4) == 0)) {
      cVar3 = FUN_004b43e0(0);
      if (cVar3 != '\0') {
        control_profile_clear_binding();
      }
      control_profile_clear_binding();
    }
    control_profile_set_binding(uVar7);
    FUN_0048be50();
  }
LAB_004b5266:
  widget_play_sound_effect();
  iVar9 = *(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x2c) + 0x2c);
  for (; DAT_006953e8 != 0; DAT_006953e8 = DAT_006953e8 + -1) {
    iVar9 = *(int *)(iVar9 + 0x2c);
  }
  iVar9 = *(int *)(*(int *)(iVar9 + 0x34) + 0x2c);
  *(undefined1 *)(iVar9 + 0x54) = 0;
  iVar9 = *(int *)(*(int *)(iVar9 + 0x2c) + 0x34);
  *(undefined1 *)(iVar9 + 0x54) = 0;
  *(undefined1 *)(*(int *)(iVar9 + 0x2c) + 0x54) = 0;
  DAT_00712542 = DAT_00712542 & 0xf7;
  puVar11 = &DAT_00712544;
  for (iVar9 = 0xa0; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  DAT_007127cc = 0;
  DAT_007127c8 = 0;
  DAT_007127c4 = 0;
  DAT_006953e8 = 0xffffffff;
  return 1;
}
#endif
