// ui_event_4a1310  (not a Ghidra function; ui_event_function_table[79])
// address 0x4a1310, size 368 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069290c (index 79); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1310.
// WRITTEN 2026-09-28 from objdump 0x4a1310..0x4a147f: picks a free saved game name and creates a custom variant
//   with it for the widget's controller (0x53bb50); on success selects it and, when it is a variant, fills the
//   working copy from the grandparent list's selected item data (or the classic slayer defaults when that item has no
//   id), clears the word at +0x94, copies up to 0x17 characters of the name (terminated at +0x2e), clears flag bits 7
//   and 8 (+0x38) and opens the virtual keyboard on the name (0x30 characters, field kind 9). Opened (1): edit field
//   2, clears the last multiplayer variant record of its directory when found, returns 1. Any other nonzero keyboard
//   result is returned. Every failure (a non-variant selection is also dropped) raises quit confirm error 0x26 when
//   none is up, sound 4, returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include <wchar.h>

extern void saved_game_allocate_new_slot(uint16_t *out_name); // 0x53ca80, blam-cc: EBX out_name
extern uint32_t saved_game_create_custom_variant(uint32_t param_1, uint16_t *name); // 0x53bb50, blam-cc: ECX name too
extern void saved_item_select(int32_t item); // 0x495be0, blam-cc: EBX -> item
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern int32_t ui_list_get_id(int32_t index); // 0x4a7c80, blam-cc: EDX index
extern void *ui_list_get_data(int32_t index); // 0x4a7c50, blam-cc: EDX index
extern void *game_engine_variant_defaults_classic_slayer(void *out); // 0x463c40
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination
extern int32_t network_host_edit_field_00719410; // 0x00719410, UNSURE name (3 after the name, 0 after the subname)
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: EAX handle, ESI out_directory
extern void saved_game_last_mp_variant_clear(const void *data); // 0x53d360
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal; // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

uint8_t ui_event_4a1310(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->parent->parent;
    uint16_t name[0x80];
    uint8_t scratch[0x100];
    uint32_t handle;

    saved_game_allocate_new_slot(name);
    if (name[0] != 0) {
        handle = saved_game_create_custom_variant((uint32_t)(uint16_t)widget->controller_index, name);
        if (handle != 0xffffffff) {
            saved_item_select((int32_t)handle);
            if ((selected_saved_item & 0xf) == 1) {
                int32_t id = ui_list_get_id(*(int16_t *)((uint8_t *)list + 0x3c));
                const void *source;
                uint8_t opened;

                source = id != -1 ? ui_list_get_data(id) : game_engine_variant_defaults_classic_slayer(scratch);
                memcpy(saved_item_working_copy, source, 0x98);
                *(uint16_t *)(saved_item_working_copy + 0x94) = 0;
                wcsncpy((wchar_t *)saved_item_working_copy, (const wchar_t *)name, 0x17);
                *(uint16_t *)(saved_item_working_copy + 0x2e) = 0;
                *(uint32_t *)(saved_item_working_copy + 0x38) &= 0xfffffe7f;
                opened = virtual_keyboard_open((uint16_t *)saved_item_working_copy, 0x30, 9);
                if (opened == 1) {
                    network_host_edit_field_00719410 = 2;
                    if (saved_game_get_directory_by_handle((int32_t)handle, (char *)scratch) != 0) {
                        saved_game_last_mp_variant_clear(scratch);
                    }
                    return 1;
                }
                if (opened != 0) {
                    return opened;
                }
            } else {
                selected_saved_item = -1;
            }
        }
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x26;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    widget_play_sound_effect(4);
    return 0;
}
