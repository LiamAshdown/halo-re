// widget_instance_handle_input_event  (Ghidra: widget_instance_handle_input_event, already named)
// address 0x499d00, size 1504 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: matches the given name; Ghidra fully resolved all four parameters (widget, tag data,
// event record, out-handled flag), matching the call sites already rewritten in interface_tick.c.
// Its own child-dispatch tail (the final `if` block below) is byte-for-byte the same logic
// separately catalogued as "render_ui_cursor" @0x49a2e0 (out/phase4/interface_types_notes.md:
// "draws nothing; it is the child-recursion tail of widget_instance_handle_input_event") --
// that address is a compiler-shared duplicate of this same tail and is not written as its own
// file; see this session's summary.
// register convention: cdecl, all four recognized stack parameters.
// TYPES-GAP: the input-event record (Ghidra's `short *param_3`) is not documented anywhere in
// types/interface.h; declared locally as a raw 8-element int16 array and accessed by the same
// indices Ghidra shows, with known meanings noted inline rather than given field names this
// session could not fully confirm (kind at [0]: 1 dpad-threshold, 2 analog-stick, 3 button-code,
// 4 mouse-click, 5 custom-activation(0x20); target controller at [1]; a sub-code at [2] whose low
// byte is compared to event handler codes and whose high byte -- read separately as a `char` at
// byte offset 5 -- is the "this is a press, not a release" flag; a signed axis extreme at [3]).
// UNSURE: widget_spinner_list_sync_selected/0049c040/0049c080/0049c0f0 (0x49c000, 0x49c040, 0x49c080, 0x49c0f0) are
// the four focused_child-moving helpers types/interface.h's widget_history_node note mentions
// ("0x49c000/0x49c040/0x49c080/0x49c0f0 move it around the sibling ring") but does not name;
// declared void(void), called with no visible arguments by Ghidra at every site.
// reconciled: R02 0x006b2ce8 team_slot_table -> input.h joystick_slot_devices[4]

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t ui_time_milliseconds; // 0x00718f9c
extern widget_instance *ui_root_widget[1]; // 0x00718f94
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, per interface_handle_quit_request.c
extern uint8_t network_join_error_reason; // 0x0071973c, byte stores only
extern uint8_t split_screen_quit_prompt_armed; // 0x00719757
extern int32_t joystick_slot_devices[4]; // 0x006b2ce8, input.h (slot -> device, -1 none); DWORD reads 0x499d5d, 0x499d83

extern void widget_close(widget_instance *widget); // 0x497c00


extern void ui_widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag,
                                          int16_t *event, void *handler, uint8_t *out_handled); // 0x49a430


extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90
extern void widget_instance_handle_input_event(widget_instance *widget, UIWidgetDefinition *tag,
                                                int16_t *event, uint8_t *out_handled); // self

