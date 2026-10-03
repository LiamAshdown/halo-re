/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget instance,
 * the event record and an out-flag and returns whether the event was consumed.
 */

#include "win32.h"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/text/api.hpp"
#include "halo/interface/engine_state.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include <wchar.h>
#include "game.h"

#include "halo/interface/uis_event_handlers.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/net_session.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/interface/constants.hpp"
#include "halo/interface/flags.hpp"

extern "C" {
extern uint8_t input_event_queue_active;
extern int32_t ui_cursor_x;
extern int32_t ui_cursor_y;
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[k_saved_player_profile_size];
extern int32_t profile_slot_lookup_cache_00692ac8;
extern int32_t ui_list_current;
extern growable_array ui_lists[3];
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern uint16_t network_host_name_00719170[0x40];
extern int32_t network_host_edit_field_00719410;
extern uint16_t network_host_subname_007191f0[9];
extern uint8_t save_in_progress_00719010;
extern int32_t resolution_selection_00719204;
extern uint8_t network_game_info_packet_flag;
extern int32_t quality_selection_00692b04;
extern uint8_t saved_item_has_unsaved_changes(void);
extern uint8_t player_profile_save(void);
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag;
extern void game_engine_send_team_allegiance_message(char broadcast);
extern uint8_t variant_teams_enabled_0071920c;
extern int32_t variant_team_selection_00692b08;
extern uint32_t network_game_option_a_00719210;
extern uint32_t network_game_option_b_00719214;
extern int32_t network_host_number_field_00719218;
extern uint16_t network_host_number_text_0071921c[0x10];
extern growable_array hud_text_message_queue;
extern int32_t hud_text_message_cycle_state_00719230;
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static void widget_absolute_origin(widget_instance *widget, int16_t *x, int16_t *y)
{
    *x = 0;
    *y = 0;
    for (; widget != 0; widget = widget->parent) {
        *x = (int16_t)(*x + widget->local_x);
        *y = (int16_t)(*y + widget->local_y);
    }
}

/** Local helper shared by the handlers of this file. */
static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != uiwidgettype_spinner_list) {
        child = child->next_sibling;
    }
    return child;
}

/** Local helper shared by the handlers of this file. */
static uint8_t clamp_selection(widget_instance *group, int16_t maximum)
{
    int16_t selection = first_list_child(group)->selection_index;

    return (uint8_t)(selection < 0 ? 0 : selection > maximum ? maximum : selection);
}

/** Local helper shared by the handlers of this file. */
static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

/** Local helper shared by the handlers of this file. */
static void row_clicked(widget_instance *list, int32_t row, int32_t old_committed, uint8_t double_click)
{
    halo::interface::widget_play_sound_effect(2);
    *(int16_t *)&((struct widget_instance *)list)->text = (int16_t)row;
    if (double_click && old_committed == row) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 5;
        halo::input::UiEvents::queue_push_event(0, &queued);
    }
}

}

/**
 * Slot 92 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1740
 */
uint8_t UiEventHandlers::event_4a1740(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint16_t *state;
    network_client_globals *client = halo::networking::globals().client;

    if (widget->item_count != 0 || client == 0) {
        return 0;
    }
    state = &client->state;
    if (*state == 1) {
        halo::cseries::time_query_performance_counter_ms();
    }
    if (*state != 0) {
        return 0;
    }
    return halo::interface::multiplayer_host_session_start();
}

/**
 * Slot 111 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1d30
 */
uint8_t UiEventHandlers::event_4a1d30(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0xb;
        queued.pressed = 1;
        halo::input::UiEvents::queue_push_event(0, &queued);
    }
    return 1;
}

/**
 * Slot 128 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1d60
 */
uint8_t UiEventHandlers::event_4a1d60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0x2;
        queued.pressed = 1;
        halo::input::UiEvents::queue_push_event(0, &queued);
    }
    return 1;
}

/**
 * Slot 164 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1d90
 */
