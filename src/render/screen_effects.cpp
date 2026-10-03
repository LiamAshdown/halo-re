#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "rasterizer.h"
#include "interface.h"
#include "structures.h"
#include "cutscene.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"

extern "C" {
extern cinematic_screen_effect_globals *cinematic_screen_effect_state;
extern game_time_globals *game_time;
extern GlobalsRasterizerData *rasterizer_globals_data;
extern ColorRGB *default_axis_b;
extern ColorRGB *global_real_rgb_green_pointer;
extern frame_graph frame_graphs[1];
extern void *rasterizer_device;
extern rasterizer_window_parameters rasterizer_window;
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count];
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders];
extern uint8_t rasterizer_software_vertex_processing;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern int16_t hud_text_draw_background_mode;
extern void rasterizer_set_shader_stage_config(int16_t mode);
extern void hud_text_draw_configure(int16_t font_table_index, uint16_t color_or_flags, int16_t column,
    uint32_t unknown_4730, int16_t color_table_index, int16_t color_index);
extern void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, Point2DInt *cursor,
    int32_t flags, const char *text);
extern void *rasterizer_dynamic_index_buffer;
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[];
extern void **rasterizer_effect_pool_scratch;
extern void rasterizer_lens_flare_batch_flush_all(void);
extern uint8_t console_debug_toggle_6893e0;
extern int32_t frame_statistics_key_a_latch;
extern int32_t frame_statistics_key_b_latch;
extern int32_t frame_graph_render_graph;
extern int32_t frame_graph_render_infos;
extern int32_t frame_statistics_last_time;
extern int64_t frame_statistics_unknown_d0;
extern int64_t frame_statistics_unknown_d8;
extern rasterizer_frame_statistics rasterizer_frame_statistics_state;
extern int64_t performance_frequency;
extern Rectangle2D game_screen_rect;
extern int32_t rasterizer_present_counter_low;
extern int32_t rasterizer_present_counter_high;
extern int16_t text_tab_stops[6];
extern void network_bandwidth_graph_update(void);
extern Rectangle2D game_window_top_left;
extern int32_t frame_graph_window_width;
extern int32_t frame_graph_window_height;
extern uint16_t unknown_006893e2;
extern uint32_t frame_statistics_times[60];
extern uint8_t frame_statistics_dropped[60];
extern int16_t frame_statistics_count;
extern int32_t time_query_performance_counter_ms(void);
extern lens_flare_batch_key lens_flare_current_key;
extern uint32_t lens_flare_vertex_specular;
extern float text_color_scale;
extern float rasterizer_default_z_near;
extern float rasterizer_default_z_far;
extern uint32_t rasterizer_frustum_z_values[2];
extern void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source);
extern uint8_t rasterizer_fullscreen;
extern int32_t rasterizer_lens_flare_occlusion_test_issue(int32_t slot_index, const real_point3d *position,
    float radius);
}

/**
 * Returns the clamped 0..1 progress of the current game time between start_time and end_time, or 1 when they are equal.
 */
static float progress(float start_time, float end_time)
{
    float t;

    if (end_time == start_time) {
        return 1.0f;
    }
    t = ((float)game_time->game_time * 0.033333335f - start_time) / (end_time - start_time);
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        return 1.0f;
    }
    return t;
}

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);

typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data,
                                         uint32_t count);

typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t primitive_type,
                                            uint32_t primitive_count, const void *data,
                                            uint32_t stride);

/**
 * Returns the method table of the rasterizer's Direct3D device.
 */
static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

/**
 * Calls IDirect3DDevice::SetRenderState through the device method table.
 */
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

/**
 * Calls IDirect3DDevice::SetTextureStageState through the device method table.
 */
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags);

typedef int32_t (__stdcall *d3d_release_fn)(void *self);

typedef int32_t (__stdcall *d3d_clear_state_slot_fn)(void *device, uint32_t value);

/**
 * stack -> (cursor, flags, text). src/rasterizer types the two stack slots as opaque
 * uint32_t position_or_color1/2; 0x512e80 passes a Point2DInt out cursor and -4 there.
 */
