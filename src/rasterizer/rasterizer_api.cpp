#include "halo/rasterizer/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/effects/api.hpp"
#include "halo/game/api.hpp"

static auto &rasterizer_device = halo::link::ref<void *>(halo::game::vars().rasterizer_device);
static auto &rasterizer_device_version = halo::link::ref<int32_t>(halo::ui::vars().rasterizer_device_version);
static auto &rasterizer_fullscreen = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_fullscreen);
static auto &rasterizer_gamma_exponent = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_gamma_exponent);
static auto &rasterizer_window_requested = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_window_requested);
static auto &rasterizer_default_z_near = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_default_z_near);
static auto &rasterizer_default_z_far = halo::link::ref<float>(halo::rasterizer::vars().rasterizer_default_z_far);
static auto &rasterizer_present_counter_high = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_present_counter_high);
static auto &rasterizer_present_counter_low = halo::link::ref<int32_t>(halo::rasterizer::vars().rasterizer_present_counter_low);
static auto &rasterizer_needs_reset = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_needs_reset);
static auto &rasterizer_vertex_buffer_lock_state = halo::link::ref<int16_t>(halo::rasterizer::vars().rasterizer_vertex_buffer_lock_state);
static auto &rasterizer_shader_file_name = halo::link::ref<char *>(halo::rasterizer::vars().rasterizer_shader_file_name);
static auto &rasterizer_dynamic_index_buffer = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_dynamic_index_buffer);
static auto &rasterizer_decal_vertex_cache_handle = halo::link::ref<uint32_t>(halo::effects::vars().rasterizer_decal_vertex_cache_handle);
static auto &rasterizer_render_states_dirty = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_render_states_dirty);
static auto &rasterizer_window = halo::link::ref<rasterizer_window_parameters>(halo::rasterizer::vars().rasterizer_window);
static auto &rasterizer_frame_statistics_state = halo::link::ref<rasterizer_frame_statistics>(halo::rasterizer::vars().rasterizer_frame_statistics_state);
static auto &rasterizer_caps = halo::link::ref<d3d_caps9>(halo::rasterizer::vars().rasterizer_caps);
static auto &rasterizer_present_parameters = halo::link::ref<d3d_present_parameters>(halo::rasterizer::vars().rasterizer_present_parameters);
static auto &rasterizer_software_vertex_processing = halo::link::ref<uint8_t>(halo::rasterizer::vars().rasterizer_software_vertex_processing);
static auto &rasterizer_window_handle = halo::link::ref<void *>(halo::rasterizer::vars().rasterizer_window_handle);

namespace halo::rasterizer {

Globals &globals()
{
    static Globals instance{::rasterizer_device, ::rasterizer_device_version, ::rasterizer_fullscreen, ::rasterizer_gamma_exponent, ::rasterizer_window_requested, ::rasterizer_default_z_near, ::rasterizer_default_z_far, ::rasterizer_present_counter_high, ::rasterizer_present_counter_low, ::rasterizer_needs_reset, ::rasterizer_vertex_buffer_lock_state, ::rasterizer_shader_file_name, ::rasterizer_dynamic_index_buffer, ::rasterizer_decal_vertex_cache_handle, ::rasterizer_render_states_dirty, ::rasterizer_window, ::rasterizer_frame_statistics_state, ::rasterizer_caps, ::rasterizer_present_parameters, ::rasterizer_software_vertex_processing, ::rasterizer_window_handle};
    return instance;
}

}
