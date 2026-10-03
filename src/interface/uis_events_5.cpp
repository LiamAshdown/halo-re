/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget instance,
 * the event record and an out-flag and returns whether the event was consumed.
 */

#include "tags.h"
#include "halo/interface/engine_state.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <stdio.h>
#include "objects.h"
#include "units.h"
#include <string.h>
#include "rasterizer.h"

#include "halo/interface/uis_event_handlers.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "saved_games.h"
#include "halo/interface/constants.hpp"
#include "halo/interface/wide_text.hpp"

extern "C" {
extern int16_t pending_difficulty;
extern void widget_play_sound_effect(int16_t effect_id);
extern uint32_t ui_restart_saved_game(void);
extern uint8_t ui_restoring_previous_widget;
extern char *campaign_level_paths[];
extern uint8_t network_wait_flag_00719739;
extern growable_array ui_lists[3];
extern int32_t ui_list_current;
extern uint8_t ui_list_has_default;
extern char pending_delete_saved_game_name_00718fd0[];
extern uint8_t autopatch_status_state_00719234;
extern uint16_t network_host_name_field_00719238[32];
extern uint16_t network_host_subname_007191f0[9];
extern int32_t network_host_edit_field_00719410;
extern int32_t controls_capture_row;
extern uint8_t controls_menu_list_mode;
extern int32_t selected_saved_item;
extern saved_player_profile saved_item_working_copy;
extern controls_edit_buffer input_controls_live_006b3a48;
extern uint8_t ui_flag_00719444;
extern int32_t controls_device_label_count;
extern uint8_t controls_device_labels[];
extern uint8_t controls_input_capture_flags;
extern uint8_t controls_input_capture_buffer[0xa0 * 4];
extern int32_t controls_selected_device;
extern uint8_t controls_device_sensitivity_a[];
extern uint8_t controls_device_sensitivity_b[];
extern uint8_t ui_flag_007196d1;
extern uint8_t ui_flag_007196d2;
extern int32_t video_resolution_count;
extern video_resolution video_resolutions[0x20];
extern int32_t video_gamma_setting;
extern rasterizer_display_mode ui_video_requested_display_mode_006b7010;
extern d3d_display_mode rasterizer_desktop_display_mode;
extern float sound_master_gain;
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static void show(widget_instance *child, uint8_t visible)
{
    if (visible) {
        child->scale = 1.0f;
        child->hidden = 0;
    } else {
        child->scale = halo::interface::k_widget_default_scale;
        child->hidden = 1;
    }
}

/** Local helper shared by the handlers of this file. */
static void *list_item_data(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].data;
    }
    return 0;
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

}

/**
 * Slot 169 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4110
 */
uint8_t UiEventHandlers::event_4a4110(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *parent = widget->parent;
    widget_instance *child = parent->first_child;
    int16_t *committed = &halo::interface::widget_list_committed(parent);
    int32_t i;

    for (i = 0; child != widget; i++) {
        if (i + 1 >= 4) {
            return 1;
        }
        child = child->next_sibling;
    }
    if (*committed == i) {
        if (i < 4) {
            if (i >= 0) {
                pending_difficulty = (int16_t)i;
            }
            halo::interface::widget_play_sound_effect(2);
        }
        halo::interface::ui_restart_saved_game();
    }
    *committed = (int16_t)i;
    return 1;
}

/**
 * Slot 172 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4190
 */
uint8_t UiEventHandlers::event_4a4190(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    halo::networking::autopatch_launch_updater();
    return 1;
}

/**
 * Slot 174 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a41a0
 */
uint8_t UiEventHandlers::event_4a41a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t has_save = halo::saved_games::saved_game_file_exists((char *)"savegame");
    uint8_t has_checkpoints = (uint8_t)(halo::saved_games::game_checkpoint_enumerate_files(1, 1, 0, 0) > 0);
    widget_instance *child = widget->first_child;

    show(child, has_save);
    if (!has_save) {
        widget->focused_child = child->next_sibling;
    }
    child = child->next_sibling->next_sibling;
    show(child, has_checkpoints);
    if (!has_checkpoints) {
        widget->focused_child = child->next_sibling;
    }
    if (has_save || has_checkpoints) {
        return 1;
    }
    if (ui_restoring_previous_widget != 0) {
        halo::interface::widget_instance_close_and_restore_previous(widget);
        return 1;
    }
    halo::main::main_queue_map_change(campaign_level_paths[0]);
    network_wait_flag_00719739 = 0;
    return (uint8_t)(ui_restoring_previous_widget != 0);
}