// Processes an input event against a widget instance's bound UI events: first, if the widget has
// a per-controller close request pending, closes the topmost ancestor and reports handled;
// otherwise applies deferred auto-close/fade timing, runs spinner_list/column_list per-frame
// upkeep, tab/dpad-driven focus and list-selection navigation when the tag's flags allow it, then
// scans the tag's own event_handlers for one matching this event and, on a match, activates it
// via ui_widget_list_item_activate. Finally, unless a higher-priority ancestor gated it, forwards
// the event either to the single focused child or (broadcast flag set) to every child in turn,
// arming the split-screen "confirm quit" prompt if the last widget just closed with the
// return_to_main_menu_if_no_history flag set. Always plays the selected UI sound effect on exit
// and writes the final handled flag to `*out_handled`.
void widget_instance_handle_input_event(widget_instance *widget, UIWidgetDefinition *tag,
                                         int16_t *event, uint8_t *out_handled)
{
    uint8_t handled = 0;        // Ghidra's local_b
    uint8_t list_nav_done = 0;  // Ghidra's local_a
    uint8_t controller_matches; // Ghidra's local_9
    int16_t sound_effect = 0;   // Ghidra's iVar8/local_8 (shared use as both a loop counter and a sound-effect id)
    int32_t handler_scan_count = 0;

    controller_matches = (widget->hidden == 0 &&
                           (widget->controller_index == -1 || widget->controller_index == event[1]));

    if (widget->close_when_controller_connected[0] == 1) {
        int16_t controller = widget->controller_index;
        widget_instance *ancestor = widget;

        if (controller < 0 || controller > 3) {
            int32_t i;

            // objdump 0x499d7b..0x499d92: close as soon as any slot is not -1; all -1 skips
            for (i = 0; i <= 3; i++) {
                if (joystick_slot_devices[i] != -1) {
                    break;
                }
            }
            if (i > 3) {
                goto after_close_check;
            }
            ancestor = widget;
            while (ancestor->parent != (widget_instance *)0) {
                ancestor = ancestor->parent;
            }
            widget_close(ancestor);
            handled = 1;
        } else if (joystick_slot_devices[controller] != -1) {
            ancestor = widget;
            while (ancestor->parent != (widget_instance *)0) {
                ancestor = ancestor->parent;
            }
            widget_close(ancestor);
            handled = 1;
        }
    }
after_close_check:

    // "Go back" (back_button / b_button style code) event, only when the tag says close-on-no-
    // history and it has no matching event handler of its own for that code.
    if (controller_matches && handled == 0 && event[0] == 3 && *((int8_t *)event + 5) == 1) {
        int8_t code = (int8_t)event[2];
        uint8_t found = 0;

        if (code == '\r') {
            if (tag->event_handlers.count > 0) {
                int16_t *entry = (int16_t *)(tag->event_handlers.pointer + 4);
                int32_t i;

                for (i = 0; i < tag->event_handlers.count; i++, entry += 0x24) {
                    if (*entry == 0xd) {
                        found = 1;
                        break;
                    }
                }
            }
        } else if (code == 1) {
            if (tag->event_handlers.count > 0) {
                int16_t *entry = (int16_t *)(tag->event_handlers.pointer + 4);
                int32_t i;

                for (i = 0; i < tag->event_handlers.count; i++, entry += 0x24) {
                    if (*entry == 1) {
                        found = 1;
                        break;
                    }
                }
            }
        } else {
            found = 1; // any other code: skip straight to the "found" path (matches goto LAB_00499e3b)
        }
        if (!found) {
            widget_instance_close_and_restore_previous(widget);
            sound_effect = 3;
            list_nav_done = 1;
            handled = 1;
        }
    }

    if (widget->milliseconds_to_auto_close == 1) {
        widget->state = 0;
    }
    if (handled == 0) {
        if (widget->milliseconds_to_auto_close != 0) {
            int32_t fade = widget->milliseconds_auto_close_fade;
            int32_t close_at = ui_time_milliseconds - widget->creation_time;

            if ((uint32_t)(fade + widget->milliseconds_to_auto_close) <= (uint32_t)close_at) {
                widget_instance *ancestor = widget;

                while (ancestor->parent != (widget_instance *)0) {
                    ancestor = ancestor->parent;
                }
                widget_close(ancestor);
                handled = 1;
                goto dispatch_to_children;
            }
            if (fade != 0) {
                int32_t remaining = close_at - widget->milliseconds_to_auto_close;

                if (remaining > 0) {
                    widget_instance *ancestor = widget;

                    while (ancestor->parent != (widget_instance *)0) {
                        ancestor = ancestor->parent;
                    }
                    ancestor->scale = 1.0f - (float)remaining / (float)fade;
                }
            }
        }
        {
            int16_t *word_5a = (int16_t *)&widget->unknown_5a[0];
            int16_t *word_5c = (int16_t *)&widget->unknown_5a[2];

            if (*word_5a < 0) *word_5a = 0;
            if (*word_5c < 0) *word_5c = 0;
        }
        if (widget->widget_type == 2) {
            widget_spinner_list_sync_selected(widget, tag);
        } else if (widget->widget_type == 3) {
            widget_column_list_sync_selected(widget);
        }
        if (controller_matches) {
            if (list_nav_done != 0) {
                goto dispatch_to_children;
            }
            if ((tag->flags & 8) != 0 && widget->focused_child != (widget_instance *)0 && handled == 0) {
                if (event[0] == 3 && *((int8_t *)event + 5) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\b') {
                        goto tab_forward;
                    }
                    if (code != '\t') {
                        goto dpad_lr_nav;
                    }
                    widget_focus_next_child(widget);
                    goto tab_commit;
                } else if (event[0] == 1) {
                    if (event[3] != (int16_t)0x8000) {
                        if (event[3] == 0x7fff) {
                            goto tab_forward;
                        }
                        goto dpad_lr_nav;
                    }
                    goto tab_back;
                }
            }
            goto dpad_lr_nav;
        tab_back:
            widget_focus_next_child(widget);
        tab_commit:
            if (sound_effect == 0) {
                sound_effect = 1;
            }
            list_nav_done = 1;
            goto dpad_lr_nav_done;
        tab_forward:
            widget_focus_previous_child(widget);
            goto tab_commit;

        dpad_lr_nav:
            if ((tag->flags & 0x10) != 0 && widget->focused_child != (widget_instance *)0 && handled == 0) {
                if (event[0] == 3 && *((int8_t *)event + 5) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\n') {
                        widget_focus_previous_child(widget);
                        goto tab_commit;
                    }
                    if (code == '\v') {
                        widget_focus_next_child(widget);
                        goto tab_commit;
                    }
                } else if (event[0] == 1) {
                    if (event[2] == (int16_t)0x8000) {
                        widget_focus_previous_child(widget);
                        goto tab_commit;
                    }
                    if (event[2] == 0x7fff) {
                        widget_focus_next_child(widget);
                        goto tab_commit;
                    }
                }
            }
        dpad_lr_nav_done:
            if ((tag->flags & 0x20) != 0 && (widget->widget_type == 2 || widget->widget_type == 3) &&
                list_nav_done == 0 && handled == 0) {
                if (event[0] == 3 && *((int8_t *)event + 5) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\b') {
                        widget_list_select_previous(widget);
                    } else if (code == '\t') {
                        widget_list_select_next(widget);
                    } else {
                        goto dpad_ud_nav;
                    }
                    if (sound_effect == 0) {
                        sound_effect = 1;
                    }
                    list_nav_done = 1;
                } else if (event[0] == 1) {
                    if (event[3] == (int16_t)0x8000) {
                        widget_list_select_next(widget);
                        if (sound_effect == 0) sound_effect = 1;
                        list_nav_done = 1;
                    } else if (event[3] == 0x7fff) {
                        widget_list_select_previous(widget);
                        if (sound_effect == 0) sound_effect = 1;
                        list_nav_done = 1;
                    }
                }
            }
        dpad_ud_nav:
            if ((tag->flags & 0x40) != 0 && (widget->widget_type == 2 || widget->widget_type == 3) &&
                list_nav_done == 0 && handled == 0) {
                if (event[0] == 3 && *((int8_t *)event + 5) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\n') {
                        widget_list_select_previous(widget);
                    } else if (code == '\v') {
                        widget_list_select_next(widget);
                    } else {
                        goto dispatch_to_children;
                    }
                    if (sound_effect == 0) sound_effect = 1; // objdump 0x49a1c8
                    list_nav_done = 1;
                } else if (event[0] == 1) {
                    if (event[2] == (int16_t)0x8000) {
                        widget_list_select_previous(widget);
                        if (sound_effect == 0) sound_effect = 1; // objdump 0x49a1c8
                        list_nav_done = 1;
                    } else if (event[2] == 0x7fff) {
                        widget_list_select_next(widget);
                        if (sound_effect == 0) sound_effect = 1; // objdump 0x49a1c8
                        list_nav_done = 1;
                    }
                }
            }
            goto dispatch_to_children;
        }
    }
    goto dispatch_to_children;