uint8_t UiEventHandlers::event_4a1d90(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_input_event queued;

    memset(&queued, 0, sizeof(queued));
    queued.kind = 5;
    halo::input::UiEvents::queue_push_event(0, &queued);
    return 1;
}

/**
 * Slot 112 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1dc0
 */
uint8_t UiEventHandlers::event_4a1dc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *definition = halo::interface::tag_data<uint8_t>(widget->definition);
    int32_t x = ui_cursor_x;
    int32_t y = ui_cursor_y;
    int16_t origin_x;
    int16_t origin_y;
    widget_instance *child;

    widget_absolute_origin(widget, &origin_x, &origin_y);
    if (x >= 0 && x <= (int16_t)(((struct UIWidgetDefinition *)definition)->header_bounds.right + origin_x) &&
        y >= (int16_t)(((struct UIWidgetDefinition *)definition)->header_bounds.top + origin_y) && y <= (int16_t)(((struct UIWidgetDefinition *)definition)->header_bounds.bottom + origin_y)) {
        int32_t selection = widget->selection_index - 1;

        if (selection < 0) {
            selection = widget->item_count - 1;
        }
        if (selection != widget->selection_index) {
            widget->selection_index = (int16_t)selection;
            widget->scroll_blink = -4;
            widget->selection_direction = -1;
            halo::interface::widget_play_sound_effect(1);
        }
        return 1;
    }
    if (x >= (int16_t)(((struct UIWidgetDefinition *)definition)->footer_bounds.left + origin_x) && x <= halo::interface::k_base_screen_width &&
        y >= (int16_t)(((struct UIWidgetDefinition *)definition)->footer_bounds.top + origin_y) && y <= (int16_t)(((struct UIWidgetDefinition *)definition)->footer_bounds.bottom + origin_y)) {
        int32_t selection = widget->selection_index + 1;

        if (selection >= widget->item_count) {
            selection = 0;
        }
        if (selection != widget->selection_index) {
            widget->selection_index = (int16_t)selection;
            widget->scroll_blink = 4;
            widget->selection_direction = 1;
            halo::interface::widget_play_sound_effect(1);
        }
        return 1;
    }
    for (child = widget->first_child; child != 0; child = child->next_sibling) {
        uint8_t *bounds = halo::interface::tag_data<uint8_t>(child->definition);
        int16_t cx;
        int16_t cy;

        widget_absolute_origin(child, &cx, &cy);
        if (x >= (int16_t)(((struct UIWidgetDefinition *)bounds)->bounds.left + cx) && x <= (int16_t)(((struct UIWidgetDefinition *)bounds)->bounds.right + cx) &&
            y >= (int16_t)(((struct UIWidgetDefinition *)bounds)->bounds.top + cy) && y <= (int16_t)(((struct UIWidgetDefinition *)bounds)->bounds.bottom + cy)) {
            if (input_event_queue_active != 0) {
                ui_input_event queued;

                memset(&queued, 0, sizeof(queued));
                queued.kind = 3;
                queued.code = 0;
                queued.pressed = 1;
                halo::input::UiEvents::queue_push_event(0, &queued);
            }
            return 1;
        }
    }
    return 1;
}

/**
 * Slot 121 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2190
 */
uint8_t UiEventHandlers::event_4a2190(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const saved_player_profile *profile = (selected_saved_item & 0xf) == 0 ? (const saved_player_profile *)saved_item_working_copy : nullptr;

    if (profile == 0) {
        return 0;
    }
    halo::interface::ui_controls_populate_sensitivity_row(widget, profile);
    return 1;
}

/**
 * Slot 122 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a21c0
 */
uint8_t UiEventHandlers::event_4a21c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;

    if (profile == 0) {
        return 0;
    }
    group = widget->parent->parent->first_child;
    ((struct saved_player_profile *)profile)->mouse_look_x_sensitivity = (uint8_t)(first_list_child(group)->selection_index + 1);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->mouse_look_y_sensitivity = (uint8_t)(first_list_child(group)->selection_index + 1);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->look_inverted = (uint8_t)(first_list_child(group)->selection_index == 1);
    return 1;
}