/**
 * Slot 175 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4270
 */
uint8_t UiEventHandlers::event_4a4270(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::saved_games::saved_game_load_checkpoint_by_name((char *)"savegame");
}

/**
 * Slot 176 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a44f0
 */
uint8_t UiEventHandlers::event_4a44f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t count;
    int32_t i;

    for (i = 0; i < 3; i++) {
        ui_lists[i].element_size = 0x10;
        ui_lists[i].count = 0;
        ui_lists[i].data = 0;
    }
    ui_list_current = -1;
    ui_list_has_default = 0;
    count = halo::saved_games::game_checkpoint_enumerate_files(1, 1, (checkpoint_enumerate_proc)((void *)halo::interface::checkpoint_list_add_row), 0);
    pending_delete_saved_game_name_00718fd0[0] = 0;
    if (count == 0) {
        halo::interface::widget_instance_close_and_restore_previous(widget);
    }
    return (uint8_t)(count != 0);
}

/**
 * Slot 177 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4570
 */
uint8_t UiEventHandlers::event_4a4570(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    halo::interface::ui_list_free_all();
    return 1;
}

/**
 * Slot 188 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4580
 */
uint8_t UiEventHandlers::event_4a4580(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *data = (uint8_t *)list_item_data(halo::interface::widget_list_committed(widget));

    sprintf(pending_delete_saved_game_name_00718fd0, "checkpoints\\%s", (char *)(data + 0x48));
    return 1;
}

/**
 * Slot 189 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a45d0
 */
uint8_t UiEventHandlers::event_4a45d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (pending_delete_saved_game_name_00718fd0[0] != 0) {
        halo::saved_games::saved_game_delete_files(pending_delete_saved_game_name_00718fd0);
    }
    return 1;
}

/**
 * Slot 178 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a45f0
 */
uint8_t UiEventHandlers::event_4a45f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *data = (uint8_t *)list_item_data(halo::interface::widget_list_committed(widget));
    char name[0x40];

    sprintf(name, "checkpoints\\%s", (char *)(data + 0x48));
    return halo::saved_games::saved_game_load_checkpoint_by_name(name);
}

/**
 * Slot 179 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a47b0
 */
uint8_t UiEventHandlers::event_4a47b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    halo::saved_games::game_checkpoint_save_new();
    return 1;
}

/**
 * Slot 182 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4870
 */
uint8_t UiEventHandlers::event_4a4870(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return autopatch_status_state_00719234;
}

/**
 * Slot 185 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a4af0
 */
uint8_t UiEventHandlers::event_4a4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *first = widget->parent->parent->first_child;

    if (first == widget->parent && halo::interface::virtual_keyboard_open(network_host_name_field_00719238, 0x40, 0xd) != 0) {
        network_host_edit_field_00719410 = 4;
        return 1;
    }
    if (first->next_sibling == widget->parent && halo::interface::virtual_keyboard_open(network_host_subname_007191f0, 0x12, 0xc) != 0) {
        network_host_edit_field_00719410 = 0;
        return 1;
    }
    return 0;
}

/**
 * Slot 113 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b4980
 */
uint8_t UiEventHandlers::event_4b4980(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *second = widget->first_child->next_sibling;
    widget_instance *third = second->next_sibling;
    controls_edit_buffer *live = &input_controls_live_006b3a48;
    saved_player_profile *profile;
    widget_instance *list;

    controls_capture_row = -1;
    third->state = 0;
    third->hidden = 1;
    widget->focused_child = second;
    second->state = 1;
    second->hidden = 0;
    controls_menu_list_mode = 0;
    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    profile = &saved_item_working_copy;
    memcpy(live->keyboard, profile->keyboard_bindings, sizeof(live->keyboard));
    memcpy(live->mouse_button, profile->mouse_button_bindings, sizeof(live->mouse_button));
    memcpy(live->mouse_axis, profile->mouse_axis_bindings, sizeof(live->mouse_axis));
    memcpy(live->gamepad_button, profile->gamepad_button_bindings, sizeof(live->gamepad_button));
    memcpy(live->gamepad_action_button, profile->gamepad_action_buttons, sizeof(live->gamepad_action_button));
    memcpy(live->gamepad_axis, profile->gamepad_axis_bindings, sizeof(live->gamepad_axis));
    memcpy(live->gamepad_pov, profile->gamepad_pov_bindings, sizeof(live->gamepad_pov));
    ui_flag_00719444 = 0;
    memcpy(live->gamepad_rate_a, profile->gamepad_rate_a, sizeof(live->gamepad_rate_a));
    memcpy(live->gamepad_rate_b, profile->gamepad_rate_b, sizeof(live->gamepad_rate_b));
    halo::interface::controls_build_device_label_table();
    list = second->first_child->first_child->next_sibling;
    list->selection_index = 0;
    list->item_count = (uint16_t)controls_device_label_count;
    list->list_items = controls_device_labels;
    halo::interface::controls_binding_list_refresh_rows(second, 0);
    return 1;
}

