#include "halo/interface/ifr2_video.hpp"
#include "halo/text/api.hpp"
#include "crt.h"
#include <wchar.h>
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/interface/constants.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t video_force_mode_flag;
extern int32_t os_platform;
extern int32_t os_platform_refresh_default;
extern video_resolution video_resolutions[0x20];
extern int32_t video_resolution_count;
extern uint32_t rasterizer_device;
extern uint32_t rasterizer_device_version;
extern uint32_t rasterizer_capability_007c10e4;
extern int32_t video_gamma_setting;
extern heap *widget_memory_pool;
}

namespace halo::interface {

/**
 * @address 0x4baec0
 */
void VideoOptions::populate(uint8_t *context, uint8_t *settings)
{
    int32_t target_refresh;
    uint8_t *resolution_field;
    uint8_t *resolution_field2;
    uint8_t *refresh_field;
    int32_t resolution_index;
    uint32_t refresh_index;
    int i;
    uint8_t *node;

    if (video_force_mode_flag == 0) {
        target_refresh = ((struct saved_player_profile *)settings)->refresh_rate;
    } else {
        if (os_platform == 0) {
            halo::shell::os_platform_identify();
        }
        target_refresh = os_platform_refresh_default;
        if (os_platform < 3) {
            target_refresh = 0x3c;
        }
    }

    resolution_field = *(uint8_t **)(*(uint8_t **)(context + 0x34) + 0x2c);
    resolution_field2 = *(uint8_t **)(*(uint8_t **)(*(uint8_t **)(context + 0x34) + 0x34) + 0x2c);
    refresh_field = *(uint8_t **)(*(uint8_t **)(resolution_field + 0x34) + 0x2c);

    halo::interface::video_resolution_list_build();

    resolution_index = -1;
    for (i = 0; i < video_resolution_count; i++) {
        if (video_resolutions[i].width == ((struct saved_player_profile *)settings)->screen_width &&
            video_resolutions[i].height == ((struct saved_player_profile *)settings)->screen_height) {
            resolution_index = i;
            break;
        }
    }
    if (resolution_index == -1) {
        for (i = 0; i < video_resolution_count; i++) {
            if (video_resolutions[i].width == halo::interface::k_base_screen_width && video_resolutions[i].height == halo::interface::k_base_screen_height) {
                resolution_index = i;
                break;
            }
        }
    }

    refresh_index = 0xffffffffu;
    if (resolution_index >= 0 && resolution_index < video_resolution_count) {
        for (i = 0; i < (int32_t)video_resolutions[resolution_index].refresh_rate_count; i++) {
            if (video_resolutions[resolution_index].refresh_rates[i] == target_refresh) {
                refresh_index = (uint32_t)i;
                break;
            }
        }
    }
    if (refresh_index == 0xffffffffu) {
        refresh_index = halo::interface::video_refresh_rate_find_index(resolution_index, 0x3c);
    }

    *(video_resolution **)(resolution_field2 + 0x44) = video_resolutions;
    *(int16_t *)(resolution_field2 + 0x40) = (int16_t)resolution_index;
    *(int16_t *)(resolution_field2 + 0x48) = (int16_t)video_resolution_count;

    *(video_resolution **)(refresh_field + 0x44) = video_resolutions;
    *(int16_t *)(refresh_field + 0x48) = (int16_t)video_resolutions[0].refresh_rate_count;
    if (os_platform == 0) {
        halo::shell::os_platform_identify();
    }
    if (os_platform < 3) {
        *(int16_t *)(refresh_field + 0x40) = 0;
        (*(uint8_t **)(refresh_field + 0x30))[0x12] = 1;
        *(uint32_t *)(*(uint8_t **)(refresh_field + 0x30) + 0x24) = 0x3eaa7efa;
    } else if (video_force_mode_flag == 0 && halo::rasterizer::globals().fullscreen != 0 && rasterizer_device != 0) {
        *(int16_t *)(refresh_field + 0x40) = (int16_t)refresh_index;
        (*(uint8_t **)(refresh_field + 0x30))[0x12] = 0;
        *(uint32_t *)(*(uint8_t **)(refresh_field + 0x30) + 0x24) = 0x3f800000;
    } else {
        *(int16_t *)(refresh_field + 0x40) = (int16_t)refresh_index;
        (*(uint8_t **)(refresh_field + 0x30))[0x12] = 1;
        *(uint32_t *)(*(uint8_t **)(refresh_field + 0x30) + 0x24) = 0x3eaa7efa;
    }

    {
        uint8_t *base = *(uint8_t **)(resolution_field + 0x2c);

        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (((struct saved_player_profile *)settings)->frame_rate_mode < 3) ? ((struct saved_player_profile *)settings)->frame_rate_mode : 2;

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(int8_t *)(settings + 0xa70) != 0) ? 1 : 0;
        if (rasterizer_device_version < 0xffff0101u || halo::shell::globals().disable_specular != 0) {
            *(uint16_t *)(node + 0x40) = 0;
            base[0x12] = 1;
            *(uint32_t *)(base + 0x24) = 0x3eaa7efa;
        } else {
            base[0x12] = 0;
            *(uint32_t *)(base + 0x24) = 0x3f800000;
        }

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(int8_t *)(settings + 0xa71) != 0) ? 1 : 0;
        if (rasterizer_device_version < 0xffff0101u) {
            *(uint16_t *)(node + 0x40) = 0;
            base[0x12] = 1;
            *(uint32_t *)(base + 0x24) = 0x3eaa7efa;
        } else {
            base[0x12] = 0;
            *(uint32_t *)(base + 0x24) = 0x3f800000;
        }

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (*(int8_t *)(settings + 0xa72) != 0) ? 1 : 0;
        if ((rasterizer_capability_007c10e4 & 0x6000000) == 0) {
            *(uint16_t *)(node + 0x40) = 0;
            base[0x12] = 1;
            *(uint32_t *)(base + 0x24) = 0x3eaa7efa;
        } else {
            base[0x12] = 0;
            *(uint32_t *)(base + 0x24) = 0x3f800000;
        }

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (((struct saved_player_profile *)settings)->particles < 3) ? ((struct saved_player_profile *)settings)->particles : 2;

