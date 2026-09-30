// ui_event_49dab0  (not a Ghidra function; ui_event_function_table[31])
// address 0x49dab0, size 258 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069284c (index 31); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49dab0.
// WRITTEN 2026-09-28 from objdump 0x49dab0..0x49dbb1: reads the widget list (+0x44) at the ui list id of its
//   committed selection (index -1 when out of range). -1: sound 4, returns 0. Not negative: raises quit confirm error
//   0x1f when none is up, sound 4, returns 0. Negative: loads that saved variant (0x98 bytes; failure returns 0),
//   clears the last multiplayer variant record of its directory when found, copies it over the saved default variant
//   (0x00714de0) and marks it valid; in network_game_mode 2 then makes sure the variant history has an entry, closes
//   every widget, begins the end game sequence and returns 0; else returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "fn_game.h"
#include "fn_interface.h"

extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal; // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern uint8_t saved_game_get_variant(int32_t handle, void *out); // 0x53bee0
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: EAX handle, ESI out_directory
extern void saved_game_last_mp_variant_clear(const void *data); // 0x53d360
extern uint8_t game_variant_saved_default[0x98]; // 0x00714de0 (game_variant)
extern uint8_t game_variant_saved_default_valid; // 0x00714e78
extern int16_t network_game_mode; // 0x00719720
extern uint32_t game_engine_ensure_variant_history_has_entry(void); // 0x463b20


static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

uint8_t ui_event_49dab0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint32_t variant[0x26];
    char directory[0x100];
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item = ((int32_t *)widget->list_items)[id];

    if (item == -1 || item >= 0) {
        if (item != -1 && quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = -1;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
        widget_play_sound_effect(4);
        return 0;
    }
    if (saved_game_get_variant(item, variant) == 0) {
        return 0;
    }
    if (saved_game_get_directory_by_handle(item, directory) != 0) {
        saved_game_last_mp_variant_clear(directory);
    }
    memcpy(game_variant_saved_default, variant, sizeof(variant));
    game_variant_saved_default_valid = 1;
    if (network_game_mode != 2) {
        return 1;
    }
    game_engine_ensure_variant_history_has_entry();
    widget_close_all();
    game_engine_begin_end_game_sequence();
    return 0;
}
