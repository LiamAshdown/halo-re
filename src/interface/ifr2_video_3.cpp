#include "halo/interface/ifr2_video.hpp"
#include "halo/text/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

#ifdef interface
#undef interface
#endif

static auto &video_gamma_current = halo::link::ref<uint8_t>(halo::ui::vars().video_gamma_current);
static auto &video_resolutions = halo::link::ref<video_resolution [0x20]>(halo::ui::vars().video_resolutions);
static auto &video_resolution_count = halo::link::ref<int32_t>(halo::ui::vars().video_resolution_count);

namespace halo::interface {

/**
 * @address 0x4bb5e0
 */
uint8_t VideoOptions::reset_to_defaults(widget_instance *button)
{
    saved_player_profile profile;
    uint8_t result = halo::saved_games::player_profile_set_default_video_options(&profile, 0);

    if (result != 0) {
        profile.gamma = video_gamma_current;
        halo::interface::video_options_menu_populate(button->parent->parent, &profile);
        halo::interface::widget_play_sound_effect(2);
    }
    return result;
}

/**
 * blam-cc: height -> EAX
 *
 * @address 0x4badc0
 */
void VideoOptions::resolution_add(int32_t height, int32_t width, int32_t refresh_rate)
{
    static const uint16_t name_format[8] = {'%', 'd', ' ', 'x', ' ', '%', 'd', 0};
    video_resolution *entry;
    int32_t index = -1;
    int32_t i;
    uint32_t j;

    for (i = 0; i < video_resolution_count; i++) {
        if (video_resolutions[i].width == width && video_resolutions[i].height == height) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        if (video_resolution_count >= 0x20) {
            return;
        }
        index = video_resolution_count++;
    }

    entry = &video_resolutions[index];
    entry->width = width;
    entry->height = height;
    halo::text::string_format_wide_va(entry->name, name_format, width, height);
    entry->name[15] = 0;
    for (j = 0; j < entry->refresh_rate_count; j++) {
        if (entry->refresh_rates[j] == refresh_rate) {
            return;
        }
    }
    if (entry->refresh_rate_count < 8) {
        entry->refresh_rates[entry->refresh_rate_count] = refresh_rate;
        entry->refresh_rate_count++;
    }
}

} // namespace halo::interface

namespace halo::interface {

uint8_t video_options_reset_to_defaults(widget_instance *button)
{
    return halo::interface::VideoOptions::reset_to_defaults(button);
}

void video_resolution_add(int32_t height, int32_t width, int32_t refresh_rate)
{
    halo::interface::VideoOptions::resolution_add(height, width, refresh_rate);
}

}