static void set_text_state(const float color[4], const int16_t tab_stops[6])
{
    int16_t i;

    hud_text_draw_color_a = color[0];
    hud_text_draw_color_r = color[1];
    hud_text_draw_color_g = color[2];
    hud_text_draw_color_b = color[3];
    hud_text_draw_background_mode = 6;
    for (i = 0; i < 6; i++) {
        text_tab_stops[i] = tab_stops[i];
    }
}

namespace halo::render::cinematic_screen_effect {

/**
 * a variable cannot share the typedef's own name in C)
 * Returns cinematic_screen_effect_globals.script_values[index], or 0.0 if there is no active
 * effect block or the index is out of range.
 *
 * @address 0x005121a0
 */
float get_script_value(int16_t index)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;

    if (g != 0 && index >= 0 && index < 4) {
        return g->script_values[index];
    }
    return 0.0f;
}

/**
 * Starts (or restarts) the convolution screen effect: clears the unrelated video fields, then
 * sets the extra-passes count, type, radius bounds and the [start, start+duration] time window
 * the per-frame update (cinematic_screen_effect_update 0x512360) will interpolate over.
 *
 * @address 0x005121d0
 */
void set_convolution(int16_t convolution_type, int16_t extra_passes, float radius_lower_bound,
    float radius_upper_bound, float duration)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    float start_time;

    if (g == 0) {
        return;
    }

    g->video_enabled = 0;
    g->video_overbright_mode = 0;
    g->video_scanline_map = 0;
    g->video_noise_intensity = 0.0f;
    g->unknown_30 = 0.0f;
    g->video_noise_map = 0;

    g->convolution_extra_passes = extra_passes;
    g->convolution_type = convolution_type;
    g->convolution_radius_lower_bound = radius_lower_bound;
    g->convolution_radius_upper_bound = radius_upper_bound;

    start_time = (float)game_time->game_time * 0.033333335f;
    g->convolution_start_time = start_time;
    g->convolution_end_time = start_time + duration;
}

/**
 * Starts (or restarts) the light-enhancement/desaturation filter screen effect: sets the four
 * intensity bounds, the additive-blend flag, clears the unrelated video fields, and sets the
 * [start, start+duration] time window the per-frame update interpolates over.
 *
 * @address 0x00512230
 */
void set_filter(float light_enhancement_lower, float light_enhancement_upper, float desaturation_lower,
    float desaturation_upper, uint8_t is_additive, float duration)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    float start_time;

    if (g == 0) {
        return;
    }

    g->filter_light_enhancement_intensity_lower_bound = light_enhancement_lower;
    g->filter_light_enhancement_intensity_upper_bound = light_enhancement_upper;
    g->filter_desaturation_intensity_lower_bound = desaturation_lower;
    g->filter_desaturation_intensity_upper_bound = desaturation_upper;

    g->video_enabled = 0;
    g->video_overbright_mode = 0;
    g->video_scanline_map = 0;
    g->video_noise_intensity = 0.0f;
    g->unknown_30 = 0.0f;
    g->video_noise_map = 0;

    g->filter_desaturation_is_additive = is_additive;

    start_time = (float)game_time->game_time * 0.033333335f;
    g->night_vision_masked = 0;
    g->desaturation_masked = 0;
    g->filter_start_time = start_time;
    g->filter_end_time = start_time + duration;
}

/**
 * Starts (or restarts) the video screen effect (overbright mode plus scanline/noise maps),
 * provided both bitmap tags are actually set; otherwise leaves the block untouched.
 *
 * @address 0x005122a0
 */