/**
 * Slot 126 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b4af0
 */
uint8_t UiEventHandlers::event_4b4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const controls_edit_buffer *live = &input_controls_live_006b3a48;
    widget_instance *list;

    if (ui_flag_00719444 == 0 && (selected_saved_item & 0xf) == 0) {
        saved_player_profile *profile = &saved_item_working_copy;

        memcpy(profile->keyboard_bindings, live->keyboard, sizeof(live->keyboard));
        memcpy(profile->mouse_button_bindings, live->mouse_button, sizeof(live->mouse_button));
        memcpy(profile->mouse_axis_bindings, live->mouse_axis, sizeof(live->mouse_axis));
        memcpy(profile->gamepad_button_bindings, live->gamepad_button, sizeof(live->gamepad_button));
        memcpy(profile->gamepad_action_buttons, live->gamepad_action_button, sizeof(live->gamepad_action_button));
        memcpy(profile->gamepad_axis_bindings, live->gamepad_axis, sizeof(live->gamepad_axis));
        memcpy(profile->gamepad_pov_bindings, live->gamepad_pov, sizeof(live->gamepad_pov));
        memcpy(profile->gamepad_rate_a, live->gamepad_rate_a, sizeof(live->gamepad_rate_a));
        memcpy(profile->gamepad_rate_b, live->gamepad_rate_b, sizeof(live->gamepad_rate_b));
    }
    list = widget->first_child->next_sibling->first_child->first_child->next_sibling;
    list->item_count = 0;
    list->list_items = 0;
    ui_flag_00719444 = 0;
    if (controls_capture_row != -1) {
        controls_input_capture_flags &= 0xf7;
        memset(controls_input_capture_buffer, 0, sizeof(controls_input_capture_buffer));
        controls_capture_row = -1;
    }
    return 1;
}

/**
 * Slot 127 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b4c40
 */
uint8_t UiEventHandlers::event_4b4c40(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_flag_00719444 = 1;
    return 1;
}

/**
 * Slot 115 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b52f0
 */
uint8_t UiEventHandlers::event_4b52f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->parent->first_child->next_sibling->next_sibling;
    widget_instance *second;
    widget_instance *third;
    int32_t i;

    for (i = 0; child != widget; i++) {
        if (i + 1 >= 8) {
            return 1;
        }
        child = child->next_sibling;
    }
    controls_capture_row = i;
    halo::input::Bindings::bind_scan_set_active(1);
    second = child->first_child->next_sibling;
    third = second->next_sibling;
    *((uint8_t *)second + 0x54) = 1;
    *((uint8_t *)third->first_child + 0x54) = 1;
    *((uint8_t *)third->first_child->next_sibling + 0x54) = 1;
    return 1;
}

/**
 * Slot 152 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b5350
 */
uint8_t UiEventHandlers::event_4b5350(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (controls_menu_list_mode != 0) {
        widget_instance *second = widget->first_child->next_sibling;
        widget_instance *third = second->next_sibling;

        third->state = 0;
        third->hidden = 1;
        widget->focused_child = second;
        second->state = 1;
        second->hidden = 0;
        controls_menu_list_mode = 0;
        return 1;
    }
    halo::interface::widget_instance_close_and_restore_previous(widget);
    *out_handled = 1;
    return 1;
}

/**
 * Slot 153 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b54a0
 */
uint8_t UiEventHandlers::event_4b54a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    halo::interface::controls_binding_rows_toggle_device_mode(widget->parent->parent->parent, 1);
    return 1;
}