dispatch_to_children:
    // Scan the tag's own event_handlers for one that matches this event.
    if (controller_matches && tag->event_handlers.count > 0) {
        int16_t *entry_base = (int16_t *)tag->event_handlers.pointer;
        int32_t offset = 0;

        handler_scan_count = 0;
        while (handler_scan_count < tag->event_handlers.count) {
            if (handled != 0) {
                break;
            }
            {
                uint8_t *entry = (uint8_t *)entry_base + offset;
                int16_t event_type = *(int16_t *)(entry + 4);
                uint8_t match = 0;

                switch (event[0]) {
                case 1:
                    switch (event_type) {
                    case 0x10: match = (event[3] == 0x7fff); break;
                    case 0x11: match = (event[3] == (int16_t)0x8000); break;
                    case 0x12: match = (event[2] == (int16_t)0x8000); break;
                    case 0x13: match = (event[2] == 0x7fff); break;
                    default: goto scan_next;
                    }
                    break;
                case 2:
                    switch (event_type) {
                    case 0x14: match = (event[3] == 0x7fff); break;
                    case 0x15: match = (event[3] == (int16_t)0x8000); break;
                    case 0x16: match = (event[2] == (int16_t)0x8000); break;
                    case 0x17: match = (event[2] == 0x7fff); break;
                    default: goto scan_next;
                    }
                    break;
                case 3:
                    if (event_type == (uint16_t)(uint8_t)event[2]) {
                        match = (*((int8_t *)event + 5) == 1);
                    }
                    break;
                case 4:
                    if (*((int8_t *)event + 5) == 1 && widget_instance_point_in_bounds(widget) != 0) {
                        switch (event_type) {
                        case 0x1c: match = ((int8_t)event[2] == 0); break;
                        case 0x1d: match = ((int8_t)event[2] == 1); break;
                        case 0x1e: match = ((int8_t)event[2] == 2); break;
                        case 0x1f: match = ((int8_t)event[2] == 3); break;
                        default: goto scan_next;
                        }
                    }
                    break;
                case 5:
                    match = (event_type == 0x20);
                    break;
                default:
                    break;
                }
                if (match) {
                    list_nav_done = 1;
                    ui_widget_list_item_activate(widget, tag, event, entry, &handled);
                }
            }
        scan_next:
            handler_scan_count = handler_scan_count + 1;
            offset = offset + 0x48;
        }
    }

    {
        uint32_t flags = tag->flags;

        if (((flags & 0x400) != 0 || list_nav_done == 0) && ((flags & 1) != 0 || (flags & 0x100) != 0) &&
            handled == 0) {
            if ((flags & 0x100) == 0) {
                widget_instance *child = widget->focused_child;

                if (child != (widget_instance *)0 &&
                    (child->controller_index == -1 || child->controller_index == event[1])) {
                    UIWidgetDefinition *child_tag =
                        (UIWidgetDefinition *)tag_instances[child->definition & 0xffff].data;

                    widget_instance_handle_input_event(child, child_tag, event, &handled);
                }
            } else {
                widget_instance *child = widget->first_child;

                while (child != (widget_instance *)0) {
                    if (child->controller_index == -1 || child->controller_index == event[1]) {
                        UIWidgetDefinition *child_tag =
                            (UIWidgetDefinition *)tag_instances[child->definition & 0xffff].data;

                        widget_instance_handle_input_event(child, child_tag, event, &handled);
                        if (handled == 1) {
                            break;
                        }
                    }
                    child = child->next_sibling;
                }
            }
        }
    }

    if (handled == 1 && (tag->flags & 0x800) != 0) {
        int32_t i;

        for (i = 0; i < 1; i++) {
            if (ui_root_widget[i] != (widget_instance *)0) {
                break;
            }
        }
        if (i == 1) {
            split_screen_quit_prompt_string = 0xffff;
            network_join_error_reason = 0;
            split_screen_quit_prompt_armed = 1;
        }
    }

    widget_play_sound_effect(sound_effect);
    *out_handled = handled;
}