void set_video(int16_t overbright_mode, float noise_intensity)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    tag_instance *scanline_tag;
    tag_instance *noise_tag;
    uint32_t *block;
    int i;

    if (g == 0) {
        return;
    }
    if (*(int32_t *)&rasterizer_globals_data->video_scanline_map.tag_id == -1) {
        return;
    }
    if (*(int32_t *)&rasterizer_globals_data->video_noise_map.tag_id == -1) {
        return;
    }

    block = (uint32_t *)g;
    for (i = 0; i < 0xe; i++) {
        block[i] = 0;
    }

    g->video_overbright_mode = overbright_mode;

    g->convolution_radius_lower_bound = 0.0f;
    g->convolution_radius_upper_bound = 0.0f;
    g->convolution_start_time = 0.0f;
    g->convolution_end_time = 0.0f;
    g->filter_light_enhancement_intensity_lower_bound = 0.0f;
    g->filter_light_enhancement_intensity_upper_bound = 0.0f;
    g->filter_desaturation_intensity_lower_bound = 0.0f;
    g->filter_desaturation_intensity_upper_bound = 0.0f;
    g->filter_start_time = 0.0f;
    g->filter_end_time = 0.0f;

    g->video_enabled = 1;

    scanline_tag = &halo::cache::globals().tag_instances[(uint16_t)*(int32_t *)&rasterizer_globals_data->video_scanline_map.tag_id];
    g->video_scanline_map = *(uint32_t *)((uint8_t *)scanline_tag->data + 0x64);

    g->video_noise_intensity = noise_intensity;
    g->unknown_30 = 1.0f;

    noise_tag = &halo::cache::globals().tag_instances[(uint16_t)*(int32_t *)&rasterizer_globals_data->video_noise_map.tag_id];
    g->video_noise_map = *(uint32_t *)((uint8_t *)noise_tag->data + 0x64);
}

/**
 * Recomputes the interpolated convolution radius and filter intensities / tint of the active
 * cinematic screen effect from the elapsed game time and clears each part once it has decayed.
 * Returns the cinematic block when one is active, else hands the caller's own block back.
 *
 * @address 0x00512360
 */
cinematic_screen_effect_globals *update(cinematic_screen_effect_globals *input)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    float convolution_progress;
    float filter_progress;
    uint32_t *tint;
    uint32_t *black;

    if (g == 0 || g->active == 0) {
        return input;
    }

    convolution_progress = progress(g->convolution_start_time, g->convolution_end_time);
    filter_progress = progress(g->filter_start_time, g->filter_end_time);

    g->convolution_radius = (1.0f - convolution_progress) * g->convolution_radius_lower_bound +
                            convolution_progress * g->convolution_radius_upper_bound;
    halo::math::real_lerp_clamped(g->filter_light_enhancement_intensity,
                      g->filter_light_enhancement_intensity_lower_bound,
                      g->filter_light_enhancement_intensity_upper_bound, filter_progress);
    halo::math::real_lerp_clamped(g->filter_desaturation_intensity,
                      g->filter_desaturation_intensity_lower_bound,
                      g->filter_desaturation_intensity_upper_bound, filter_progress);

    tint = (uint32_t *)&g->filter_desaturation_tint;
    black = (uint32_t *)default_axis_b;
    if (tint[0] == black[0] && tint[1] == black[1] && tint[2] == black[2]) {
        g->filter_desaturation_tint = *global_real_rgb_green_pointer;
    }

    if (g->convolution_radius <= 0.0001f) {
        g->convolution_radius = 0.0f;
        g->convolution_type = 0;
        g->convolution_extra_passes = 0;
    }
    if (g->filter_light_enhancement_intensity <= 0.0001f &&
        g->filter_desaturation_intensity <= 0.0001f && filter_progress >= 1.0f) {
        g->filter_light_enhancement_intensity = 0.0f;
        g->filter_desaturation_intensity = 0.0f;
    }
    return g;
}

}  // namespace halo::render::cinematic_screen_effect