/**
 * Slot 154 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4b54c0
 */
uint8_t UiEventHandlers::event_4b54c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *screen;
    widget_instance *second;
    widget_instance *third;

    if ((selected_saved_item & 0xf) == 0) {
        widget_instance *group = widget->parent->parent->first_child;
        int32_t device = controls_selected_device;

        controls_device_sensitivity_a[device] = (uint8_t)(first_list_child(group)->selection_index + 1);
        controls_device_sensitivity_b[device] = (uint8_t)(first_list_child(group->next_sibling)->selection_index + 1);
    }
    screen = widget->parent->parent->parent->parent;
    second = screen->first_child->next_sibling;
    third = second->next_sibling;
    third->state = 0;
    third->hidden = 1;
    screen->focused_child = second;
    second->state = 1;
    second->hidden = 0;
    controls_menu_list_mode = 0;
    return 1;
}

/**
 * Slot 114 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4bb290
 */
uint8_t UiEventHandlers::event_4bb290(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_flag_007196d1 = 0;
    if (ui_flag_007196d2 != 0) {
        widget_instance *root = widget;

        while (root->parent != 0) {
            root = root->parent;
        }
        root->milliseconds_to_auto_close = 1;
        root->milliseconds_auto_close_fade = 0;
        root->state = 0;
        ui_flag_007196d2 = 0;
        return 1;
    }
    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    halo::interface::video_options_menu_populate(widget, &saved_item_working_copy);
    return 1;
}

/**
 * Slot 173 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4bb300
 */
uint8_t UiEventHandlers::event_4bb300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *first = widget->first_child;

    if (ui_flag_007196d1 == 0) {
        saved_player_profile *profile = (selected_saved_item & 0xf) == 0 ? &saved_item_working_copy : nullptr;

        halo::rasterizer::globals().gamma_exponent = profile->gamma;
        halo::rasterizer::chimera__gamma();
    }
    first->first_child->next_sibling->list_items = 0;
    first->next_sibling->first_child->next_sibling->list_items = 0;
    return 1;
}

/**
 * Slot 125 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4bb360
 */
uint8_t UiEventHandlers::event_4bb360(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    saved_player_profile *profile = (selected_saved_item & 0xf) == 0 ? &saved_item_working_copy : nullptr;
    widget_instance *screen = widget->parent->parent;
    uint8_t result = 1;

    if (profile != 0) {
        widget_instance *group = screen->first_child;
        rasterizer_display_mode mode;
        int32_t resolution;
        int32_t refresh;

        resolution = first_list_child(group)->selection_index;
        if (resolution < 0 || resolution >= video_resolution_count) {
            resolution = 0;
        }
        profile->screen_width = (uint16_t)video_resolutions[resolution].width;
        profile->screen_height = (uint16_t)video_resolutions[resolution].height;
        group = group->next_sibling;
        refresh = first_list_child(group)->selection_index;
        if (refresh < 0 || (uint32_t)refresh >= video_resolutions[resolution].refresh_rate_count) {
            refresh = 0;
        }
        profile->refresh_rate = (uint16_t)video_resolutions[resolution].refresh_rates[refresh];
        group = group->next_sibling;
        profile->frame_rate_mode = clamp_selection(group, 2);
        group = group->next_sibling;
        profile->specular = (uint8_t)(first_list_child(group)->selection_index != 0);
        group = group->next_sibling;
        profile->shadows = (uint8_t)(first_list_child(group)->selection_index != 0);
        group = group->next_sibling;
        profile->decals = (uint8_t)(first_list_child(group)->selection_index != 0);
        group = group->next_sibling;
        profile->particles = clamp_selection(group, 2);
        group = group->next_sibling;
        profile->texture_quality = clamp_selection(group, 2);
        profile->gamma = (uint8_t)video_gamma_setting;
        mode.width = profile->screen_width;
        mode.height = profile->screen_height;
        mode.refresh_rate = profile->refresh_rate;
        mode.vsync = (uint8_t)(profile->frame_rate_mode != 0);
        result = halo::rasterizer::rasterizer_display_mode_differs(&mode);
        if (result == 0) {
            halo::interface::widget_instance_close_and_restore_previous(screen);
        }
    }
    halo::interface::widget_play_sound_effect(3);
    ui_flag_007196d1 = 1;
    return result;
}

/**
 * Slot 145 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4bb7e0
 */
