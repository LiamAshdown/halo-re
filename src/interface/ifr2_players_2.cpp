#include "win32.h"
#include "halo/interface/engine_state.hpp"
#include "halo/rasterizer/globals.hpp"
#include "halo/interface/ifr2_players.hpp"
#include "rasterizer.h"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/shell/vars.hpp"
static auto &config_disable_specular = halo::link::ref<uint32_t>(halo::shell::vars().config_disable_specular);
#include "halo/interface/constants.hpp"
#include "halo/interface/com_object.hpp"

#ifdef interface
#undef interface
#endif

static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
static auto &renderer_texture_quality = halo::link::ref<int16_t>(halo::ui::vars().renderer_texture_quality);
static auto &rasterizer_desktop_display_mode = halo::link::ref<uint8_t []>(halo::ui::vars().rasterizer_desktop_display_mode);
static auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
static auto &rasterizer_capability_007c10e4 = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_capability_007c10e4);
static auto &light_count_enabled = halo::link::ref<int16_t>(halo::effects::vars().light_count_enabled);
static auto &particle_systems_enabled = halo::link::ref<uint8_t>(halo::effects::vars().particle_systems_enabled);

typedef int32_t (__stdcall *d3d_get_display_mode_fn)(void *device, uint32_t swap_chain, void *mode);

namespace halo::interface {

/**
 * blam-cc: EAX -> settings Applies the profile's video options (settings is the profile's settings block): the
 * present mode from +0xa74, the display mode from +0xa68..+0xa6f (resetting the device when it differs), and
 * the detail toggles and gamma from +0xa70..+0xa76. Flushes the texture cache when the present mode changed.
 *
 * @address 0x495580
 */
uint8_t PlayerProfiles::apply_video_options(saved_player_profile *settings)
{
    rasterizer_display_mode mode;
    win32_rect desktop;
    uint8_t present_parameters[0x38];
    uint8_t mode_changed = 0;
    int32_t reset = 0;
    int16_t new_mode;
    uint8_t value;

    if (safe_mode != 0) {
        settings->specular = 0;
        settings->shadows = 0;
        settings->decals = 0;
        settings->texture_quality = 0;
        settings->particles = 0;
    }
    switch (settings->texture_quality) {
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

    mode.width = settings->screen_width;
    mode.height = settings->screen_height;
    mode.refresh_rate = settings->refresh_rate;
    mode.vsync = settings->frame_rate_mode != 0;
    state::frame_rate_limiter_enabled = halo::game::globals().time_force_single_tick != 0 ? 0 : settings->frame_rate_mode == 2;

    if (halo::rasterizer::globals().fullscreen == 0 || halo::rasterizer::globals().device == 0) {
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
    if (halo::rasterizer::globals().needs_reset == 0 && halo::rasterizer::rasterizer_display_mode_differs(&mode)) {
        void **vtable;

        halo::rasterizer::rasterizer_build_present_parameters((d3d_present_parameters *)present_parameters, &mode);
        halo::rasterizer::rasterizer_device_reset((d3d_present_parameters *)present_parameters);
        vtable = halo::interface::com_vtable(halo::rasterizer::globals().device);
        ((d3d_get_display_mode_fn)vtable[0x20 / 4])(halo::rasterizer::globals().device, 0, rasterizer_desktop_display_mode);
        reset = 1;
        halo::rasterizer::rasterizer_resize_game_window(mode.height, mode.width);
        halo::rasterizer::globals().needs_reset = 0;
    }

    value = config_disable_specular != 0 ? 0 : settings->specular;
    halo::rasterizer::fields::specular_lightmap_enabled = value;
    halo::rasterizer::fields::specular_projected_light_enabled = value;
    halo::rasterizer::fields::specular_enabled = value;
    halo::rasterizer::fields::object_shadows_enabled = rasterizer_device_version < halo::interface::k_pixel_shader_version_1_1 ? 0 : settings->shadows;
    light_count_enabled = 2;
    state::decals_and_lens_flares_enabled = 1;
    halo::rasterizer::fields::detail_objects_enabled = 1;
    halo::effects::globals().decals_for_all_responses = (rasterizer_capability_007c10e4 & halo::interface::k_decal_capability_mask) != 0 ? settings->decals : 0;
    halo::effects::globals().particle_spawn_debug_mode = settings->particles;
    particle_systems_enabled = settings->particles;
    halo::rasterizer::globals().gamma_exponent = (uint8_t)settings->gamma;
    halo::rasterizer::chimera__gamma();

    if (mode_changed) {
        halo::cache::globals().texture_cache->age = halo::cache::globals().texture_cache->age + 1;
        halo::memory::cache_flush(halo::cache::globals().texture_cache);
    }
    return reset >= 0;
}

} // namespace halo::interface

namespace halo::interface {

uint8_t player_profile_apply_video_options(saved_player_profile *settings)
{
    return halo::interface::PlayerProfiles::apply_video_options(settings);
}

}