namespace halo::render::fg {

/**
 * Pushes a new sample into frame_graphs[index]'s 4 entry recent-sample ring, recomputes the
 * running average, scrolls the graph's 0x200 line-strip vertices left by one, and appends the
 * new sample (clamped to the graph's maximum) as the rightmost vertex height.
 *
 * @address 0x00512d90
 */
void add_sample(int32_t index, float sample)
{
    frame_graph *g = &frame_graphs[index];
    int i;
    float clamped;

    g->recent_samples[0] = g->recent_samples[1];
    g->recent_samples[1] = g->recent_samples[2];
    g->recent_samples[2] = g->recent_samples[3];
    g->recent_samples[3] = sample;

    g->average = (g->recent_samples[0] + g->recent_samples[1] + g->recent_samples[2] +
                  g->recent_samples[3]) * 0.25f;

    clamped = sample;
    if (g->maximum < sample) {
        clamped = g->maximum;
    }

    for (i = 0; i < 0x200 - 1; i++) {
        g->vertices[i].y = g->vertices[i + 1].y;
    }

    g->vertices[0x200 - 1].y = (float)g->bounds.bottom - (clamped / g->maximum) * 120.0f;
}

/**
 * Draws the frame rate graph (when render_graph is set) as two screen space line strips plus its
 * "FPS" / maximum / average labels, after putting the device into a plain vertex colour, no
 * depth, no blend state.
 *
 * @address 0x005129a0
 */
void draw(uint8_t render_graph, uint8_t render_infos)
{
    float white[4];
    float constants[20];
    char text[0x200];
    float inverse;
    int16_t width;
    int16_t height;

    white[0] = 1.0f;
    white[1] = 1.0f;
    white[2] = 1.0f;
    white[3] = 1.0f;
    if (!render_graph && !render_infos) {
        return;
    }

    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
        rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
        ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
         rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[35].shader);

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right -
                      rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom -
                       rasterizer_window.camera.viewport_bounds.top);
    inverse = 1.0f / (float)width;
    constants[0] = inverse + inverse;
    constants[1] = 0.0f;
    constants[2] = 0.0f;
    constants[3] = -1.0f - inverse;
    inverse = 1.0f / (float)height;
    constants[4] = 0.0f;
    constants[5] = -2.0f * inverse;
    constants[6] = 0.0f;
    constants[7] = inverse + 1.0f;
    constants[8] = 0.0f;
    constants[9] = 0.0f;
    constants[10] = 0.0f;
    constants[11] = 0.5f;
    constants[12] = 0.0f;
    constants[13] = 0.0f;
    constants[14] = 0.0f;
    constants[15] = 1.0f;
    constants[16] = 1.0f;
    constants[17] = 1.0f;
    constants[18] = 0.0f;
    constants[19] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 13, constants, 5);

    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);
    rasterizer_set_shader_stage_config(0);
    set_render_state(0x16, 1);
    set_render_state(0xa8, 0xf);
    set_render_state(0x1b, 0);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 0);
    set_render_state(0x0e, 0);
    set_render_state(0x1c, 0);
    set_texture_stage_state(0, 1, 3);
    set_texture_stage_state(0, 3, 0);
    set_texture_stage_state(0, 4, 3);
    set_texture_stage_state(0, 6, 0);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, 0, 0);
    set_render_state(0x9c, 1);
    set_render_state(0x9d, 0);

    if (render_graph) {
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 3, 0x1ff,
            frame_graphs[0].vertices, sizeof(rasterizer_dynamic_screen_vertex));
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 3, 4,
            frame_graphs[0].frame_vertices, sizeof(rasterizer_dynamic_screen_vertex));
    }

    hud_text_draw_configure(1, 0xffff, 0, 0, 5, 0);
    hud_text_draw_color_a = white[0];
    hud_text_draw_background_mode = 0;
    hud_text_draw_color_r = white[1];
    hud_text_draw_color_g = white[2];
    hud_text_draw_color_b = white[3];

    if (render_graph) {
        chimera__draw_8_bit_text(0, (int32_t *)&frame_graphs[0].name_bounds, 0, 0,
                                 frame_graphs[0].name);
        sprintf(text, "%d", (int32_t)frame_graphs[0].maximum);
        chimera__draw_8_bit_text(0, (int32_t *)&frame_graphs[0].maximum_bounds, 0, 0, text);
        sprintf(text, "%d", (int32_t)frame_graphs[0].average);
        chimera__draw_8_bit_text(0, (int32_t *)&frame_graphs[0].average_bounds, 0, 0, text);
    }

    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

}  // namespace halo::render::fg