/**
 * Slot 123 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2490
 */
uint8_t UiEventHandlers::event_4a2490(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const saved_player_profile *profile = (selected_saved_item & 0xf) == 0 ? (const saved_player_profile *)saved_item_working_copy : nullptr;

    if (profile == 0) {
        return 0;
    }
    halo::interface::ui_controls_populate_input_row(widget, profile);
    return 1;
}

/**
 * Slot 124 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a24c0
 */
uint8_t UiEventHandlers::event_4a24c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    group = widget->parent->parent->first_child;
    ((struct saved_player_profile *)profile)->master_volume = clamp_selection(group, 10);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->effects_volume = clamp_selection(group, 10);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->music_volume = clamp_selection(group, 10);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->eax_enabled = (uint8_t)(first_list_child(group)->selection_index != 0);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->sound_quality = clamp_selection(group, 2);
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    ((struct saved_player_profile *)profile)->hardware_acceleration = (uint8_t)(selection >= 1 && ((struct saved_player_profile *)profile)->eax_enabled != 0);
    group = group->next_sibling;
    ((struct saved_player_profile *)profile)->sound_variety = clamp_selection(group, 2);
    return 1;
}

/**
 * Slot 134 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2950
 */
uint8_t UiEventHandlers::event_4a2950(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    saved_player_profile profile;
    uint8_t ok;

    memset(&profile, 0, sizeof(profile));
    ok = halo::saved_games::player_profile_set_default_audio_options(&profile);
    if (ok != 0) {
        halo::interface::ui_controls_populate_input_row(widget->parent->parent, &profile);
        halo::interface::widget_play_sound_effect(2);
    }
    return ok;
}

/**
 * Slot 139 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2a00
 */
uint8_t UiEventHandlers::event_4a2a00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    saved_player_profile profile;
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item;

    profile_slot_lookup_cache_00692ac8 = -1;
    item = ((int32_t *)widget->list_items)[id];
    if (item == -1) {
        return 0;
    }
    if (item < 0) {
        if (halo::saved_games::player_profile_get(item, &profile) == 0) {
            return 0;
        }
        halo::interface::player_profile_load(0, &profile, item);
        return 1;
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x36;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    halo::interface::widget_play_sound_effect(4);
    return 0;
}

/**
 * Slot 142 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2c50
 */
uint8_t UiEventHandlers::event_4a2c50(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (halo::interface::virtual_keyboard_open(network_host_name_00719170, 0x80, 0xb) == 0) {
        return 0;
    }
    network_host_edit_field_00719410 = 3;
    return 1;
}

/**
 * Slot 143 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2c80
 */
uint8_t UiEventHandlers::event_4a2c80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (halo::interface::virtual_keyboard_open(network_host_subname_007191f0, 0x12, 0xc) == 0) {
        return 0;
    }
    network_host_edit_field_00719410 = 0;
    return 1;
}

/**
 * Slot 144 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a2f10
 */
uint8_t UiEventHandlers::event_4a2f10(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t handle = halo::saved_games::globals().player_profile_slots_handle;

    if (handle != -1) {
        uint8_t *profile;

        halo::interface::saved_item_select(handle);
        profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
        wcscpy((wchar_t *)(((struct saved_player_profile *)profile)->server_name), (const wchar_t *)network_host_name_00719170);
        wcscpy((wchar_t *)(((struct saved_player_profile *)profile)->server_password), (const wchar_t *)network_host_subname_007191f0);
        if (save_in_progress_00719010 == 0) {
            ((struct saved_player_profile *)profile)->server_maximum_players_index = (uint8_t)resolution_selection_00719204;
        }
        if (network_game_info_packet_flag != 0) {
            int32_t quality = quality_selection_00692b04;

            ((struct saved_player_profile *)profile)->connection_type = (uint8_t)(quality < 0 ? 0 : quality > 4 ? 4 : quality);
        }
        if (halo::interface::saved_item_has_unsaved_changes() != 0) {
            halo::interface::player_profile_save();
        } else {
            selected_saved_item = -1;
        }
    }
    if (save_in_progress_00719010 != 0) {
        return 0;
    }
    halo::networking::network_game_start_new_server_with_name_and_password(0, network_host_name_00719170, network_host_subname_007191f0);
    return 0;
}

