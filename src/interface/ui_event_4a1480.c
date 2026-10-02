// ui_event_4a1480  (not a Ghidra function; ui_event_function_table[80])
// address 0x4a1480, size 233 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692910 (index 80); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1480.
// WRITTEN 2026-09-28 from objdump 0x4a1480..0x4a1568: picks a free saved game name for slot event word 1 (-1 means
//   0) and creates a default profile with it (0x539ab0; the pushed slot is not read). On success selects it, loads it
//   for player 0, copies up to 0xb characters of the name into the profile name (+2, terminated at +0x18) and returns
//   the result of opening the virtual keyboard on it (0x18 characters, field kind 8) when nonzero. Without a selected
//   profile the selection is dropped. Every failure raises quit confirm error 0x25 (when none is up), plays sound 4
//   and returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void saved_game_allocate_new_slot(uint16_t *out_name); // 0x53ca80, blam-cc: EBX out_name
extern uint32_t saved_game_create_default_profile(uint16_t *name); // 0x539ab0, blam-cc: ECX name (the pushed slot is not read)
extern void saved_item_select(int32_t item); // 0x495be0, blam-cc: EBX -> item
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970, blam-cc: AX, EDX, stack
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal; // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

uint8_t ui_event_4a1480(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint16_t name[0x82];
    uint32_t handle;

    saved_game_allocate_new_slot(name);
    if (name[0] != 0) {
        handle = saved_game_create_default_profile(name);
        if (handle != 0xffffffff) {
            uint8_t *profile;

            saved_item_select((int32_t)handle);
            profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
            player_profile_load(0, profile, (int32_t)handle);
            if (profile != 0) {
                uint8_t opened;

                wcsncpy((wchar_t *)(profile + 2), (const wchar_t *)name, 0xb);
                *(uint16_t *)(profile + 0x18) = 0;
                opened = virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8);
                if (opened != 0) {
                    return opened;
                }
            } else {
                selected_saved_item = -1;
            }
        }
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x25;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    widget_play_sound_effect(4);
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