namespace halo::render::rasterizer {

/**
 * Locks the index range described by rasterizer_dynamic_index_slots[slot_index] (offsets scaled
 * by 6, matching the buffer's index format) and returns the locked pointer, or 0 for slot index
 * -1 (no slot).
 *
 * @address 0x00511e80
 */
void *dynamic_index_slot_lock(int32_t slot_index)
{
    rasterizer_dynamic_index_slot *slot;
    d3d_lock_fn lock;

    if (slot_index == -1) {
        return 0;
    }

    slot = &rasterizer_dynamic_index_slots[slot_index];
    lock = *(d3d_lock_fn *)((uint8_t *)*(void **)rasterizer_dynamic_index_buffer + 0x2c);
    lock(rasterizer_dynamic_index_buffer, slot->first_index * 6, slot->index_count * 6,
         (void **)&slot->locked_indices, 0x1000);
    return (void *)slot->locked_indices;
}

/**
 * Flushes pending lens flare batches, releases and clears the active rasterizer effect slot's
 * COM object pointer, and clears a device state slot (vtable+0x164).
 *
 * @address 0x00512150
 */
void effect_slot_release_active(void)
{
    void **vtable;

    rasterizer_lens_flare_batch_flush_all();

    if (rasterizer_effect_pool_scratch != 0 && *rasterizer_effect_pool_scratch != 0) {
        void *effect = *rasterizer_effect_pool_scratch;
        vtable = *(void ***)effect;
        ((d3d_release_fn)vtable[0x108 / 4])(effect);
    }
    rasterizer_effect_pool_scratch = 0;

    vtable = *(void ***)rasterizer_device;
    ((d3d_clear_state_slot_fn)vtable[0x164 / 4])(rasterizer_device, 0);
}

/**
 * Latches the current lens flare batch key (bitmap_tag_index, second_bitmap_tag_index,
 * bitmap_index), substituting the glow bitmap's tag id when no second bitmap was given, and
 * returns that resolved second bitmap tag index with its low byte cleared.
 *
 * @address 0x005120f0
 */
uint8_t lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index, int16_t bitmap_index)
{
    lens_flare_current_key.bitmap_tag_index = bitmap_tag_index;
    if (second_bitmap_tag_index == -1) {
        second_bitmap_tag_index = *(int32_t *)&rasterizer_globals_data->glow.tag_id;
    }
    lens_flare_current_key.second_bitmap_tag_index = second_bitmap_tag_index;
    lens_flare_current_key.bitmap_index = bitmap_index;
    return 0;
}

/**
 * __ftol (0x6391b4, input on the FPU stack, chops toward zero) is written as a (long long) cast below
 * Scales a normalized intensity to a byte and broadcasts it into all four bytes of the packed
 * ARGB specular color every lens flare vertex shares.
 *
 * @address 0x00512120
 */
void lens_flare_set_vertex_specular(float intensity)
{
    uint32_t byte_value;
    uint32_t packed;

    byte_value = (uint32_t)(int32_t)(long long)((double)(intensity * text_color_scale)) & 0xff;
    packed = byte_value;
    packed = (packed << 8) | byte_value;
    packed = (packed << 8) | byte_value;
    packed = (packed << 8) | byte_value;
    lens_flare_vertex_specular = packed;
}

}  // namespace halo::render::rasterizer