/**
 * Slot 155 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3000
 */
uint8_t UiEventHandlers::event_4a3000(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->first_child->next_sibling;

    if (halo::networking::globals().game_mode == 2 || (current_game_engine != 0 && game_engine_teams_enabled_flag != 0)) {
        child->hidden = 0;
        child->scale = 1.0f;
    } else {
        child->hidden = 1;
        *(uint32_t *)&child->scale = halo::interface::k_widget_default_scale_bits;
    }
    return 1;
}

/**
 * Slot 156 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3050
 */
uint8_t UiEventHandlers::event_4a3050(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->first_child;
    int32_t i;

    if (halo::game::globals().current_engine != 0 && halo::game::globals().teams_enabled != 0) {
        child->hidden = 0;
        child->scale = 1.0f;
    } else {
        child->hidden = 1;
        *(uint32_t *)&child->scale = halo::interface::k_widget_default_scale_bits;
        child->parent->focused_child = child->parent->first_child->next_sibling;
    }
    for (i = 0; i < 2; i++) {
        child = child->next_sibling;
        if (halo::networking::globals().game_mode == 2) {
            child->hidden = 0;
            child->scale = 1.0f;
        } else {
            child->hidden = 1;
            *(uint32_t *)&child->scale = halo::interface::k_widget_default_scale_bits;
        }
    }
    return 1;
}

/**
 * Slot 157 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3150
 */
uint8_t UiEventHandlers::event_4a3150(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *first = widget->parent->first_child;

    if (first == widget) {
        halo::game::game_engine_send_team_allegiance_message(1);
        return 1;
    }
    if (first->next_sibling == widget) {
        halo::game::game_engine_send_team_allegiance_message(0);
        return 1;
    }
    return 0;
}

/**
 * Slot 158 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a33a0
 */
uint8_t UiEventHandlers::event_4a33a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *first;
    widget_instance *second;
    widget_instance *second_list;
    int32_t time;

    if (variant == 0) {
        return 0;
    }
    time = ((struct game_variant *)variant)->vehicle_respawn_time;
    variant_teams_enabled_0071920c = (uint8_t)(((struct game_variant *)variant)->teams != 0);
    halo::interface::state::vehicle_options_team_page = 0;
    halo::interface::state::vehicle_options_red_set = ((struct game_variant *)variant)->red_vehicle_set;
    halo::interface::state::vehicle_options_blue_set = ((struct game_variant *)variant)->blue_vehicle_set;
    halo::interface::state::vehicle_options_respawn_time = (uint32_t)time;
    halo::interface::ui_controls_populate_bind_rows(widget, ((struct game_variant *)variant)->red_vehicle_set);
    first = widget->first_child;
    first_list_child(first)->selection_index = (int16_t)(time == 30 * halo::interface::k_ticks_per_second ? 1 : time == 1 * halo::interface::k_ticks_per_minute ? 2 : time == 90 * halo::interface::k_ticks_per_second ? 3 :
        time == 2 * halo::interface::k_ticks_per_minute ? 4 : time == 3 * halo::interface::k_ticks_per_minute ? 5 : time == 5 * halo::interface::k_ticks_per_minute ? 6 : 0);
    second = first->next_sibling;
    second_list = first_list_child(second);
    if (variant_teams_enabled_0071920c != 0) {
        second->hidden = 0;
        second->state = 1;
    } else {
        second->hidden = 1;
        second->state = 0;
    }
    first_list_child(second->next_sibling)->selection_index = (int16_t)((halo::interface::state::vehicle_options_red_set & 0xf) < 9 ? (halo::interface::state::vehicle_options_red_set & 0xf) : 0);
    variant_team_selection_00692b08 = second_list->selection_index;
    return 1;
}

