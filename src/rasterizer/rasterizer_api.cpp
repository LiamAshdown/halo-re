#include "halo/rasterizer/api.hpp"

extern "C" {
extern void *rasterizer_device;
extern int32_t rasterizer_device_version;
extern uint8_t rasterizer_fullscreen;
extern int32_t rasterizer_gamma_exponent;
extern int32_t rasterizer_window_requested;
extern float rasterizer_default_z_near;
extern float rasterizer_default_z_far;
extern int32_t rasterizer_present_counter_high;
extern int32_t rasterizer_present_counter_low;
extern uint8_t rasterizer_needs_reset;
extern int16_t rasterizer_vertex_buffer_lock_state;
extern char *rasterizer_shader_file_name;
extern void *rasterizer_dynamic_index_buffer;
extern uint32_t rasterizer_decal_vertex_cache_handle;
extern uint8_t rasterizer_render_states_dirty;
extern rasterizer_window_parameters rasterizer_window;
extern rasterizer_frame_statistics rasterizer_frame_statistics_state;
extern d3d_caps9 rasterizer_caps;
extern d3d_present_parameters rasterizer_present_parameters;
extern uint8_t rasterizer_software_vertex_processing;
extern void *rasterizer_window_handle;
}

namespace halo::rasterizer {

Globals &globals()
{
    static Globals instance{::rasterizer_device, ::rasterizer_device_version, ::rasterizer_fullscreen, ::rasterizer_gamma_exponent, ::rasterizer_window_requested, ::rasterizer_default_z_near, ::rasterizer_default_z_far, ::rasterizer_present_counter_high, ::rasterizer_present_counter_low, ::rasterizer_needs_reset, ::rasterizer_vertex_buffer_lock_state, ::rasterizer_shader_file_name, ::rasterizer_dynamic_index_buffer, ::rasterizer_decal_vertex_cache_handle, ::rasterizer_render_states_dirty, ::rasterizer_window, ::rasterizer_frame_statistics_state, ::rasterizer_caps, ::rasterizer_present_parameters, ::rasterizer_software_vertex_processing, ::rasterizer_window_handle};
    return instance;
}

}