uint8_t UiEventHandlers::event_4bb7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root;
    int32_t changed = -1;

    if ((selected_saved_item & 0xf) == 0) {
        saved_player_profile *profile = &saved_item_working_copy;
        float gain = sound_master_gain;
        rasterizer_display_mode mode;

        mode.width = profile->screen_width;
        mode.height = profile->screen_height;
        mode.refresh_rate = profile->refresh_rate;
        mode.vsync = (uint8_t)(profile->frame_rate_mode != 0);
        halo::rasterizer::display_mode_get_current(&ui_video_requested_display_mode_006b7010);
        halo::sound::sound_set_master_gain(0.05f);
        changed = 0;
        if (halo::rasterizer::rasterizer_display_mode_differs(&mode) != 0) {
            d3d_present_parameters parameters;

            halo::rasterizer::rasterizer_build_present_parameters(&parameters, &mode);
            halo::rasterizer::rasterizer_device_reset(&parameters);
            ((int32_t (__stdcall *)(void *, uint32_t, void *))(*(void ***)halo::rasterizer::globals().device)[0x20 / 4])(halo::rasterizer::globals().device, 0,
                &rasterizer_desktop_display_mode);
            changed = 1;
            halo::rasterizer::rasterizer_resize_game_window(mode.height, mode.width);
            halo::rasterizer::globals().needs_reset = 0;
        }
        halo::sound::sound_set_master_gain(gain);
        widget->creation_time = (int32_t)halo::cseries::time_query_performance_counter_ms();
    }
    ui_flag_007196d2 = 0;
    if (changed == 1) {
        if (halo::game::globals().time_force_single_tick != 0) {
            halo::interface::state::frame_rate_limiter_enabled = 0;
        } else {
            halo::interface::state::frame_rate_limiter_enabled = (uint8_t)(saved_item_working_copy.frame_rate_mode == 2);
        }
        return 1;
    }
    if (changed == 0) {
        ui_flag_007196d2 = 1;
    }
    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }
    root->milliseconds_to_auto_close = 1;
    root->milliseconds_auto_close_fade = 0;
    root->state = 0;
    return 0;
}

/**
 * Slot 146 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4bb970
 */
uint8_t UiEventHandlers::event_4bb970(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    float gain;
    saved_player_profile *profile;

    if (ui_flag_007196d2 != 0) {
        return 1;
    }
    gain = sound_master_gain;
    profile = (selected_saved_item & 0xf) == 0 ? &saved_item_working_copy : nullptr;
    profile->screen_width = (uint16_t)ui_video_requested_display_mode_006b7010.width;
    profile->refresh_rate = (uint16_t)ui_video_requested_display_mode_006b7010.refresh_rate;
    profile->screen_height = (uint16_t)ui_video_requested_display_mode_006b7010.height;
    if (ui_video_requested_display_mode_006b7010.vsync != 0) {
        profile->frame_rate_mode = (uint8_t)((halo::interface::state::frame_rate_limiter_enabled != 0) + 1);
    }
    if (halo::game::globals().time_force_single_tick != 0) {
        halo::interface::state::frame_rate_limiter_enabled = 0;
    } else {
        halo::interface::state::frame_rate_limiter_enabled = (uint8_t)(profile->frame_rate_mode == 2);
    }
    halo::sound::sound_set_master_gain(0.05f);
    if (halo::rasterizer::rasterizer_display_mode_differs(&ui_video_requested_display_mode_006b7010) != 0) {
        d3d_present_parameters parameters;

        halo::rasterizer::rasterizer_build_present_parameters(&parameters, &ui_video_requested_display_mode_006b7010);
        halo::rasterizer::rasterizer_device_reset(&parameters);
        ((int32_t (__stdcall *)(void *, uint32_t, void *))(*(void ***)halo::rasterizer::globals().device)[0x20 / 4])(halo::rasterizer::globals().device, 0,
            &rasterizer_desktop_display_mode);
        halo::rasterizer::rasterizer_resize_game_window(ui_video_requested_display_mode_006b7010.height, ui_video_requested_display_mode_006b7010.width);
        halo::rasterizer::globals().needs_reset = 0;
    }
    halo::sound::sound_set_master_gain(gain);
    return 1;
}

/**
 * Slot 147 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4bba80
 */
uint8_t UiEventHandlers::event_4bba80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_flag_007196d2 = 1;
    return 1;
}

}