/**
 * Slot 159 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3510
 */
uint8_t UiEventHandlers::event_4a3510(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if ((selected_saved_item & 0xf) == 1) {
        uint32_t *out = (uint32_t *)(saved_item_working_copy + 0x60);

        out[0] = halo::interface::state::vehicle_options_red_set;
        out[1] = halo::interface::state::vehicle_options_blue_set;
        out[2] = halo::interface::state::vehicle_options_respawn_time;
    }
    return 1;
}

/**
 * Slot 160 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3540
 */
uint8_t UiEventHandlers::event_4a3540(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->parent;
    uint8_t *definition = halo::interface::tag_data<uint8_t>(list->definition);
    int32_t rows = (int32_t)((struct UIWidgetDefinition *)definition)->child_widgets.count;
    int32_t first_visible = *(int16_t *)((uint8_t *)list + 0x3e);
    int32_t committed = *(int16_t *)&((struct widget_instance *)list)->text;
    widget_instance *child = list->first_child;
    uint8_t header = (uint8_t)(child != 0 && child->first_child != 0 && child->first_child->widget_type == uiwidgettype_spinner_list);
    uint8_t double_click = 0;
    uint8_t fits;
    int32_t shown;
    int32_t row;
    int32_t position;

    if (event[0] == 3 && (((uint8_t *)event)[4] == 0 || ((uint8_t *)event)[4] == 0xc) &&
        halo::cseries::time_query_performance_counter_ms() - (uint32_t)widget->creation_time > 0xfa) {
        double_click = 1;
    }
    if (header) {
        rows--;
    }
    fits = (uint8_t)(halo::interface::has_bit(((UIWidgetDefinition *)definition)->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_single_preview_no_scroll) || (int32_t)list->item_count <= rows - 1);
    shown = rows - (fits ? 1 : 3);
    if (shown > (int32_t)list->item_count) {
        shown = list->item_count;
    }
    row = first_visible;
    for (position = 0; child != 0 && child->next_sibling != 0; position++, row++, child = child->next_sibling) {
        if (child == widget) {
            break;
        }
        if ((position == 0 && (header || !fits)) || (position == 1 && !fits && header)) {
            row--;
        }
    }
    if (child == 0 || child->next_sibling == 0) {
        return 1;
    }
    if ((position == 0 && !header) || (position == 1 && header)) {
        if (fits) {
            row_clicked(list, row, committed, double_click);
            return 1;
        }
        first_visible = first_visible + 1 - shown;
        if (first_visible < 0) {
            first_visible = 0;
        }
        *(int16_t *)((uint8_t *)list + 0x3e) = (int16_t)first_visible;
        halo::interface::widget_play_sound_effect(2);
        return 1;
    }
    if (child->next_sibling->next_sibling == 0 && !fits) {
        int32_t last = first_visible + shown - 1;

        if (last >= (int32_t)list->item_count - shown) {
            last = (int32_t)list->item_count - shown;
        }
        *(int16_t *)((uint8_t *)list + 0x3e) = (int16_t)last;
        halo::interface::widget_play_sound_effect(2);
        return 1;
    }
    row_clicked(list, row, committed, double_click);
    return 1;
}

/**
 * Slot 161 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3790
 */
uint8_t UiEventHandlers::event_4a3790(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = saved_item_working_copy;
    widget_instance *group;
    int32_t time;

    if ((selected_saved_item & 0xf) != 1) {
        return 1;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->friendly_fire <= 3 ? ((struct game_variant *)variant)->friendly_fire : 1);
    group = group->next_sibling;
    time = ((struct game_variant *)variant)->betrayal_penalty;
    first_list_child(group)->selection_index = (int16_t)(time == 5 * halo::interface::k_ticks_per_second ? 1 : time == 10 * halo::interface::k_ticks_per_second ? 2 : time == 15 * halo::interface::k_ticks_per_second ? 3 : 0);
    first_list_child(group->next_sibling)->selection_index = (int16_t)(((struct game_variant *)variant)->team_autobalance != 0);
    return 1;
}

