/**
 * Assorted UI screen-level behaviour: cursor, error modal, pause check, colours and option application.
 */

#include "win32.h"
#include "halo/interface/engine_state.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "units.h"
#include "cutscene.h"
#include <string.h>

#include "halo/interface/uis_screens.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern int32_t chat_window_default_x;
extern int32_t chat_window_default_y;
extern int32_t chat_window_default_width;
extern int32_t chat_window_default_height;
extern int32_t chat_listbox_x;
extern int32_t chat_listbox_y;
extern int32_t chat_listbox_width;
extern int32_t chat_listbox_height;
extern int32_t chat_dialog_open;
extern int32_t chat_scope_active;
extern uint8_t ui_use_os_cursor;
extern int32_t previous_mouse_x;
extern int32_t previous_mouse_y;
extern int32_t ui_cursor_x;
extern int32_t ui_cursor_y;
extern int32_t mouse_device;
extern int32_t mouse_neutral_state[2];
extern int32_t live_mouse_state[2];
extern float cursor_sensitivity_x;
extern float cursor_sensitivity_y;
extern double cursor_sensitivity_curve_scale;
extern double cursor_sensitivity_curve_bias;
extern int32_t __ftol(double x);
extern uint8_t widget_memory_pool_valid;
extern widget_instance *ui_root_widget[1];
extern float ui_saved_color[3];
extern const ColorARGB *global_white_argb;
extern int32_t network_disabled_flag;
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static float find_row_spinner_gain(widget_instance *row)
{
    widget_instance *spinner = row->first_child;
    float value;
    while (spinner != 0 && spinner->widget_type != uiwidgettype_spinner_list) {
        spinner = spinner->next_sibling;
    }
    value = (float)(int16_t)spinner->selection_index * 0.1f;
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return value;
}

}

/**
 * Reads four rows of volume-slider widgets and pushes their (tenths, 0..10) values to the sound engine as 0..1
 * gains, then shows/hides a widget two siblings past the fourth row depending on whether that row's slider is at
 * zero, before refreshing the paired extended_description widget.
 *
 * @address 0x4a76d0
 */
void UiScreens::audio_options_apply_volume_sliders(widget_instance *widget)
{
    widget_instance *row;
    widget_instance *spinner;
    widget_instance *target;

    row = widget->first_child;
    halo::sound::sound_set_master_gain(find_row_spinner_gain(row));

    row = row->next_sibling;
    halo::sound::sound_set_effects_gain(find_row_spinner_gain(row));

    row = row->next_sibling;
    halo::sound::sound_set_music_gain(find_row_spinner_gain(row));

    row = row->next_sibling;
    spinner = row->first_child;
    while (spinner != 0 && spinner->widget_type != uiwidgettype_spinner_list) {
        spinner = spinner->next_sibling;
    }
    target = row->next_sibling->next_sibling;
    if (spinner->selection_index == 0) {
        target->hidden = 1;
        target->scale = 0.333f;
    } else {
        target->hidden = 0;
        target->scale = 1.0f;
    }
    halo::interface::widget_extended_description_sync_selection(widget);
}

/**
 * Resets the multiplayer chat window's screen position/size to its defaults and clears its message listbox.
 *
 * @address 0x4aa6b0
 */
void UiScreens::chat_window_reset_position(void)
{
    chat_listbox_x = chat_window_default_x;
    chat_listbox_y = chat_window_default_y;
    chat_dialog_open = 0;
    halo::interface::state::chat_window_unused_6b3914 = 0;
    chat_listbox_width = chat_window_default_width;
    chat_listbox_height = chat_window_default_height;
    halo::interface::state::chat_window_unused_6b38f4 = 0;
    chat_scope_active = -1;
    halo::interface::hud_chat_listbox_clear();
}

/**
 * Per-frame cursor position update: either reads the real OS cursor and turns it into a delta against last
 * frame's OS position, or (when the OS cursor is not in use) reads a raw two-axis device delta and runs each
 * axis through a quadratic sensitivity curve (`raw * (|raw| * sensitivity * curve_scale + curve_bias)`), then
 * applies the resulting delta to ui_cursor_x/ui_cursor_y via interface_update_for_resolution_change.
 *
 * Register convention: no register-passed arguments
 *
 * @address 0x4972c0
 */
