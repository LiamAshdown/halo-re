#include "win32.h"
#include "halo/interface/engine_state.hpp"
#include "halo/interface/ifr2_players.hpp"
#include "rasterizer.h"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/shell/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern int32_t safe_mode;
extern int16_t renderer_texture_quality;
extern int32_t game_time_force_single_tick;
extern uint8_t rasterizer_fullscreen;
extern void *rasterizer_device;
extern uint8_t rasterizer_needs_reset;
extern uint8_t rasterizer_desktop_display_mode[];
extern uint8_t console_debug_toggle_6893f7;
extern uint8_t console_debug_toggle_6893f6;
extern uint8_t console_debug_toggle_6893fa;
extern uint32_t rasterizer_device_version;
extern uint8_t console_debug_toggle_6893f2;
extern uint32_t rasterizer_capability_007c10e4;
extern int16_t light_count_enabled;
extern uint8_t console_debug_toggle_689404;
extern uint8_t particle_systems_enabled;
extern int32_t rasterizer_gamma_exponent;
extern uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested);
extern void rasterizer_build_present_parameters(void *dest, rasterizer_display_mode *source);
extern uint8_t rasterizer_device_reset(void *present_parameters);
extern void rasterizer_resize_game_window(int32_t height, int32_t width);
extern void chimera__gamma(void);
}

typedef int32_t (__stdcall *d3d_get_display_mode_fn)(void *device, uint32_t swap_chain, void *mode);

namespace halo::interface {

/**
 * blam-cc: EAX -> settings Applies the profile's video options (settings is the profile's settings block): the
 * present mode from +0xa74, the display mode from +0xa68..+0xa6f (resetting the device when it differs), and
 * the detail toggles and gamma from +0xa70..+0xa76. Flushes the texture cache when the present mode changed.
 *
 * @address 0x495580
 */
uint8_t PlayerProfiles::apply_video_options(uint8_t *settings)
{
    rasterizer_display_mode mode;
    win32_rect desktop;
    uint8_t present_parameters[0x38];
    uint8_t mode_changed = 0;
    int32_t reset = 0;
    int16_t new_mode;
    uint8_t value;

    if (safe_mode != 0) {
        settings[0xa70] = 0;
        settings[0xa71] = 0;
        settings[0xa72] = 0;
        settings[0xa74] = 0;
        settings[0xa73] = 0;
    }
    switch (settings[0xa74]) {
    case 0: new_mode = 2; break;
    case 1: new_mode = 1; break;
    case 2: new_mode = 0; break;
    default: new_mode = -1; break;
    }
    if (new_mode >= 0) {
        mode_changed = renderer_texture_quality != new_mode;
        renderer_texture_quality = new_mode;
    }
    state::object_lod_quality = 2;

    mode.width = *(int16_t *)(settings + 0xa68);
    mode.height = *(int16_t *)(settings + 0xa6a);
    mode.refresh_rate = *(int16_t *)(settings + 0xa6c);
    mode.vsync = settings[0xa6f] != 0;
    state::frame_rate_limiter_enabled = game_time_force_single_tick != 0 ? 0 : settings[0xa6f] == 2;

    if (rasterizer_fullscreen == 0 || rasterizer_device == 0) {
        GetWindowRect(GetDesktopWindow(), &desktop);
        if ((uint32_t)mode.height >= (uint32_t)desktop.bottom || (uint32_t)mode.width >= (uint32_t)desktop.right) {
            if (desktop.bottom > 600) {
                mode.width = 800;
                mode.height = 600;
            } else {
                mode.width = 640;
                mode.height = 480;
            }
        }
    }
    if (rasterizer_needs_reset == 0 && rasterizer_display_mode_differs(&mode)) {
        void **vtable;

        rasterizer_build_present_parameters(present_parameters, &mode);
        rasterizer_device_reset(present_parameters);
        vtable = *(void ***)rasterizer_device;
        ((d3d_get_display_mode_fn)vtable[0x20 / 4])(rasterizer_device, 0, rasterizer_desktop_display_mode);
        reset = 1;
        rasterizer_resize_game_window(mode.height, mode.width);
        rasterizer_needs_reset = 0;
    }

    value = halo::shell::globals().disable_specular != 0 ? 0 : settings[0xa70];
    console_debug_toggle_6893f7 = value;
    console_debug_toggle_6893f6 = value;
    console_debug_toggle_6893fa = value;
    console_debug_toggle_6893f2 = rasterizer_device_version < 0xffff0101u ? 0 : settings[0xa71];
    light_count_enabled = 2;
    state::decals_and_lens_flares_enabled = 1;
    console_debug_toggle_689404 = 1;
    halo::effects::globals().decals_for_all_responses = (rasterizer_capability_007c10e4 & 0x6000000u) != 0 ? settings[0xa72] : 0;
    halo::effects::globals().particle_spawn_debug_mode = settings[0xa73];
    particle_systems_enabled = settings[0xa73];
    rasterizer_gamma_exponent = settings[0xa76];
    chimera__gamma();

    if (mode_changed) {
        halo::cache::globals().texture_cache->age = halo::cache::globals().texture_cache->age + 1;
        halo::memory::cache_flush(halo::cache::globals().texture_cache);
    }
    return reset >= 0;
}

} // namespace halo::interface

extern "C" {

uint8_t player_profile_apply_video_options(uint8_t *settings)
{
    return halo::interface::PlayerProfiles::apply_video_options(settings);
}

}