#if 0
Original Ghidra decompilation (0x499d00):

void widget_instance_handle_input_event(int param_1,int param_2,short *param_3,char *param_4)

{
  int iVar1;
  uint uVar2;
  float fVar3;
  char cVar4;
  short sVar5;
  short *psVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  uint *puVar10;
  bool bVar11;
  char local_b;
  char local_a;
  char local_9;
  int local_8;
  int local_4;

  local_a = '\0';
  local_b = '\0';
  if ((*(char *)(param_1 + 0x12) == '\0') &&
     ((*(short *)(param_1 + 8) == -1 || (*(short *)(param_1 + 8) == param_3[1])))) {
    local_9 = '\x01';
  }
  else {
    local_9 = '\0';
  }
  iVar8 = 0;
  local_8 = 0;
  if (*(char *)(param_1 + 0x16) == '\x01') {
    sVar5 = *(short *)(param_1 + 8);
    iVar7 = param_1;
    if ((sVar5 < 0) || (3 < sVar5)) {
      iVar9 = 0;
LAB_00499d80:
      if ((&DAT_006b2ce8)[(short)iVar9] == -1) goto code_r0x00499d8c;
      iVar9 = *(int *)(param_1 + 0x30);
      while (iVar1 = iVar9, iVar1 != 0) {
        iVar7 = iVar1;
        iVar9 = *(int *)(iVar1 + 0x30);
      }
      goto LAB_00499da9;
    }
    if ((&DAT_006b2ce8)[sVar5] != -1) {
      iVar9 = *(int *)(param_1 + 0x30);
      while (iVar1 = iVar9, iVar1 != 0) {
        iVar7 = iVar1;
        iVar9 = *(int *)(iVar1 + 0x30);
      }
LAB_00499da9:
      widget_close(iVar7);
      local_b = '\x01';
    }
  }
LAB_00499db7:
  if ((((local_9 != '\0') && (local_b == '\0')) && (*param_3 == 3)) &&
     (*(char *)((int)param_3 + 5) == '\x01')) {
    if ((char)param_3[2] == '\r') {
      if (0 < *(int *)(param_2 + 0x54)) {
        psVar6 = (short *)(*(int *)(param_2 + 0x58) + 4);
        iVar7 = 0;
        do {
          if (*psVar6 == 0xd) goto LAB_00499e3b;
          iVar7 = iVar7 + 1;
          psVar6 = psVar6 + 0x24;
        } while (iVar7 < *(int *)(param_2 + 0x54));
      }
    }
    else {
      if ((char)param_3[2] != '\x01') goto LAB_00499e3b;
      iVar7 = 0;
      if (0 < *(int *)(param_2 + 0x54)) {
        psVar6 = (short *)(*(int *)(param_2 + 0x58) + 4);
        do {
          if (*psVar6 == 1) goto LAB_00499e3b;
          iVar7 = iVar7 + 1;
          psVar6 = psVar6 + 0x24;
        } while (iVar7 < *(int *)(param_2 + 0x54));
      }
    }
    FUN_0049c3e0();
    iVar8 = 3;
    local_8 = 3;
    local_a = '\x01';
    local_b = '\x01';
  }
LAB_00499e3b:
  iVar7 = *(int *)(param_1 + 0x1c);
  if (iVar7 == 1) {
    *(undefined1 *)(param_1 + 0x10) = 0;
  }
  if (local_b == '\0') {
    if (iVar7 != 0) {
      iVar9 = *(int *)(param_1 + 0x20);
      if ((uint)(iVar9 + iVar7) <= (uint)(DAT_00718f9c - *(int *)(param_1 + 0x18))) {
        iVar7 = *(int *)(param_1 + 0x30);
        iVar8 = param_1;
        while (iVar9 = iVar7, iVar9 != 0) {
          iVar8 = iVar9;
          iVar7 = *(int *)(iVar9 + 0x30);
        }
        widget_close(iVar8);
        local_b = '\x01';
        goto LAB_00499e93;
      }
      iVar8 = local_8;
      if ((iVar9 != 0) && (local_4 = (DAT_00718f9c - *(int *)(param_1 + 0x18)) - iVar7, 0 < local_4)
         ) {
        iVar7 = *(int *)(param_1 + 0x30);
        iVar1 = param_1;
        while (iVar7 != 0) {
          iVar1 = *(int *)(iVar1 + 0x30);
          iVar7 = *(int *)(iVar1 + 0x30);
        }
        fVar3 = (float)iVar9;
        if (iVar9 < 0) {
          fVar3 = fVar3 + 4.2949673e+09;
        }
        *(float *)(iVar1 + 0x24) = 1.0 - (float)local_4 / fVar3;
        local_4 = iVar9;
      }
    }
    sVar5 = *(short *)(param_1 + 0x5a);
    if (sVar5 < 0) {
      sVar5 = 0;
    }
    *(short *)(param_1 + 0x5a) = sVar5;
    sVar5 = *(short *)(param_1 + 0x5c);
    if (sVar5 < 0) {
      sVar5 = 0;
    }
    *(short *)(param_1 + 0x5c) = sVar5;
    if (*(short *)(param_1 + 0xe) == 2) {
      FUN_0049c000();
    }
    else if (*(short *)(param_1 + 0xe) == 3) {
      FUN_0049c040();
    }
    if (local_9 != '\0') {
      if (local_a != '\0') goto LAB_0049a06e;
      if ((((*(uint *)(param_2 + 0x2c) & 8) == 0) || (*(int *)(param_1 + 0x38) == 0)) ||
         (local_b != '\0')) goto LAB_00499ffc;
      if ((*param_3 == 3) && (*(char *)((int)param_3 + 5) == '\x01')) {
        if ((char)param_3[2] == '\b') goto LAB_0049a055;
        if ((char)param_3[2] != '\t') goto LAB_00499ffc;
        FUN_0049c080();
LAB_0049a05c:
        if (iVar8 == 0) {
          iVar8 = 1;
          local_8 = 1;
        }
        local_a = '\x01';
      }
      else {
        if (*param_3 == 1) {
          if (param_3[3] != -0x8000) {
            if (param_3[3] == 0x7fff) goto LAB_0049a055;
            goto LAB_00499ffc;
          }
LAB_0049a04c:
          FUN_0049c080();
          goto LAB_0049a05c;
        }
LAB_00499ffc:
        if ((((*(uint *)(param_2 + 0x2c) & 0x10) != 0) && (*(int *)(param_1 + 0x38) != 0)) &&
           (local_b == '\0')) {
          if ((*param_3 == 3) && (*(char *)((int)param_3 + 5) == '\x01')) {
            if ((char)param_3[2] == '\n') {
LAB_0049a055:
              FUN_0049c0f0();
              goto LAB_0049a05c;
            }
            if ((char)param_3[2] == '\v') {
              FUN_0049c080();
              goto LAB_0049a05c;
            }
          }
          else if (*param_3 == 1) {
            if (param_3[2] == -0x8000) goto LAB_0049a055;
            if (param_3[2] == 0x7fff) goto LAB_0049a04c;
          }
        }
      }
LAB_0049a06e:
      if (((((*(byte *)(param_2 + 0x2c) & 0x20) != 0) &&
           ((*(short *)(param_1 + 0xe) == 2 || (*(short *)(param_1 + 0xe) == 3)))) &&
          (local_a == '\0')) && (local_b == '\0')) {
        if ((*param_3 == 3) && (*(char *)((int)param_3 + 5) == '\x01')) {
          if ((char)param_3[2] == '\b') {
            widget_list_select_previous(param_1,param_3,&local_b);
          }
          else {
            if ((char)param_3[2] != '\t') goto LAB_0049a0dc;
LAB_0049a0bb:
            widget_list_select_next(param_1,param_3,&local_b);
          }
LAB_0049a0c7:
          if (iVar8 == 0) {
            iVar8 = 1;
            local_8 = 1;
          }
          local_a = '\x01';
        }
        else if (*param_3 == 1) {
          if (param_3[3] == -0x8000) goto LAB_0049a0bb;
          if (param_3[3] == 0x7fff) {
            widget_list_select_previous(param_1,param_3,&local_b);
            goto LAB_0049a0c7;
          }
        }
      }
LAB_0049a0dc:
      if (((*(byte *)(param_2 + 0x2c) & 0x40) != 0) &&
         ((((*(short *)(param_1 + 0xe) == 2 || (*(short *)(param_1 + 0xe) == 3)) &&
           (local_a == '\0')) && (local_b == '\0')))) {
        if ((*param_3 == 3) && (*(char *)((int)param_3 + 5) == '\x01')) {
          if ((char)param_3[2] == '\n') {
LAB_0049a1be:
            widget_list_select_previous(param_1,param_3,&local_b);
          }
          else {
            if ((char)param_3[2] != '\v') goto LAB_00499e93;
            widget_list_select_next(param_1,param_3,&local_b);
          }
LAB_0049a1c5:
          if (iVar8 == 0) {
            local_8 = 1;
          }
          local_a = '\x01';
        }
        else if (*param_3 == 1) {
          if (param_3[2] == -0x8000) goto LAB_0049a1be;
          if (param_3[2] == 0x7fff) {
            widget_list_select_next(param_1,param_3,&local_b);
            goto LAB_0049a1c5;
          }
        }
      }
      goto LAB_00499e93;
    }
  }
  else {
LAB_00499e93:
    if ((local_9 != '\0') && (iVar8 = 0, 0 < *(int *)(param_2 + 0x54))) {
      iVar7 = 0;
      do {
        if (local_b != '\0') break;
        iVar9 = *(int *)(param_2 + 0x58) + iVar7;
        switch(*param_3) {
        case 1:
          switch(*(undefined2 *)(iVar9 + 4)) {
          case 0x10:
switchD_0049a20f_caseD_14:
            bVar11 = param_3[3] == 0x7fff;
            break;
          case 0x11:
switchD_0049a20f_caseD_15:
            bVar11 = param_3[3] == -0x8000;
            break;
          case 0x12:
switchD_0049a20f_caseD_16:
            bVar11 = param_3[2] == -0x8000;
            break;
          case 0x13:
switchD_0049a20f_caseD_17:
            bVar11 = param_3[2] == 0x7fff;
            break;
          default:
            goto switchD_00499ed2_default;
          }
          goto LAB_0049a283;
        case 2:
          switch(*(undefined2 *)(iVar9 + 4)) {
          case 0x14:
            goto switchD_0049a20f_caseD_14;
          case 0x15:
            goto switchD_0049a20f_caseD_15;
          case 0x16:
            goto switchD_0049a20f_caseD_16;
          case 0x17:
            goto switchD_0049a20f_caseD_17;
          }
          break;
        case 3:
          if (*(ushort *)(iVar9 + 4) == (ushort)*(byte *)(param_3 + 2)) {
            bVar11 = *(char *)((int)param_3 + 5) == '\x01';
            goto LAB_0049a283;
          }
          break;
        case 4:
          if ((*(char *)((int)param_3 + 5) == '\x01') &&
             (cVar4 = widget_instance_point_in_bounds(), cVar4 != '\0')) {
            switch(*(undefined2 *)(iVar9 + 4)) {
            case 0x1c:
              bVar11 = (char)param_3[2] == '\0';
              break;
            case 0x1d:
              bVar11 = (char)param_3[2] == '\x01';
              break;
            case 0x1e:
              bVar11 = (char)param_3[2] == '\x02';
              break;
            case 0x1f:
              bVar11 = (char)param_3[2] == '\x03';
              break;
            default:
              goto switchD_00499ed2_default;
            }
            goto LAB_0049a283;
          }
          break;
        case 5:
          bVar11 = *(short *)(iVar9 + 4) == 0x20;
LAB_0049a283:
          if (bVar11) {
            local_a = '\x01';
            ui_widget_list_item_activate(param_1,param_2,param_3,iVar9,&local_b);
          }
        }
switchD_00499ed2_default:
        iVar8 = iVar8 + 1;
        iVar7 = iVar7 + 0x48;
      } while (iVar8 < *(int *)(param_2 + 0x54));
    }
  }
  uVar2 = *(uint *)(param_2 + 0x2c);
  if (((((uVar2 & 0x400) != 0) || (local_a == '\0')) &&
      (((uVar2 & 1) != 0 || ((uVar2 & 0x100) != 0)))) && (local_b == '\0')) {
    if ((uVar2 & 0x100) == 0) {
      puVar10 = *(uint **)(param_1 + 0x38);
      if ((puVar10 == (uint *)0x0) ||
         (((short)puVar10[2] != -1 && ((short)puVar10[2] != param_3[1])))) goto LAB_0049a3cb;
      widget_instance_handle_input_event
                (puVar10,*(undefined4 *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),param_3,
                 &local_b);
    }
    else {
      puVar10 = *(uint **)(param_1 + 0x34);
      if (puVar10 == (uint *)0x0) goto LAB_0049a3cb;
      do {
        if ((((short)puVar10[2] == -1) || ((short)puVar10[2] == param_3[1])) &&
           (widget_instance_handle_input_event
                      (puVar10,*(undefined4 *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                       param_3,&local_b), local_b == '\x01')) goto LAB_0049a38d;
        puVar10 = (uint *)puVar10[0xb];
      } while (puVar10 != (uint *)0x0);
    }
  }
  if (local_b == '\x01') {
LAB_0049a38d:
    if ((*(uint *)(param_2 + 0x2c) & 0x800) != 0) {
      iVar8 = 0;
      do {
        if ((&DAT_00718f94)[iVar8] != 0) break;
        iVar8 = iVar8 + 1;
      } while (iVar8 < 1);
      if (iVar8 == 1) {
        DAT_00719754._0_2_ = 0xffff;
        DAT_0071973c = 0;
        DAT_00719754._3_1_ = 1;
      }
    }
  }
LAB_0049a3cb:
  widget_play_sound_effect();
  *param_4 = local_b;
  return;
code_r0x00499d8c:
  iVar9 = iVar9 + 1;
  if (3 < iVar9) goto LAB_00499db7;
  goto LAB_00499d80;
}
#endif