void UiScreens::cursor_update(void)
{
    int32_t delta_x, delta_y;

    if (ui_use_os_cursor != 0) {
        win32_point point;

        GetCursorPos(&point);
        delta_x = previous_mouse_x - point.x;
        delta_y = previous_mouse_y - point.y;
        previous_mouse_x = point.x;
        previous_mouse_y = point.y;
    } else {
        int32_t *record;
        double raw_x, raw_y, scaled_x, scaled_y;

        record = live_mouse_state;
        if (mouse_device != 0) {
            record = mouse_neutral_state;
            if (halo::input::globals().suppressed == 0) {
                record = live_mouse_state;
            }
        }
        raw_x = (double)record[0];
        scaled_x = (raw_x < 0.0 ? -raw_x : raw_x) * (double)cursor_sensitivity_x * cursor_sensitivity_curve_scale +
            cursor_sensitivity_curve_bias;
        delta_x = __ftol(scaled_x * raw_x);
        raw_y = (double)record[1];
        scaled_y = (raw_y < 0.0 ? -raw_y : raw_y) * (double)cursor_sensitivity_y * cursor_sensitivity_curve_scale +
            cursor_sensitivity_curve_bias;
        delta_y = __ftol(scaled_y * raw_y);
    }
    halo::interface::interface_update_for_resolution_change(ui_cursor_x + delta_x, ui_cursor_y - delta_y);
}

/**
 * If there is no game engine active (or the player count is outside 2..3) and a certain block's flag byte (+9)
 * is clear: closes/reopens the error_modal widget if the current root widget is one (name matched via _strstr),
 * then calls ui_draw_filled_rectangle once or twice depending on local_player_globals::local_player_count (once
 * if > 1, twice if > 2) -- presumably showing the modal for each additional local player.
 *
 * Register convention: EAX -> packed_color, ECX -> rect (objdump call sites 0x494d28, 0x4973f9, 0x498617)
 *
 * @address 0x494ca0
 */
void UiScreens::error_modal_update(void)
{
    int16_t player_count_field;

    if ((halo::game::globals().current_engine == nullptr || (int32_t)halo::game::globals().state < 2 || (int32_t)halo::game::globals().state > 3) &&
        halo::cutscene::globals().cinematic_globals->in_progress == 0) {
        if (widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0) {
            strstr(ui_root_widget[0]->name, "error_modal");
        }
        player_count_field = *(int16_t *)((char *)halo::game::globals().local_player_globals + 0xc);
        if (player_count_field > 1) {
            Rectangle2D bar;

            bar.top = 0xef;
            bar.left = 0;
            bar.bottom = 0xf1;
            bar.right = 0x280;
            halo::interface::ui_draw_filled_rectangle(0xff000000, &bar);
            if (player_count_field > 2) {
                bar.top = (player_count_field == 3) ? 0xf0 : 0;
                bar.left = 0x13f;
                bar.bottom = 0x1e0;
                bar.right = 0x141;
                halo::interface::ui_draw_filled_rectangle(0xff000000, &bar);
            }
        }
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_get_saved_color.c.txt for the recovery notes.
 *
 * Register convention: EAX -> out
 *
 * @address 0x49c5c0
 */
ColorRGB * UiScreens::get_saved_color(ColorRGB *out)
{
    out->red = ui_saved_color[0];
    out->green = ui_saved_color[1];
    out->blue = ui_saved_color[2];
    return out;
}

/**
 * Original UI routine; see docs/original/interface/ui_get_saved_pulse_color.c.txt for the recovery notes.
 *
 * Register convention: EAX -> out
 *
 * @address 0x49c620
 */
ColorARGB * UiScreens::get_saved_pulse_color(ColorARGB *out)
{
    out->alpha = global_white_argb->alpha;
    out->red = ui_saved_color[0];
    out->green = ui_saved_color[1];
    out->blue = ui_saved_color[2];
    return out;
}

/**
 * Original UI routine; see docs/original/interface/ui_handler_4a68f0.c.txt for the recovery notes.
 *
 * @address 0x4a68f0
 */
void UiScreens::handler_4a68f0(uint8_t *widget)
{
    uint8_t *child = *(uint8_t **)(widget + 0x4c);
    uint8_t *record;

    ((struct widget_instance *)child)->background_bitmap_frame = *(int16_t *)(widget + 0x40);
    if (((struct widget_instance *)child)->background_bitmap_frame < 0) {
        ((struct widget_instance *)child)->background_bitmap_frame = 0;
    }
    if (network_disabled_flag != 0) {
        record = *(uint8_t **)(*(uint8_t **)(widget + 0x34) + 0x2c);
        record[0x12] = 1;
        *(uint32_t *)(record + 0x24) = 0x3eaa7efa;
    }
}

}
