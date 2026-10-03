#include "halo/interface/ifr1_audio_options_profile.hpp"
#include "halo/sound/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &saved_item_working_copy = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().saved_item_working_copy);

static widget_instance *find_row_control(widget_instance *row)
{
    widget_instance *control;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    return control;
}

static int32_t clamp_slider(int16_t selection)
{
    if (selection < 0) {
        return 0;
    }
    if (selection < 0xb) {
        return selection;
    }
    return 10;
}

static void apply_gain(void (*setter)(float), uint8_t stored_value)
{
    float gain = (float)stored_value * 0.1f;

    if (gain < 0.0f) {
        setter(0.0f);
    } else if (gain <= 1.0f) {
        setter(gain);
    } else {
        setter(1.0f);
    }
}

namespace halo::interface {

/**
 * Original engine function audio_options_apply_from_profile; the author notes are in
 * docs/original/interface/audio_options_apply_from_profile.txt.
 *
 * @address 0x4a26a0
 */
uint32_t AudioOptionsProfile::apply_from_profile(widget_instance *widget)
{
    const saved_player_profile *record;
    widget_instance *row;
    widget_instance *control;

    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    record = &saved_item_working_copy;

    row = widget->first_child;
    control = find_row_control(row);
    if (clamp_slider(control->selection_index) != record->master_volume) {
        apply_gain(halo::sound::sound_set_master_gain, record->master_volume);
    }

    row = row->next_sibling;
    control = find_row_control(row);
    if (clamp_slider(control->selection_index) != record->effects_volume) {
        apply_gain(halo::sound::sound_set_effects_gain, record->effects_volume);
    }

    row = row->next_sibling;
    control = find_row_control(row);
    if (clamp_slider(control->selection_index) != record->music_volume) {
        apply_gain(halo::sound::sound_set_music_gain, record->music_volume);
    }
    return 1;
}

}