        base = *(uint8_t **)(base + 0x2c);
        for (node = *(uint8_t **)(base + 0x34); node != 0 && *(int16_t *)(node + 0xe) != 2; node = *(uint8_t **)(node + 0x2c)) {}
        *(uint16_t *)(node + 0x40) = (((struct saved_player_profile *)settings)->texture_quality < 3) ? ((struct saved_player_profile *)settings)->texture_quality : 2;

        base = *(uint8_t **)(base + 0x2c);
        (void)base;
    }

    {
        uint8_t gamma = *(uint8_t *)(settings + 0xa76);
        if (gamma == 0) {
            video_gamma_setting = 1;
            halo::rasterizer::globals().gamma_exponent = 1;
        } else if (gamma == 0xff) {
            video_gamma_setting = 0xfe;
            halo::rasterizer::globals().gamma_exponent = 0xfe;
        } else {
            video_gamma_setting = gamma;
            halo::rasterizer::globals().gamma_exponent = gamma;
        }
        halo::rasterizer::chimera__gamma();
    }
}

/**
 * @address 0x4bb640
 */
uint8_t VideoOptions::update(widget_instance *screen)
{
    static const uint16_t empty_text[1] = {0};
    static const uint16_t hz_format[6] = {'%', 'd', ' ', 'H', 'z', 0};
    widget_instance *row_a = screen->first_child->next_sibling;
    widget_instance *resolution = screen->first_child->first_child->next_sibling;
    widget_instance *refresh = row_a->first_child->next_sibling;
    widget_instance *gamma;
    int32_t resolution_index;
    int32_t refresh_index;
    uint16_t *text;
    int32_t i;

    resolution->parent->focused_child = resolution;
    refresh->parent->focused_child = refresh;

    resolution_index = resolution->selection_index;
    text = (uint16_t *)halo::memory::heap_reallocate(resolution->list_render_data, 0x20, widget_memory_pool);
    resolution->list_render_data = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text,
                (const wchar_t *)(resolution_index >= 0 && resolution_index < video_resolution_count
                                      ? video_resolutions[resolution_index].name
                                      : empty_text),
                0xf);
        ((uint16_t *)resolution->list_render_data)[0xf] = 0;
    }

    refresh_index = refresh->selection_index;
    refresh->item_count = (uint16_t)video_resolutions[resolution_index].refresh_rate_count;
    if ((uint32_t)refresh_index >= video_resolutions[resolution_index].refresh_rate_count) {
        refresh->selection_index = (int16_t)(video_resolutions[resolution_index].refresh_rate_count - 1);
        refresh_index = refresh->selection_index;
    }
    text = (uint16_t *)halo::memory::heap_reallocate(refresh->list_render_data, 0x20, widget_memory_pool);
    refresh->list_render_data = text;
    if (text != 0) {
        halo::text::string_format_wide_va(text, hz_format, video_resolutions[resolution_index].refresh_rates[refresh_index]);
        ((uint16_t *)refresh->list_render_data)[0xf] = 0;
    }

    gamma = row_a;
    for (i = 0; i < 7; i++) {
        gamma = gamma->next_sibling;
    }
    gamma = gamma->first_child;
    while (gamma != 0 && gamma->widget_type != uiwidgettype_spinner_list) {
        gamma = gamma->next_sibling;
    }
    if (gamma->selection_direction == -1) {
        video_gamma_setting -= 5;
        if (video_gamma_setting < 1) {
            halo::interface::widget_play_sound_effect(4);
            video_gamma_setting = 1;
        }
    } else if (gamma->selection_direction == 1) {
        video_gamma_setting += 5;
        if (video_gamma_setting > 0xfe) {
            halo::interface::widget_play_sound_effect(4);
            video_gamma_setting = 0xfe;
        }
    }
    halo::rasterizer::globals().gamma_exponent = video_gamma_setting;
    halo::rasterizer::chimera__gamma();
    halo::interface::widget_extended_description_sync_selection(screen);
    return 1;
}

