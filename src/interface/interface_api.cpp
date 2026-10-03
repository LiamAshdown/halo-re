#include "halo/interface/api.hpp"

extern "C" {
extern uint8_t chat_dialog_open;
extern int32_t controls_capture_row;
extern uint8_t controls_input_capture_flags;
extern int16_t current_local_player_index;
extern map_list_entry *map_list;
extern int32_t map_list_count;
extern uint8_t quit_confirm_error_is_error;
extern uint8_t quit_confirm_error_modal;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern int32_t selected_saved_item;
extern uint8_t split_screen_quit_prompt_armed;
extern heap *widget_memory_pool;
extern uint8_t widget_memory_pool_valid;
extern virtual_keyboard_globals virtual_keyboard;
extern HUDGlobals *hud_globals_tag_data;
extern hud_messaging_globals *hud_messaging;
extern hud_unit_meter_globals *hud_unit_meters;
extern first_person_weapon_interface *first_person_weapon_interfaces;
}

namespace halo::interface {

Globals &globals()
{
    static Globals instance{::chat_dialog_open, ::controls_capture_row, ::controls_input_capture_flags, ::current_local_player_index, ::map_list, ::map_list_count, ::quit_confirm_error_is_error, ::quit_confirm_error_modal, ::quit_confirm_error_string_index, ::quit_confirm_error_unknown_ae, ::selected_saved_item, ::split_screen_quit_prompt_armed, ::widget_memory_pool, ::widget_memory_pool_valid, ::virtual_keyboard, ::hud_globals_tag_data, ::hud_messaging, ::hud_unit_meters, ::first_person_weapon_interfaces};
    return instance;
}

}