namespace halo::render::frame_statistics {

/**
 * Per frame: handles the ctrl+f11 / ctrl+f12 toggles, samples the frame time into the frame
 * graph and draws it, and, when frame statistics are enabled, prints the two line table of
 * framerate / average / overall average / min / max / dropped percentage.
 *
 * @address 0x00512e80
 */
void draw(void)
{
    large_integer counter;
    uint32_t milliseconds;
    uint32_t delta;
    uint32_t sample;
    Rectangle2D bounds;
    int16_t tab_stops[6];
    Point2DInt cursor;
    float header_color[4];
    float value_color[4];
    float restore_color[4];
    int64_t presents;
    int64_t elapsed;
    int16_t left;
    int16_t i;
    char text[0x3000];

    if (halo::input::input_get_key_state(0x6f) && halo::input::input_get_key_state(0x0b)) {
        if (frame_statistics_key_a_latch == 0) {
            frame_graph_render_infos = 1 - frame_graph_render_infos;
            frame_statistics_key_a_latch = 1;
        }
    } else {
        frame_statistics_key_a_latch = 0;
    }
    if (halo::input::input_get_key_state(0x6f) && halo::input::input_get_key_state(0x0c)) {
        if (frame_statistics_key_b_latch == 0) {
            frame_graph_render_graph = 1 - frame_graph_render_graph;
            frame_statistics_key_b_latch = 1;
        }
    } else {
        frame_statistics_key_b_latch = 0;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    milliseconds = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    delta = milliseconds - (uint32_t)frame_statistics_last_time;
    frame_statistics_last_time = (int32_t)milliseconds;
    sample = delta;
    if (delta != 0) {
        sample = 1000 / delta;
    }
    rasterizer_frame_statistics_graph_init();
    fg_add_sample(0, (float)sample);
    fg_render((uint8_t)frame_graph_render_graph, (uint8_t)frame_graph_render_infos);
    network_bandwidth_graph_update();

    if (!console_debug_toggle_6893e0) {
        return;
    }

    presents = (int64_t)(((uint64_t)(uint32_t)rasterizer_present_counter_high << 32) | (uint32_t)rasterizer_present_counter_low) -
               frame_statistics_unknown_d8;
    left = game_screen_rect.left;
    tab_stops[0] = 100;
    tab_stops[1] = 200;
    tab_stops[2] = 300;
    tab_stops[3] = 400;
    tab_stops[4] = 500;
    tab_stops[5] = 600;
    cursor.y = 0;
    header_color[0] = 1.0f;
    header_color[1] = 1.0f;
    header_color[2] = 1.0f;
    header_color[3] = 1.0f;
    value_color[0] = 1.0f;
    value_color[1] = 0.66f;
    value_color[2] = 1.0f;
    value_color[3] = 0.66f;
    restore_color[0] = 1.0f;
    restore_color[1] = 1.0f;
    restore_color[2] = 1.0f;
    restore_color[3] = 1.0f;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    milliseconds = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed = (int64_t)(uint64_t)milliseconds - frame_statistics_unknown_d0;

    for (i = 0; i < 6; i++) {
        tab_stops[i] = (int16_t)(tab_stops[i] + left);
    }
    bounds = game_screen_rect;
    bounds.top = (int16_t)(bounds.top + 0x20);
    bounds.bottom = (int16_t)(bounds.bottom + 0x20);

    hud_text_draw_configure(1, 0xffff, 0, 0, 5, 0);
    sprintf(text, "|n|tframerate|taverage (of %d)|tmin|tmax|tdropped",
            (int32_t)rasterizer_frame_statistics_state.sample_count);
    tab_stops[0] = left;
    set_text_state(header_color, tab_stops);
    chimera__draw_8_bit_text(0, (int32_t *)&bounds, &cursor, -4, text);

    bounds.top = (int16_t)(cursor.y - 1);
    sprintf(text, "|t%.0f|t%.0f/%.0f|t%.0f|t%.0f|t%5.1f%%|n",
            (double)rasterizer_frame_statistics_state.framerate,
            (double)rasterizer_frame_statistics_state.average_framerate,
            (double)(((float)presents * 1000.0f) / (float)elapsed),
            (double)rasterizer_frame_statistics_state.minimum_framerate,
            (double)rasterizer_frame_statistics_state.maximum_framerate,
            (double)rasterizer_frame_statistics_state.dropped_percentage);
    tab_stops[0] = left;
    set_text_state(value_color, tab_stops);
    chimera__draw_8_bit_text(0, (int32_t *)&bounds, &cursor, -4, text);

    hud_text_draw_color_a = restore_color[0];
    hud_text_draw_background_mode = 0;
    hud_text_draw_color_r = restore_color[1];
    hud_text_draw_color_g = restore_color[2];
    hud_text_draw_color_b = restore_color[3];
}

/**
 * Rebuilds the frame rate graph layout (line strip, border, label rectangles) for the current
 * game window size, and resets its samples and labels, whenever the window size changed.
 *
 * @address 0x00512700
 */
void graph_init(void)
{
    frame_graph *g = &frame_graphs[0];
    int32_t width = (int32_t)game_window_top_left.right - (int32_t)game_window_top_left.left;
    int32_t height = (int32_t)game_window_top_left.bottom - (int32_t)game_window_top_left.top;
    float right;
    float span;
    float x_scale;
    float y_scale;
    float border_right;
    uint32_t *raw;
    int32_t i;

    if (frame_graph_window_width == height && frame_graph_window_height == width) {
        return;
    }

    right = (float)((int32_t)game_window_top_left.right - 0x40);
    frame_graph_window_width = height;
    frame_graph_window_height = width;
    g->bounds.left = 0x40;
    g->bounds.top = 0x1e;
    x_scale = 640.0f / (float)width;
    y_scale = 480.0f / (float)height;
    g->bounds.right = (int16_t)(int32_t)right;
    span = right - 64.0f;
    g->bounds.bottom = 0x96;

    raw = (uint32_t *)g->vertices;
    for (i = 0; i < 0xc00; i++) {
        raw[i] = 0;
    }
    raw = (uint32_t *)g->frame_vertices;
    for (i = 0; i < 0x1e; i++) {
        raw[i] = 0;
    }
    g->recent_samples[0] = 0.0f;
    g->recent_samples[1] = 0.0f;
    g->recent_samples[2] = 0.0f;
    g->recent_samples[3] = 0.0f;

    for (i = 0; i < 0x200; i++) {
        g->vertices[i].y = 150.0f;
        g->vertices[i].color = 0xffffffff;
        g->vertices[i].x = (float)(int32_t)((float)i * span * 0.001953125f + 64.0f);
    }

    border_right = right + 1.0f;
    g->frame_vertices[0].x = 63.0f;
    g->frame_vertices[1].x = border_right;
    g->frame_vertices[0].y = 29.0f;
    g->frame_vertices[2].x = border_right;
    g->frame_vertices[0].color = 0xffffff00;
    g->frame_vertices[1].y = 29.0f;
    g->frame_vertices[1].color = 0xffffff00;
    g->frame_vertices[2].y = 151.0f;
    g->frame_vertices[2].color = 0xffffff00;
    g->frame_vertices[3].x = 63.0f;
    g->frame_vertices[3].y = 151.0f;
    g->frame_vertices[3].color = 0xffffff00;
    g->frame_vertices[4].x = 63.0f;
    g->frame_vertices[4].y = 29.0f;
    g->frame_vertices[4].color = 0xffffff00;

    g->name_bounds.left = (int16_t)(int32_t)((float)g->bounds.left * x_scale);
    g->name_bounds.top = (int16_t)(int32_t)((float)g->bounds.top * y_scale);
    g->name_bounds.right = 0x280;
    g->name_bounds.bottom = 0x1e0;
    g->maximum_bounds.left = (int16_t)(int32_t)((float)g->bounds.right * x_scale);
    g->maximum_bounds.top = (int16_t)(int32_t)((float)g->bounds.top * y_scale);
    g->maximum_bounds.right = 0x280;
    g->maximum_bounds.bottom = 0x1e0;
    g->average_bounds.left = (int16_t)(int32_t)((float)g->bounds.right * x_scale);
    g->average_bounds.top = (int16_t)(int32_t)(((float)g->bounds.top + 60.0f) * y_scale);
    g->average_bounds.right = 0x280;
    g->average_bounds.bottom = 0x1e0;

    *(uint32_t *)g->name = 0x00535046;
    g->maximum = 60.0f;
    g->average = 0.0f;
}

/**
 * milliseconds-resolution system time counter
 * Records a new frame sample (its system-time timestamp and whether it was dropped) into the
 * 60 entry rolling history, shifting the arrays and folding in the running current/average/min/
 * max framerate and dropped-frame percentage into *statistics. Does nothing (but resets the
 * count) while frame statistics are disabled or statistics is NULL.
 *
 * @address 0x00512530
 */
void sample(rasterizer_frame_statistics *statistics, uint8_t dropped)
{
    int32_t now;
    int16_t count;
    uint32_t current_interval;
    uint32_t min_interval;
    uint32_t max_interval;
    int16_t dropped_count;
    int16_t i;

    if ((console_debug_toggle_6893e0 == 0 && unknown_006893e2 == 0) || statistics == 0) {
        frame_statistics_count = 0;
        return;
    }

    now = time_query_performance_counter_ms();
    count = frame_statistics_count;

    if (count != 0) {
        current_interval = (uint32_t)now - frame_statistics_times[0];
        min_interval = current_interval;
        max_interval = current_interval;
        dropped_count = 0;

        for (i = (int16_t)(count - 1); i > 0; i--) {
            if (i > 1) {
                uint32_t interval = frame_statistics_times[i - 1] - frame_statistics_times[i];
                if (interval <= min_interval) {
                    min_interval = interval;
                }
                if (max_interval < interval) {
                    max_interval = interval;
                }
            }
            if (frame_statistics_dropped[i] != 0) {
                dropped_count = dropped_count + 1;
            }
            frame_statistics_times[i] = frame_statistics_times[i - 1];
            frame_statistics_dropped[i] = frame_statistics_dropped[i - 1];
        }

        {
            uint32_t latest_interval = (uint32_t)now - frame_statistics_times[0];
            if (latest_interval < 2) {
                latest_interval = 1;
            }
            statistics->sample_count = count;
            statistics->framerate = 1000.0f / (float)latest_interval;
        }
        {
            uint32_t elapsed = (uint32_t)now - frame_statistics_times[count - 1];
            if (elapsed < 2) {
                elapsed = 1;
            }
            statistics->average_framerate =
                ((float)count * 1000.0f) / (float)elapsed;
        }
        if (min_interval < 2) {
            min_interval = 1;
        }
        statistics->maximum_framerate = 1000.0f / (float)min_interval;
        if (max_interval < 2) {
            max_interval = 1;
        }
        statistics->minimum_framerate = 1000.0f / (float)max_interval;
        statistics->dropped_percentage =
            ((float)dropped_count * 100.0f) / (float)count;
    }

    frame_statistics_times[0] = now;
    frame_statistics_dropped[0] = dropped;

    if (count + 1 > 0x3c) {
        frame_statistics_count = 0x3c;
    } else {
        frame_statistics_count = count + 1;
    }
}

}  // namespace halo::render::frame_statistics

namespace halo::render::frame {

/**
 * Ensures the two default near/far clip distance pairs have been seeded (once, the first time
 * either is still exactly 0.0), then dispatches the per-frame cinematic screen effect update.
 *
 * @address 0x00511df0
 */
void cinematic_screen_effect_update(rasterizer_frame_time *time_source)
{
    if (rasterizer_default_z_near == 0.0f) {
        rasterizer_default_z_near = 0.0625f;
    }
    if (rasterizer_default_z_far == 0.0f) {
        rasterizer_default_z_far = 1024.0f;
    }
    if (*(float *)&rasterizer_frustum_z_values[0] == 0.0f) {
        *(float *)&rasterizer_frustum_z_values[0] = 0.01171875f;
    }
    if (*(float *)&rasterizer_frustum_z_values[1] == 0.0f) {
        *(float *)&rasterizer_frustum_z_values[1] = 1024.0f;
    }
    chimera__cinematic_screen_effect(time_source);
}

/**
 * Returns whether the rasterizer device has been created and the fullscreen/initialized flag is
 * set.
 *
 * @address 0x00511d80
 */
int device_is_ready(void)
{
    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        return 1;
    }
    return 0;
}

/**
 * Forwards a sample point and radius to the rasterizer routine at 0x537800 with the caller's lens flare slot index preserved.
 *
 * @address 0x00512190
 */
int32_t rasterizer_dispatch_537800(int32_t slot_index, real_point3d *point, float radius)
{
    return rasterizer_lens_flare_occlusion_test_issue(slot_index, point, radius);
}

}  // namespace halo::render::frame
