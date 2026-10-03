#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &chat_dialog_open = halo::link::ref<uint8_t>(halo::ui::vars().chat_dialog_open);
static auto &controls_capture_row = halo::link::ref<int32_t>(halo::ui::vars().controls_capture_row);
static auto &controls_input_capture_flags = halo::link::ref<uint8_t>(halo::ui::vars().controls_input_capture_flags);
static auto &current_local_player_index = halo::link::ref<int16_t>(halo::ui::vars().current_local_player_index);
static auto &map_list = halo::link::ref<map_list_entry *>(halo::ui::vars().map_list);
static auto &map_list_count = halo::link::ref<int32_t>(halo::ui::vars().map_list_count);
static auto &quit_confirm_error_is_error = halo::link::ref<uint8_t>(halo::ui::vars().quit_confirm_error_is_error);
static auto &quit_confirm_error_modal = halo::link::ref<uint8_t>(halo::ui::vars().quit_confirm_error_modal);
static auto &quit_confirm_error_string_index = halo::link::ref<int16_t>(halo::ui::vars().quit_confirm_error_string_index);
static auto &quit_confirm_error_unknown_ae = halo::link::ref<int16_t>(halo::ui::vars().quit_confirm_error_unknown_ae);
static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &split_screen_quit_prompt_armed = halo::link::ref<uint8_t>(halo::ui::vars().split_screen_quit_prompt_armed);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &widget_memory_pool_valid = halo::link::ref<uint8_t>(halo::ui::vars().widget_memory_pool_valid);
static auto &virtual_keyboard = halo::link::ref<virtual_keyboard_globals>(halo::ui::vars().virtual_keyboard);
static auto &hud_globals_tag_data = halo::link::ref<HUDGlobals *>(halo::ui::vars().hud_globals_tag_data);
static auto &hud_messaging = halo::link::ref<hud_messaging_globals *>(halo::ui::vars().hud_messaging);
static auto &hud_unit_meters = halo::link::ref<hud_unit_meter_globals *>(halo::ui::vars().hud_unit_meters);
static auto &first_person_weapon_interfaces = halo::link::ref<first_person_weapon_interface *>(halo::ui::vars().first_person_weapon_interfaces);

namespace halo::interface {

Globals &globals()
{
    static Globals instance{::chat_dialog_open, ::controls_capture_row, ::controls_input_capture_flags, ::current_local_player_index, ::map_list, ::map_list_count, ::quit_confirm_error_is_error, ::quit_confirm_error_modal, ::quit_confirm_error_string_index, ::quit_confirm_error_unknown_ae, ::selected_saved_item, ::split_screen_quit_prompt_armed, ::widget_memory_pool, ::widget_memory_pool_valid, ::virtual_keyboard, ::hud_globals_tag_data, ::hud_messaging, ::hud_unit_meters, ::first_person_weapon_interfaces};
    return instance;
}

}