/**
 * @address 0x4bab80
 */
uint32_t VideoOptions::refresh_rate_compare(const uint32_t *a, const uint32_t *b)
{
    if (*a < *b) {
        return 0xffffffffu;
    }
    return (uint32_t)(*b < *a);
}

/**
 * blam-cc: resolution_index -> ECX, refresh_rate -> EDI
 *
 * @address 0x4bae80
 */
uint32_t VideoOptions::refresh_rate_find_index(int32_t resolution_index, int32_t refresh_rate)
{
    uint32_t i;

    if (resolution_index < 0 || resolution_index >= video_resolution_count) {
        return 0xffffffffu;
    }
    if (video_resolutions[resolution_index].refresh_rate_count == 0) {
        return 0xffffffffu;
    }
    for (i = 0; i < (uint32_t)video_resolutions[resolution_index].refresh_rate_count; i++) {
        if (video_resolutions[resolution_index].refresh_rates[i] == refresh_rate) {
            return i;
        }
    }
    return 0xffffffffu;
}

/**
 * @address 0x4bab50
 */
int VideoOptions::resolution_compare(const video_resolution *a, const video_resolution *b)
{
    uint32_t aw = (uint32_t)a->width, bw = (uint32_t)b->width;
    uint32_t ah = (uint32_t)a->height, bh = (uint32_t)b->height;

    if (aw < bw) {
        return -1;
    }
    if (aw <= bw) {
        if (ah < bh) {
            return -1;
        }
        if (ah <= bh) {
            return 0;
        }
    }
    return 1;
}

/**
 * @address 0x4bad40
 */
void VideoOptions::resolution_list_build()
{
    int i;

    video_resolution_count = 0;
    memset(video_resolutions, 0, sizeof(video_resolutions));

    halo::interface::video_display_modes_enumerate(0x16);
    qsort(video_resolutions, (uint32_t)video_resolution_count, sizeof(video_resolution),
          (int (__cdecl *)(const void *, const void *))halo::interface::video_resolution_compare);

    for (i = 0; i < video_resolution_count; i++) {
        qsort(video_resolutions[i].refresh_rates, video_resolutions[i].refresh_rate_count,
              sizeof(int32_t), (int (__cdecl *)(const void *, const void *))halo::interface::video_refresh_rate_compare);
    }
}

} // namespace halo::interface

namespace halo::interface {

void video_options_menu_populate(uint8_t *context, uint8_t *settings)
{
    halo::interface::VideoOptions::populate(context, settings);
}

uint8_t video_options_menu_update(widget_instance *screen)
{
    return halo::interface::VideoOptions::update(screen);
}

uint32_t __cdecl video_refresh_rate_compare(const uint32_t *a, const uint32_t *b)
{
    return halo::interface::VideoOptions::refresh_rate_compare(a, b);
}

uint32_t video_refresh_rate_find_index(int32_t resolution_index, int32_t refresh_rate)
{
    return halo::interface::VideoOptions::refresh_rate_find_index(resolution_index, refresh_rate);
}

int __cdecl video_resolution_compare(const video_resolution *a, const video_resolution *b)
{
    return halo::interface::VideoOptions::resolution_compare(a, b);
}

void __cdecl video_resolution_list_build(void)
{
    halo::interface::VideoOptions::resolution_list_build();
}

}