/**
 * Slot 162 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3870
 */
uint8_t UiEventHandlers::event_4a3870(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = saved_item_working_copy;
    widget_instance *group;
    int16_t selection;

    if ((selected_saved_item & 0xf) != 1) {
        return 1;
    }
    group = widget->parent->parent->first_child;
    selection = first_list_child(group)->selection_index;
    ((struct game_variant *)variant)->friendly_fire = (uint8_t)(selection >= 0 && selection <= 3 ? selection : 1);
    group = group->next_sibling;
    switch (first_list_child(group)->selection_index) {
    case 1:
        ((struct game_variant *)variant)->betrayal_penalty = 5 * halo::interface::k_ticks_per_second;
        break;
    case 2:
        ((struct game_variant *)variant)->betrayal_penalty = 10 * halo::interface::k_ticks_per_second;
        break;
    case 3:
        ((struct game_variant *)variant)->betrayal_penalty = 15 * halo::interface::k_ticks_per_second;
        break;
    default:
        ((struct game_variant *)variant)->betrayal_penalty = 0;
        break;
    }
    ((struct game_variant *)variant)->team_autobalance = (uint8_t)(first_list_child(group->next_sibling)->selection_index == 1);
    return 1;
}

/**
 * Slot 166 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a39c0
 */
uint8_t UiEventHandlers::event_4a39c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const saved_player_profile *profile = (selected_saved_item & 0xf) == 0 ? (const saved_player_profile *)saved_item_working_copy : nullptr;

    return halo::interface::ui_network_game_options_populate(widget, profile);
}

/**
 * Slot 167 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a39e0
 */
uint8_t UiEventHandlers::event_4a39e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    selection = first_list_child(widget->parent->parent->first_child)->selection_index;
    if (selection < 0) {
        selection = 0;
    } else if (selection > 4) {
        selection = 4;
    }
    ((struct saved_player_profile *)profile)->connection_type = (uint8_t)selection;
    ((struct saved_player_profile *)profile)->server_port = (uint16_t)network_game_option_a_00719210;
    ((struct saved_player_profile *)profile)->client_port = (uint16_t)network_game_option_b_00719214;
    return 1;
}

/**
 * Slot 186 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3a70
 */
uint8_t UiEventHandlers::event_4a3a70(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *second = widget->parent->parent->first_child->next_sibling;
    widget_instance *third;
    uint8_t result = 0;

    if (second->parent->focused_child == second) {
        halo::text::string_format_wide_va(network_host_number_text_0071921c, (const uint16_t *)L"%d", network_game_option_a_00719210);
        if (halo::interface::virtual_keyboard_open(network_host_number_text_0071921c, 0x10, 0xd) != 0) {
            network_host_edit_field_00719410 = 4;
            network_host_number_field_00719218 = 1;
            result = 1;
        }
    }
    third = second->next_sibling;
    if (result == 0 && third->parent->focused_child == third) {
        halo::text::string_format_wide_va(network_host_number_text_0071921c, (const uint16_t *)L"%d", network_game_option_b_00719214);
        if (halo::interface::virtual_keyboard_open(network_host_number_text_0071921c, 0x10, 0xd) != 0) {
            network_host_edit_field_00719410 = 5;
            network_host_number_field_00719218 = 2;
            return 1;
        }
    }
    return result;
}

/**
 * Slot 171 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a3d40
 */
uint8_t UiEventHandlers::event_4a3d40(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    hud_text_message_queue.element_size = -1;
    hud_text_message_queue.count = -1;
    if (hud_text_message_queue.data != 0) {
        GlobalFree(hud_text_message_queue.data);
        hud_text_message_queue.data = 0;
    }
    if (hud_text_message_cycle_state_00719230 > 0) {
        halo::interface::chimera__main_menu_music((uint8_t)(hud_text_message_cycle_state_00719230 == 2));
    }
    hud_text_message_cycle_state_00719230 = 0;
    return 1;
}

}
