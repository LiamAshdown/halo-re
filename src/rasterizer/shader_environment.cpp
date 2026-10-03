/**
 * @file src/rasterizer/shader_environment.cpp
 * ShaderEnvironment draw passes.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "internal/state.hpp"
#include "halo/shaders/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {

extern int32_t sprintf(char *buffer, const char *format, ...);
extern void debug_fp_dispatch_note(int32_t toggle, int32_t mode, int32_t shader_type, int32_t primitives, void *draw, void *draw_simple, void *overlay);
extern uint32_t color_rgb_float_to_int(const ColorRGB *color);
extern uint32_t color_pack_argb_from_real(ColorARGB *color);
extern BitmapData *bitmap_group_get_bitmap_data(uint32_t bitmap_tag_id, int16_t index);

}  // extern "C"

namespace halo::rasterizer {

static uint8_t build_stage(int32_t *table, int32_t count, int effect_index, const char *format, uint8_t skip_middle)
{
    char name[128];
    int32_t i;

    for (i = 0; i < count; i++) {
        int32_t suffix = (skip_middle && i >= 6) ? i + 6 : i;
        void *technique;

        sprintf(name, format, rasterizer_shader_technique_name_suffixes[suffix]);
        technique = rasterizer_shader_technique_for_name((void *)(uintptr_t)rasterizer_effects[effect_index].effect, name);
        table[i] = (int32_t)(uintptr_t)technique;
        if (technique == NULL) {
            return 0;
        }
    }
    return 1;
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_build_technique_table. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_build_technique_table.c.txt.
 *
 * @address 0x526930
 */
uint8_t rasterizer_shader_environment_build_technique_table(void)
{
    uint8_t ps_1_4 = rasterizer_caps.pixel_shader_version >= 0xffff0104;
    int32_t short_count = ps_1_4 ? 0xc : 6;
    int32_t long_count = ps_1_4 ? 0x18 : 0xc;
    uint8_t ok;

    ok = build_stage(environment_techniques_no, short_count, 116, "EnvironmentNo%s", 1);
    ok = ok && build_stage(environment_techniques_self_illumination, long_count, 117, "SelfIllumination%s", 0);
    ok = ok && build_stage(environment_techniques_change_color, long_count, 118, "ChangeColor%s", 0);
    ok = ok && build_stage(environment_techniques_multipurpose, long_count, 119, "Multipurpose%s", 0);
    ok = ok && build_stage(environment_techniques_reflection, long_count, 120, "Reflection%s", 0);
    ok = ok && build_stage(environment_techniques_plain, short_count, 121, "No%s", 1);
    if (rasterizer_caps.pixel_shader_version >= 0xffff0101 && rasterizer_caps.pixel_shader_version < 0xffff0104 && ok) {
        ok = build_stage(&environment_techniques_plain[6], short_count, 121, "No%sSelfIllumination", 0);
    }
    return ok;
}

typedef void (*rasterizer_part_draw_procedure)(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
                                               int32_t dynamic_index_slot, int32_t primitive_count,
                                               rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot);

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_dispatch. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_draw_dispatch.c.txt.
 *
 * @address 0x52b050
 */
void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    rasterizer_model_draw_context *context;
    uint8_t *overlay;

    debug_fp_dispatch_note(console_debug_toggle_6893ec, rasterizer_active_model_mode, *(int16_t *)&((struct Shader *)shader)->shader_type,
        primitive_count, shader_environment_draw, shader_environment_draw_simple,
        rasterizer_active_model_context ? (void *)(uintptr_t)rasterizer_active_model_context->group_parameters.shader : 0);

    if (!console_debug_toggle_6893ec) {
        return;
    }
    context = rasterizer_active_model_context;
    overlay = (uint8_t *)(uintptr_t)context->group_parameters.shader;
    if (overlay != NULL) {
        int16_t source = *(int16_t *)(overlay + 0x2c);
        const float *function_values = (const float *)(uintptr_t)context->group_parameters.function_values;
        uint8_t hidden = (*(int16_t *)(overlay + 0x24) == 0xb && source >= 1 && source <= 4 &&
                          function_values != NULL && function_values[source - 1] == 0.0f);

        if (!hidden) {
            transparent_geometry_group *group =
                rasterizer_transparent_geometry_group_build(NULL, overlay, frame, index_buffer, dynamic_index_slot,
                                                            primitive_count, vertex_buffer, dynamic_vertex_slot,
                                                            &context->center);

            context = rasterizer_active_model_context;
            if (group != NULL) {
                group->lighting_extra =
                    (uint32_t)(uintptr_t)chimera__rasterizer_memory_alloc(&context->group_parameters.unknown_20, 8);
            }
        }
    }
    if (rasterizer_active_model_mode == 1) {
        rasterizer_transparent_geometry_group_build(NULL, shader, frame, index_buffer, dynamic_index_slot,
                                                    primitive_count, vertex_buffer, dynamic_vertex_slot,
                                                    &context->center);
        rasterizer_render_target_capture_requested = 1;
        return;
    }
    if (rasterizer_active_model_mode == 0) {
        if (*(int16_t *)&((struct Shader *)shader)->shader_type == 3) {
            ((rasterizer_part_draw_procedure)shader_environment_draw_simple)(
                shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
        } else {
            ((rasterizer_part_draw_procedure)shader_environment_draw)(
                shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
        }
    }
}




typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#undef DEVICE_CALL
#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_combine_stages(uint8_t *shader, int16_t frame, const float *matrix)
{
    chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame);
    render_device().set_transform(0x10, matrix);
    set_texture_stage_state(0, 0x18, 2);
    set_texture_stage_state(0, 1, 4);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 3, 0);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 0);
    set_texture_stage_state(1, 1, 2);
    set_texture_stage_state(1, 2, 1);
    set_texture_stage_state(1, 4, 2);
    set_texture_stage_state(1, 5, 2);
    set_texture_stage_state(2, 1, 1);
    set_texture_stage_state(2, 4, 1);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_fixed_function. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_draw_fixed_function.c.txt.
 *
 * @address 0x527ae0
 */
void rasterizer_shader_environment_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    float matrix[16];

    if (console_debug_toggle_6893ec == 0) {
        return;
    }
    if (((uint8_t *)rasterizer_active_model_context)[0] & 8) {
        set_render_state(7, 0);
    } else {
        set_render_state(7, 1);
        set_render_state(0xe, 1);
        set_render_state(0x17, 4);
        rasterizer_clear_decal_zbias();
    }
    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 0);
    set_render_state(0x13, 5);
    set_render_state(0x14, 6);
    set_render_state(0xab, 1);
    set_render_state(0xf, shader[0x28] & 1);
    set_render_state(0x18, 0x7f);
    set_render_state(0x1c, rasterizer_fog_enabled != 0);

    if (rasterizer_effects[116].effect == 0) {
        rasterizer_clear_decal_zbias();
        return;
    }
    memset(matrix, 0, sizeof matrix);
    matrix[0] = *(float *)(((uint8_t *)rasterizer_active_model_context) + 0xc4);
    matrix[5] = *(float *)(((uint8_t *)rasterizer_active_model_context) + 0xc8);
    matrix[15] = 1.0f;
    chimera__rasterizer_set_texture((shader[0x28] & 1) ? *(uint32_t *)(shader + 0x134) : 0xffffffff, 1, 0, 1, frame);

    if (*(uint32_t *)((uint8_t *)rasterizer_active_model_context) & 0x200) {
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[14].declaration);
        set_combine_stages(shader, frame, matrix);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
        rasterizer_clear_decal_zbias();
        return;
    }
    {
        rasterizer_vertex_buffer processed = *vertex_buffer;
        uint32_t handle = 0;

        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[4].declaration);
        if (index_buffer != 0) {
            handle = rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer);
        }
        processed.hardware_buffer = handle;
        processed.type = 0xf;
        set_texture_stage_state(0, 0x18, 2);
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[15].declaration);
        set_combine_stages(shader, frame, matrix);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
    }
    rasterizer_clear_decal_zbias();
}
#undef DEVICE_CALL

namespace rasterizer_shader_environment_draw_pixel_shader_impl {








static void environment_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static float environment_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static void environment_set_vector(void *effect, uint32_t handle, float x, float y, float z, float w)
{
    float vector[4];

    vector[0] = x;
    vector[1] = y;
    vector[2] = z;
    vector[3] = w;
    render_device().effect_set_vector(effect, handle, vector);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_pixel_shader. The original author notes are
 * in docs/original/rasterizer/rasterizer_shader_environment_draw_pixel_shader.c.txt.
 *
 * @address 0x528050
 */
void rasterizer_shader_environment_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    uint8_t *context;
    float relative[3];
    uint16_t pixel_shader_fog = *(uint16_t *)(shader + 0x28) & 4;
    int16_t vertex_shader;
    uint8_t draw_ok = 1;
    void *effect;
    float c10_c12[12];
    float c13_c14[8];
    float a[4];
    float b[4];
    float fog[4];
    float negative[3];
    float add[3];
    uint32_t passes;
    uint32_t pass;

    if (!console_debug_toggle_6893ec) {
        return;
    }
    context = ((uint8_t *)rasterizer_active_model_context);
    relative[0] = *(float *)(context + 0xb4) - rasterizer_camera_position[0];
    relative[1] = *(float *)(context + 0xb8) - rasterizer_camera_position[1];
    relative[2] = *(float *)(context + 0xbc) - rasterizer_camera_position[2];
    if (context[0] & 8) {
        environment_set_render_state(0x07, 0);
    } else {
        environment_set_render_state(0x07, 1);
        environment_set_render_state(0x0e, 1);
        environment_set_render_state(0x17, 4);
        rasterizer_clear_decal_zbias();
    }
    environment_set_render_state(0x16, 3);
    environment_set_render_state(0xa8, 7);
    environment_set_render_state(0x1b, 0);
    environment_set_render_state(0x13, 5);
    environment_set_render_state(0x14, 6);
    environment_set_render_state(0xab, 1);
    environment_set_render_state(0x0f, shader[0x28] & 1);
    environment_set_render_state(0x18, 0x7f);
    if (rasterizer_device_version < 0xffff0104) {
        environment_set_render_state(0x1c, 0);
    } else {
        environment_set_render_state(0x1c, (shader[0x28] >> 2) & 1);
    }

    if (pixel_shader_fog) {
        vertex_shader = 0x1c;
    } else if (unknown_0071d1fb) {
        vertex_shader = 0x19;
    } else if (*(int16_t *)(context + 0x50) > 0) {
        vertex_shader = 0x1a;
    } else if (*(datum_index *)(shader + 0x330) != k_datum_index_none) {
        vertex_shader = 0x1c;
    } else if (*(int16_t *)(context + 0xc) <= 1) {
        vertex_shader = 0x1d;
    } else {
        vertex_shader = 0x1c;
    }

    effect = (void *)(uintptr_t)environment_effect_slot.effect;
    if (effect == 0) {
        rasterizer_clear_decal_zbias();
        return;
    }
    render_device().effect_set_technique(effect, (rasterizer_device_version >= 0xffff0104 && !pixel_shader_fog) ?
            environment_techniques_ps14[*(int16_t *)(shader + 0xb0)] :
            (uint32_t)environment_techniques_no[*(int16_t *)(shader + 0xb0)]);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame, &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xc4), 0, 1, 2, frame, &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b((shader[0x28] & 1) ? *(uint32_t *)(shader + 0x134) : 0xffffffff, 0, 2, 1, frame,
        &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x330), 2, 3, 0, frame, &environment_effect_slot);

    a[0] = *(float *)(shader + 0x2f4) * *(float *)(context + 0x5c);
    a[1] = *(float *)(shader + 0x2a8) * *(float *)(context + 0x60);
    a[2] = *(float *)(shader + 0x2ac) * *(float *)(context + 0x64);
    a[3] = *(float *)(shader + 0x2b0) * *(float *)(context + 0x68);
    b[0] = *(float *)(shader + 0x2f8) * *(float *)(context + 0x5c);
    b[1] = *(float *)(shader + 0x2b4) * *(float *)(context + 0x60);
    b[2] = *(float *)(shader + 0x2b8) * *(float *)(context + 0x64);
    b[3] = *(float *)(shader + 0x2bc) * *(float *)(context + 0x68);
    *(uint32_t *)&c10_c12[0] = *(uint32_t *)(shader + 0xb4);
    *(uint32_t *)&c10_c12[1] = *(uint32_t *)(shader + 0xb4);
    c10_c12[2] = 1.0f;
    c10_c12[3] = 1.0f;
    *(uint32_t *)&c10_c12[4] = *(uint32_t *)(context + 0xc4);
    c10_c12[5] = 0.0f;
    c10_c12[6] = 0.0f;
    c10_c12[7] = 0.0f;
    c10_c12[8] = 0.0f;
    *(uint32_t *)&c10_c12[9] = *(uint32_t *)(context + 0xc8);
    c10_c12[10] = 0.0f;
    c10_c12[11] = 0.0f;
    c13_c14[0] = a[1] - b[1];
    c13_c14[1] = a[2] - b[2];
    c13_c14[2] = a[3] - b[3];
    c13_c14[3] = a[0] - b[0];
    c13_c14[4] = b[1];
    c13_c14[5] = b[2];
    c13_c14[6] = b[3];
    c13_c14[7] = b[0];
    if (render_device().set_vertex_shader_constant_f(10, c10_c12, 3) < 0) {
        draw_ok = 0;
    }
    if (render_device().set_vertex_shader_constant_f(13, c13_c14, 2) < 0) {
        draw_ok = 0;
    }

    fog[0] = a[0];
    fog[1] = a[1];
    fog[2] = a[2];
    fog[3] = a[3];
    negative[0] = relative[0];
    negative[1] = relative[1];
    negative[2] = relative[2];
    add[0] = add[1] = add[2] = 0.0f;
    if (!rasterizer_fog_enabled || (!pixel_shader_fog && (context[0] & 4))) {
        fog[0] = 1.0f;
        fog[1] = fog[2] = fog[3] = 0.0f;
        negative[0] = negative[1] = negative[2] = 0.0f;
    } else if (pixel_shader_fog) {
        if (rasterizer_device_version < 0xffff0104) {
            fog[0] = 1.0f;
            fog[1] = fog[2] = fog[3] = 0.0f;
            environment_set_render_state(0x22, color_rgb_float_to_int(&rasterizer_fog_atmospheric_color));
        }
    } else {
        {
            float height = environment_clamp01((rasterizer_fog_plane[2] * rasterizer_camera_position[2] +
                rasterizer_fog_plane[1] * rasterizer_camera_position[1] +
                rasterizer_fog_plane[0] * rasterizer_camera_position[0] - rasterizer_fog_plane[3]) /
                rasterizer_fog_atmospheric_max_distance);
            float depth = relative[2] * rasterizer_camera_forward[2] + relative[1] * rasterizer_camera_forward[1] +
                relative[0] * rasterizer_camera_forward[0];
            float density = environment_clamp01((depth - rasterizer_fog_atmospheric_min_distance) /
                (rasterizer_fog_atmospheric_max_distance - rasterizer_fog_atmospheric_min_distance)) *
                rasterizer_fog_atmospheric_max_density;
            float remainder[3];
            int32_t i;

            if (rasterizer_fog_flags & 2) {
                height = 1.0f;
            }
            fog[0] = 1.0f - density;
            remainder[0] = rasterizer_fog_planar_color.red - (height * rasterizer_fog_planar_color.red +
                (1.0f - height) * rasterizer_fog_atmospheric_color.red) * density;
            remainder[1] = rasterizer_fog_planar_color.green - (rasterizer_fog_atmospheric_color.green * (1.0f - height) +
                height * rasterizer_fog_planar_color.green) * density;
            remainder[2] = rasterizer_fog_planar_color.blue - (rasterizer_fog_planar_color.blue * height +
                (1.0f - height) * rasterizer_fog_atmospheric_color.blue) * density;
            for (i = 0; i < 3; i++) {
                negative[i] = environment_clamp01(-remainder[i]);
                fog[1 + i] = environment_clamp01(remainder[i]);
            }
            add[0] = density * rasterizer_fog_atmospheric_color.red;
            add[1] = rasterizer_fog_atmospheric_color.green * density;
            add[2] = rasterizer_fog_atmospheric_color.blue * density;
            if (rasterizer_device_version < 0xffff0104) {
                if (vertex_shader != 0x19) {
                    environment_set_render_state(0x22, color_rgb_float_to_int(&rasterizer_fog_atmospheric_color));
                } else {
                    for (i = 0; i < 3; i++) {
                        add[i] = environment_clamp01(add[i] - unknown_007c047c * negative[i]);
                    }
                    environment_set_render_state(0x22, color_pack_argb_from_real((ColorARGB *)fog));
                }
            }
        }
    }

    if (environment_effect_slot.constant_handles != 0) {
        uint32_t *handles = (uint32_t *)(uintptr_t)environment_effect_slot.constant_handles;

        environment_set_vector(effect, handles[0], 1.0f, 1.0f, 1.0f, 1.0f);
        environment_set_vector(effect, handles[1], fog[1], fog[2], fog[3], fog[0]);
        environment_set_vector(effect, handles[2], negative[0], negative[1], negative[2], 1.0f);
        environment_set_vector(effect, handles[3], add[0], add[1], add[2], 1.0f);
    }

    if (render_device().set_vertex_shader(rasterizer_vertex_shaders[vertex_shader].shader) < 0) {
        draw_ok = 0;
    }
    if (render_device().set_vertex_declaration(rasterizer_model_vertex_declaration) >= 0 &&
        draw_ok) {
        render_device().effect_begin(effect, &passes, 3);
        for (pass = 0; pass < passes; pass++) {
            render_device().effect_pass(effect, pass);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                dynamic_vertex_slot);
        }
        render_device().effect_end(effect);
    }
    rasterizer_clear_decal_zbias();
}

}  // namespace rasterizer_shader_environment_draw_pixel_shader_impl

namespace rasterizer_shader_environment_draw_single_stream_impl {




typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#undef DEVICE_CALL
#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_single_stream. The original author notes are
 * in docs/original/rasterizer/rasterizer_shader_environment_draw_single_stream.c.txt.
 *
 * @address 0x5276c0
 */
void rasterizer_shader_environment_draw_single_stream(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    float matrix[16];

    if (console_debug_toggle_6893ec == 0) {
        return;
    }
    if (((uint8_t *)rasterizer_active_model_context)[0] & 8) {
        set_render_state(7, 0);
    } else {
        set_render_state(7, 1);
        set_render_state(0xe, 1);
        set_render_state(0x17, 4);
        rasterizer_clear_decal_zbias();
    }
    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 0);
    set_render_state(0x13, 5);
    set_render_state(0x14, 6);
    set_render_state(0xab, 1);
    set_render_state(0xf, shader[0x28] & 1);
    set_render_state(0x18, 0x7f);
    set_render_state(0x1c, rasterizer_fog_enabled != 0);

    memset(matrix, 0, sizeof matrix);
    matrix[0] = *(float *)(((uint8_t *)rasterizer_active_model_context) + 0xc4);
    matrix[5] = *(float *)(((uint8_t *)rasterizer_active_model_context) + 0xc8);
    matrix[10] = 0.0f;
    matrix[15] = 1.0f;

    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);

    if (*(uint32_t *)((uint8_t *)rasterizer_active_model_context) & 0x200) {
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[14].declaration);
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame);
        render_device().set_transform(0x10, matrix);
        set_texture_stage_state(0, 0x18, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
        rasterizer_clear_decal_zbias();
        return;
    }
    {
        rasterizer_vertex_buffer processed = *vertex_buffer;
        uint32_t handle = 0;

        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[4].declaration);
        if (index_buffer != 0) {
            handle = rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer);
        }
        processed.hardware_buffer = handle;
        processed.type = 0xf;
        set_texture_stage_state(0, 0x18, 2);
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[15].declaration);
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame);
        render_device().set_transform(0x10, matrix);
        set_texture_stage_state(0, 0x18, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
        rasterizer_clear_decal_zbias();
    }
}
#undef DEVICE_CALL

}  // namespace rasterizer_shader_environment_draw_single_stream_impl

namespace rasterizer_shader_environment_dynamic_mirror_draw_impl {









static float real_negate_pinned(float x)
{
    float value = 0.5f - x * 0.5f;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return value + value - 1.0f;
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_dynamic_mirror_draw. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_dynamic_mirror_draw.c.txt.
 *
 * Registers: EAX -> shader
 *
 * @address 0x520e50
 */
void rasterizer_shader_environment_dynamic_mirror_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    uint32_t reflection_type;
    int16_t effect_index;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    uint32_t bump_map_tag;
    uint32_t normalization_tag;
    float constants[12];
    float vectors[12];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f9 == 0 ||
        rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1) {
        return;
    }

    reflection_type = *(uint16_t *)&((struct ShaderEnvironment *)raw)->reflection_type;
    if (reflection_type == 0 || reflection_type == 2) {
        if ((raw[0x28] & 2) != 0) {
            reflection_type = 1;
        }
        if (*(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id == 0xffffffff) {
            reflection_type = 1;
        }
    }
    if ((raw[0x2d0] & 1) == 0) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->perpendicular_brightness > 0.0f) && !(((struct ShaderEnvironment *)raw)->parallel_brightness > 0.0f)) {
        return;
    }

    switch ((int16_t)reflection_type) {
    case 0:
    case 2:
        effect_index = 0x25;
        break;
    case 1:
        effect_index = (raw[0x28] & 2) != 0 ? 0x27 : 0x26;
        break;
    default:
        effect_index = 0;
        break;
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }

    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
    bump_map_tag = *(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    if (bump_map_tag == 0xffffffff) {
        chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    } else {
        BitmapData *bump_bitmap = 0;

        if (console_debug_toggle_689409 != 0) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & 0xffff].data;
            int32_t count = (int32_t)bitmap->bitmap_data.count;

            if (count > 0) {
                bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
                if (*(int16_t *)&((struct BitmapData *)bump_bitmap)->type != 0) {
                    bump_bitmap = 0;
                }
            }
        }
        if (bump_bitmap == 0) {
            uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

            if (default_tag != 0xffffffff) {
                Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & 0xffff].data;

                if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                    bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
                }
            }
        }
        if (bump_bitmap != 0) {
            rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
            rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
            rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
        }
        chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 1, 0, effect_slot);
    }

    render_device().effect_set_texture(effect, effect_slot->texture_handles[3], (void *)rasterizer_render_targets[2].texture);

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 320.0f;
    constants[3] = 240.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    render_device().set_vertex_shader_constant_f(0xa, constants, 3);

    vectors[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
    vectors[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
    vectors[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
    vectors[3] = (raw[0x28] & 2) != 0 ? -1.0f : 0.0f;
    vectors[4] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;
    vectors[5] = *(const float *)(raw + 0x2ac);
    vectors[6] = *(const float *)(raw + 0x2b0);
    vectors[7] = ((struct ShaderEnvironment *)raw)->perpendicular_brightness;
    vectors[8] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;
    vectors[9] = *(const float *)(raw + 0x2b8);
    vectors[10] = *(const float *)(raw + 0x2bc);
    vectors[11] = ((struct ShaderEnvironment *)raw)->parallel_brightness;
    if (effect_slot->constant_handles != 0) {
        const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;

        render_device().effect_set_vector(effect, handles[0], &vectors[0]);
        render_device().effect_set_vector(effect, handles[1], &vectors[4]);
        render_device().effect_set_vector(effect, handles[2], &vectors[8]);
    }

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_dynamic_mirror_draw_impl

namespace rasterizer_shader_environment_lightmap_draw_impl {





/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_lightmap_draw.c.txt.
 *
 * @address 0x51e2a0
 */
void rasterizer_shader_environment_lightmap_draw(uint8_t *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer)
{
    rasterizer_effect_slot *slot;
    int16_t index;
    int16_t size[4][2];
    float su1 = 1.0f, sv1 = 1.0f, su2 = 1.0f, sv2 = 1.0f, su3 = 1.0f, sv3 = 1.0f;
    float constants[12];
    void *effect;
    uint32_t passes;
    uint32_t pass;

    if (!console_debug_toggle_6893f4) {
        return;
    }
    index = (int16_t)(*(uint16_t *)(shader + 0x2a) * 3 + *(uint16_t *)(shader + 0xb0));
    index = (int16_t)((uint16_t)(index * 3) + *(uint16_t *)(shader + 0xf4) + 5);
    slot = &rasterizer_effects[index];
    if (slot->effect == 0) {
        return;
    }
    *(uint32_t *)size[0] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x94), 0, 0, 1, frame, slot);
    *(uint32_t *)size[1] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xc4), 0, 1, 2, frame, slot);
    *(uint32_t *)size[2] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0xd8), 0, 2, 2, frame, slot);
    *(uint32_t *)size[3] = *(uint32_t *)rasterizer_resolve_and_cache_submap_b(*(uint32_t *)(shader + 0x108), 0, 3, 2, frame, slot);
    if (shader[0x6c] & 1) {
        float base_width = (float)size[0][0];
        float base_height = (float)size[0][1];

        su1 = base_width / (float)size[1][0];
        sv1 = base_height / (float)size[1][1];
        su2 = base_width / (float)size[2][0];
        sv2 = base_height / (float)size[2][1];
        su3 = base_width / (float)size[3][0];
        sv3 = base_height / (float)size[3][1];
    }
    constants[0] = su1 * *(float *)(shader + 0xb4);
    constants[1] = sv1 * *(float *)(shader + 0xb4);
    constants[2] = su2 * *(float *)(shader + 0xc8);
    constants[3] = sv2 * *(float *)(shader + 0xc8);
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = su3 * *(float *)(shader + 0xf8);
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = sv3 * *(float *)(shader + 0xf8);
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], (*(double *)&rasterizer_time), const_cast<ShaderEnvironment *>((const ShaderEnvironment *)shader));

    render_device().set_vertex_shader_constant_f(10, constants, 3);
    render_device().set_vertex_declaration((*(uint32_t *)&rasterizer_vertex_declarations));
    render_device().set_vertex_shader(rasterizer_vertex_shaders[slot->vertex_shader_index].shader);

    effect = (void *)(uintptr_t)slot->effect;
    render_device().effect_begin(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot,
            first_primitive);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_lightmap_draw_impl

namespace rasterizer_shader_environment_lightmap_draw_single_stream_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#undef DEVICE_CALL
#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_draw_single_stream. The original author
 * notes are in docs/original/rasterizer/rasterizer_shader_environment_lightmap_draw_single_stream.c.txt.
 *
 * @address 0x51e8f0
 */
void rasterizer_shader_environment_lightmap_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    d3d_call3_fn set_texture_stage_state;

    if (console_debug_toggle_6893f4 == 0) {
        return;
    }
    chimera__rasterizer_set_texture(*(uint32_t *)&((struct ShaderEnvironment *)shader)->base_map.tag_id, 0, 0, 1, frame);
    render_device().set_vertex_shader(0);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[19].declaration);
    render_device().set_pixel_shader(0);
    set_texture_stage_state = (d3d_call3_fn)DEVICE_CALL(0x10c);
    set_texture_stage_state(rasterizer_device, 0, 1, 2);
    set_texture_stage_state(rasterizer_device, 0, 2, 2);
    set_texture_stage_state(rasterizer_device, 0, 4, 2);
    set_texture_stage_state(rasterizer_device, 0, 5, 1);
    set_texture_stage_state(rasterizer_device, 1, 1, 1);
    set_texture_stage_state(rasterizer_device, 1, 4, 1);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
}
#undef DEVICE_CALL

}  // namespace rasterizer_shader_environment_lightmap_draw_single_stream_impl

namespace rasterizer_shader_environment_lightmap_draw_two_stream_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#undef DEVICE_CALL
#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_draw_two_stream. The original author
 * notes are in docs/original/rasterizer/rasterizer_shader_environment_lightmap_draw_two_stream.c.txt.
 *
 * @address 0x51e570
 */
void rasterizer_shader_environment_lightmap_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    int16_t effect_index;
    int16_t *base_size;
    int16_t base_width, base_height;

    if (console_debug_toggle_6893f4 == 0) {
        return;
    }
    effect_index = (int16_t)((uint16_t)(((uint16_t)(*(uint16_t *)&((struct ShaderEnvironment *)raw)->shader_environment_type * 3) + *(uint16_t *)&((struct ShaderEnvironment *)raw)->detail_map_function) * 3) +
        *(uint16_t *)&((struct ShaderEnvironment *)raw)->micro_detail_map_function + 5);
    if (rasterizer_effects[effect_index].effect == 0) {
        return;
    }

    base_size = chimera__rasterizer_set_texture(*(uint32_t *)&((struct ShaderEnvironment *)raw)->base_map.tag_id, 0, 0, 1, frame);
    base_width = base_size[0];
    base_height = base_size[1];
    render_device().set_vertex_shader(0);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[12].declaration);
    render_device().set_pixel_shader(0);

    if (*(int32_t *)&((struct ShaderEnvironment *)raw)->primary_detail_map.tag_id != -1) {
        int16_t *detail_size = chimera__rasterizer_set_texture(*(uint32_t *)&((struct ShaderEnvironment *)raw)->primary_detail_map.tag_id, 1, 0, 2, frame);
        float matrix[16];

        memset(matrix, 0, sizeof matrix);
        matrix[0] = (float)(int32_t)base_width / (float)(int32_t)detail_size[0] * ((struct ShaderEnvironment *)raw)->primary_detail_map_scale;
        matrix[5] = (float)(int32_t)base_height / (float)(int32_t)detail_size[1] * ((struct ShaderEnvironment *)raw)->primary_detail_map_scale;
        matrix[10] = 1.0f;
        matrix[15] = 1.0f;
        set_texture_stage_state(1, 0x18, 2);
        render_device().set_transform(0x11, matrix);
        set_texture_stage_state(1, 0xb, 0);
        set_texture_stage_state(0, 1, 2);
        set_texture_stage_state(0, 2, 2);
        set_texture_stage_state(0, 4, 2);
        set_texture_stage_state(0, 5, 1);
        set_texture_stage_state(1, 1, 4);
        set_texture_stage_state(1, 2, 2);
        set_texture_stage_state(1, 3, 1);
        set_texture_stage_state(1, 4, 2);
        set_texture_stage_state(1, 5, 2);
        set_texture_stage_state(2, 1, 1);
        set_texture_stage_state(2, 4, 1);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
        set_texture_stage_state(1, 0x18, 0);
        set_texture_stage_state(1, 0xb, 1);
        return;
    }
    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 1);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
}
#undef DEVICE_CALL

}  // namespace rasterizer_shader_environment_lightmap_draw_two_stream_impl

namespace rasterizer_shader_environment_lightmap_specular_draw_impl {








/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_specular_draw. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_lightmap_specular_draw.c.txt.
 *
 * Registers: EAX -> shader
 *
 * @address 0x521f90
 */
void rasterizer_shader_environment_lightmap_specular_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    uint16_t specular_flags;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float specular_exponent;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap;
    uint32_t normalization_tag;
    float constants[12];
    float pixel_constants[16];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f7 == 0 || render_force_flag != 0 ||
        rasterizer_lightmap_bitmap_missing != 0 || rasterizer_caps.pixel_shader_version < 0xffff0104) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->brightness > 0.0f)) {
        return;
    }
    specular_flags = *(uint16_t *)&((struct ShaderEnvironment *)raw)->specular_flags;
    if ((specular_flags & 4) == 0) {
        return;
    }
    effect_slot = (raw[0x28] & 2) != 0 ? &rasterizer_effects[42] : &rasterizer_effects[43];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }
    specular_exponent = (specular_flags & 1) != 0 ? 4.0f : 2.0f;

    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    bump_map_tag = *(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    bump_bitmap = 0;
    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bump_bitmap)->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

        if (default_tag != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & 0xffff].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }

    if (rasterizer_lightmap_bitmap_missing == 0) {
        rasterizer_bind_texture_d3dx(1, rasterizer_lightmap_bitmap, effect_slot);
    } else {
        render_device().set_texture(1, 0);
    }
    normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 3, 0, effect_slot);

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    render_device().set_vertex_shader_constant_f(0xa, constants, 3);

    pixel_constants[0] = ((struct ShaderEnvironment *)raw)->brightness;
    pixel_constants[1] = pixel_constants[0];
    pixel_constants[2] = pixel_constants[0];
    pixel_constants[3] = pixel_constants[0];
    pixel_constants[4] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;
    pixel_constants[5] = *(const float *)(raw + 0x2ac);
    pixel_constants[6] = *(const float *)(raw + 0x2b0);
    pixel_constants[7] = 1.0f;
    pixel_constants[8] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;
    pixel_constants[9] = *(const float *)(raw + 0x2b8);
    pixel_constants[10] = *(const float *)(raw + 0x2bc);
    pixel_constants[11] = 1.0f;
    pixel_constants[12] = specular_exponent;
    pixel_constants[13] = specular_exponent;
    pixel_constants[14] = specular_exponent;
    pixel_constants[15] = specular_exponent;
    render_device().set_pixel_shader_constant_f(0, pixel_constants, 3);

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + 1);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_lightmap_specular_draw_impl

namespace rasterizer_shader_environment_projected_light_draw_impl {








static void rasterizer_bind_bump_map(uint32_t bump_map_tag, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    BitmapData *bump_bitmap = 0;

    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bump_bitmap)->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

        if (default_tag == 0xffffffff) {
            return;
        }
        {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & 0xffff].data;

            if (bitmap == 0 || (int32_t)bitmap->bitmap_data.count <= 3) {
                return;
            }
            bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
        }
    }
    rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
    rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
    rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_projected_light_draw. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_projected_light_draw.c.txt.
 *
 * @address 0x521900
 */
void rasterizer_shader_environment_projected_light_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float specular_exponent;
    uint32_t normalization_tag;
    float constants[12];
    float vector[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f6 == 0 ||
        rasterizer_caps.pixel_shader_version < 0xffff0104) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->brightness > 0.0f) || !(rasterizer_projected_light_luminance > 0.0f)) {
        return;
    }
    effect_slot = (raw[0x28] & 2) != 0 ? &rasterizer_effects[40] : &rasterizer_effects[41];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }
    specular_exponent = (raw[0x27c] & 1) != 0 ? 4.0f : 2.0f;

    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index + rasterizer_projected_light_shader_variant].shader);
    render_device().set_vertex_shader_constant_f(0xd, (const float *)&rasterizer_projected_light, 5);

    rasterizer_bind_bump_map(*(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id, frame, effect_slot);
    if (rasterizer_projected_light_has_cube_map == 1) {
        rasterizer_resolve_and_cache_submap_b(rasterizer_projected_light_cube_map, 2, 1, 1, 0, effect_slot);
    } else {
        chimera__rasterizer_set_texture_direct_d3dx(rasterizer_projected_light_cube_map, 1, 0, effect_slot);
    }
    normalization_tag = *(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id;
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 3, 0, effect_slot);

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    render_device().set_vertex_shader_constant_f(0xa, constants, 3);

    if (effect_slot->constant_handles != 0) {
        const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;
        float light = rasterizer_projected_light_luminance * ((struct ShaderEnvironment *)raw)->brightness;

        vector[0] = light;
        vector[1] = light;
        vector[2] = light;
        vector[3] = light;
        render_device().effect_set_vector(effect, handles[0], vector);
        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;
        vector[1] = *(const float *)(raw + 0x2ac);
        vector[2] = *(const float *)(raw + 0x2b0);
        vector[3] = 1.0f;
        render_device().effect_set_vector(effect, handles[1], vector);
        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;
        vector[1] = *(const float *)(raw + 0x2b8);
        vector[2] = *(const float *)(raw + 0x2bc);
        vector[3] = 1.0f;
        render_device().effect_set_vector(effect, handles[2], vector);
        vector[0] = specular_exponent;
        vector[1] = specular_exponent;
        vector[2] = specular_exponent;
        vector[3] = specular_exponent;
        render_device().effect_set_vector(effect, handles[3], vector);
    }

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_projected_light_draw_impl

namespace rasterizer_shader_environment_reflection_draw_impl {








static float real_negate_pinned(float x)
{
    float value = 0.5f - x * 0.5f;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return value + value - 1.0f;
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_reflection_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_reflection_draw.c.txt.
 *
 * @address 0x5202f0
 */
void rasterizer_shader_environment_reflection_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    uint32_t reflection_type;
    int16_t effect_index;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap;
    float constants[12];
    float vector[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893fa == 0 ||
        rasterizer_caps.pixel_shader_version < 0xffff0101) {
        return;
    }

    reflection_type = *(uint16_t *)&((struct ShaderEnvironment *)raw)->reflection_type;
    if (reflection_type == 0 || reflection_type == 2) {
        if ((raw[0x28] & 2) != 0) {
            reflection_type = 1;
        }
        if (*(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id == 0xffffffff) {
            reflection_type = 1;
        }
    }
    if (!(((struct ShaderEnvironment *)raw)->perpendicular_brightness > 0.0f) && !(((struct ShaderEnvironment *)raw)->parallel_brightness > 0.0f)) {
        return;
    }
    if (*(uint32_t *)&((struct ShaderEnvironment *)raw)->reflection_cube_map.tag_id == 0xffffffff) {
        return;
    }

    switch (reflection_type) {
    case 0:
    case 2:
        effect_index = 0x20;
        break;
    case 1:
        effect_index = (raw[0x28] & 2) != 0 ? 0x22 : 0x21;
        break;
    default:
        effect_index = 0;
        break;
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }

    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    bump_map_tag = *(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    bump_bitmap = 0;
    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bump_bitmap)->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

        if (default_tag != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & 0xffff].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }
    chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 1, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 2, 0, effect_slot);
    rasterizer_resolve_and_cache_submap_b(*(uint32_t *)&((struct ShaderEnvironment *)raw)->reflection_cube_map.tag_id, 2, 3, 0, frame, effect_slot);

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 320.0f;
    constants[3] = 240.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    render_device().set_vertex_shader_constant_f(0xa, constants, 3);

    if (effect_slot->constant_handles != 0) {
        const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;

        vector[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
        vector[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
        vector[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
        vector[3] = 0.0f;
        render_device().effect_set_vector(effect, handles[0], vector);

        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->perpendicular_color;
        vector[1] = *(const float *)(raw + 0x2ac);
        vector[2] = *(const float *)(raw + 0x2b0);
        vector[3] = ((struct ShaderEnvironment *)raw)->perpendicular_brightness;
        render_device().effect_set_vector(effect, handles[1], vector);

        vector[0] = *(float *)&((struct ShaderEnvironment *)raw)->parallel_color;
        vector[1] = *(const float *)(raw + 0x2b8);
        vector[2] = *(const float *)(raw + 0x2bc);
        vector[3] = ((struct ShaderEnvironment *)raw)->parallel_brightness;
        render_device().effect_set_vector(effect, handles[2], vector);
    }

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        int32_t lightmap_stream;

        render_device().effect_pass(effect, pass);
        lightmap_stream = (render_force_flag != 0 && (int16_t)reflection_type == 2) ? 1 : 0;
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + lightmap_stream);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_reflection_draw_impl

/**
 * Direct3D 9 back end function rasterizer_shader_environment_select_draw_functions. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_select_draw_functions.c.txt.
 *
 * @address 0x52b630
 */
void rasterizer_shader_environment_select_draw_functions(void)
{
    if ((int32_t)rasterizer_caps.max_streams <= 1) {
        shader_environment_draw_simple = (void *)rasterizer_shader_environment_draw_single_stream;
        shader_environment_draw = (void *)rasterizer_shader_model_draw_limited;
        return;
    }
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        shader_environment_draw_simple = (void *)rasterizer_shader_environment_draw_fixed_function;
        shader_environment_draw = (void *)rasterizer_shader_model_draw_fixed_function;
        return;
    }
    shader_environment_draw_simple = (void *)rasterizer_shader_environment_draw_pixel_shader;
    shader_environment_draw = (void *)rasterizer_shader_model_draw_pixel_shader;
}

namespace rasterizer_shader_environment_self_illumination_draw_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);









static void rasterizer_set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static float shader_field(const uint8_t *raw, uint32_t offset)
{
    return *(const float *)(raw + offset);
}

static float self_illumination_animation(const uint8_t *raw, uint32_t offset)
{
    return halo::math::periodic_function_evaluate((periodic_function_t)*(const int16_t *)(raw + offset),
                                      (shader_field(raw, offset + 8) + rasterizer_time.time) / shader_field(raw, offset + 4));
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_self_illumination_draw. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_environment_self_illumination_draw.c.txt.
 *
 * @address 0x51f3e0
 */
void rasterizer_shader_environment_self_illumination_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    int16_t effect_index;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    uint32_t map_tag;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap;
    float constants[12];
    float primary, secondary, plasma, a, b;
    float primary_color[3], secondary_color[3];
    float material[4];
    float vectors[20];
    uint32_t pass_count;
    uint32_t pass;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }

    render_device().set_render_state(0xf, (raw[0x28] & 1) != 0 && console_debug_toggle_68941c != 0 ? 1 : 0);

    map_tag = *(uint32_t *)&((struct ShaderEnvironment *)raw)->map.tag_id;
    if (map_tag == 0xffffffff) {
        effect_index = (int16_t)(2 + (unknown_006e0a04 != 0 ? 1 : 0));
    } else {
        effect_index = (int16_t)(unknown_006e0a04 != 0 ? 1 : 0);
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = (void *)effect_slot->effect;
    if (effect == 0) {
        return;
    }

    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[13].shader);

    bump_map_tag = (raw[0x28] & 2) != 0 ? 0xffffffff : *(uint32_t *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    bump_bitmap = 0;
    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bump_bitmap)->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

        if (default_tag != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & 0xffff].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }

    rasterizer_resolve_and_cache_submap_b(map_tag, 0, 1, 0, frame, effect_slot);
    if ((raw[0x180] & 1) != 0) {
        rasterizer_set_sampler_state(1, 5, 1);
        rasterizer_set_sampler_state(1, 6, 1);
        rasterizer_set_sampler_state(1, 7, 1);
    } else {
        rasterizer_set_sampler_state(1, 5, 2);
        rasterizer_set_sampler_state(1, 6, 2);
        rasterizer_set_sampler_state(1, 7, 2);
    }

    if (rasterizer_environment_lightmap != 0) {
        rasterizer_bind_texture_d3dx(2, rasterizer_environment_lightmap, effect_slot);
    } else {
        render_device().effect_set_texture(effect, effect_slot->texture_handles[2], rasterizer_capture_surfaces[0]);
    }
    chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 3, 0,
                                                effect_slot);

    constants[0] = shader_field(raw, 0x138);
    constants[1] = shader_field(raw, 0x13c);
    constants[2] = shader_field(raw, 0x250);
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    render_device().set_vertex_shader_constant_f(0xa, constants, 3);
    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[13].shader);

    primary = self_illumination_animation(raw, 0x1b4);
    secondary = self_illumination_animation(raw, 0x1f0);
    plasma = self_illumination_animation(raw, 0x22c);
    a = 1.0f - primary;
    b = 1.0f - secondary;
    primary_color[0] = primary * shader_field(raw, 0x19c) + a * shader_field(raw, 0x1a8);
    primary_color[1] = primary * shader_field(raw, 0x1a0) + a * shader_field(raw, 0x1ac);
    primary_color[2] = primary * shader_field(raw, 0x1a4) + a * shader_field(raw, 0x1b0);
    secondary_color[0] = secondary * shader_field(raw, 0x1d8) + b * shader_field(raw, 0x1e4);
    secondary_color[1] = secondary * shader_field(raw, 0x1dc) + b * shader_field(raw, 0x1e8);
    secondary_color[2] = secondary * shader_field(raw, 0x1e0) + b * shader_field(raw, 0x1ec);

    material[0] = shader_field(raw, 0x10c);
    material[1] = shader_field(raw, 0x110);
    material[2] = shader_field(raw, 0x114);
    material[3] = 1.0f;
    if (effect_slot->constant_handles != 0) {
        render_device().effect_set_vector(effect, ((const uint32_t *)effect_slot->constant_handles)[0], material);
    }

    if (effect_index == 0) {
        vectors[0] = plasma;
        vectors[1] = plasma;
        vectors[2] = plasma;
        vectors[3] = 0.5f - plasma;
        vectors[4] = primary_color[0];
        vectors[5] = primary_color[1];
        vectors[6] = primary_color[2];
        vectors[7] = 1.0f;
        vectors[8] = secondary_color[0];
        vectors[9] = secondary_color[1];
        vectors[10] = secondary_color[2];
        vectors[11] = 1.0f;
        vectors[12] = shader_field(raw, 0x214);
        vectors[13] = shader_field(raw, 0x218);
        vectors[14] = shader_field(raw, 0x21c);
        vectors[15] = 1.0f;
        vectors[16] = shader_field(raw, 0x220);
        vectors[17] = shader_field(raw, 0x224);
        vectors[18] = shader_field(raw, 0x228);
        vectors[19] = 1.0f;
        if (effect_slot->constant_handles != 0) {
            const uint32_t *handles = (const uint32_t *)effect_slot->constant_handles;
            int32_t i;

            for (i = 0; i < 5; i++) {
                render_device().effect_set_vector(effect, handles[1 + i], &vectors[i * 4]);
            }
        }
    }

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + (unknown_006e0a04 == 0 ? 1 : 0));
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_self_illumination_draw_impl

namespace rasterizer_shader_environment_self_illumination_draw_single_stream_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#undef DEVICE_CALL
#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_self_illumination_draw_single_stream. The
 * original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_self_illumination_draw_single_stream.c.txt.
 *
 * @address 0x51fd80
 */
void rasterizer_shader_environment_self_illumination_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    datum_index self_illumination;
    BitmapData *bitmap = 0;
    uint32_t colour;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    render_device().set_render_state(0xf, (raw[0x28] & 1) != 0 && console_debug_toggle_68941c != 0);
    render_device().set_vertex_shader(0);

    colour = 0xffffff00u | (uint32_t)(int32_t)(*(float *)&((struct ShaderEnvironment *)raw)->material_color * 255.0f);
    colour = (colour << 8) | ((uint32_t)(int32_t)(*(float *)(raw + 0x110) * 255.0f) & 0xff);
    colour = (colour << 8) | ((uint32_t)(int32_t)(*(float *)(raw + 0x114) * 255.0f) & 0xff);
    render_device().set_render_state(0x3c, colour);

    self_illumination = (raw[0x28] & 2) ? k_datum_index_none : *(datum_index *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    if (console_debug_toggle_689409 != 0 && self_illumination != k_datum_index_none) {
        int32_t count = *(int32_t *)((uint8_t *)halo::cache::globals().tag_instances[self_illumination & 0xffff].data + 0x60);

        if (count > 0) {
            bitmap = bitmap_group_get_bitmap_data(self_illumination, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bitmap)->type != 0) {
                bitmap = 0;
            }
        }
    }
    if (bitmap == 0) {
        datum_index fallback = *(datum_index *)((uint8_t *)rasterizer_globals_data + 0xb8);

        if (fallback != k_datum_index_none) {
            uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[fallback & 0xffff].data;

            if (tag != 0 && *(int32_t *)(tag + 0x60) > 3) {
                bitmap = (BitmapData *)(*(uint8_t **)(tag + 0x64) + 0x90);
            }
        }
    }
    if (bitmap != 0) {
        rasterizer_bind_texture_d3d9(0, bitmap);
        rasterizer_bound_bitmap_size_a[0] = *(int16_t *)&((struct BitmapData *)bitmap)->width;
        rasterizer_bound_bitmap_size_a[1] = *(int16_t *)&((struct BitmapData *)bitmap)->height;
    }

    {
        uint32_t texture;

        if (rasterizer_environment_lightmap != 0) {
            halo::cache::texture_cache_get(rasterizer_environment_lightmap, 1, 1);
            texture = *(uint32_t *)((uint8_t *)rasterizer_environment_lightmap + 0x28);
        } else {
            texture = (uint32_t)rasterizer_capture_surfaces[0];
        }
        render_device().set_texture(1, texture);
    }
    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 0);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(1, 1, 7);
    set_texture_stage_state(1, 2, 2);
    set_texture_stage_state(1, 3, 1);
    set_texture_stage_state(1, 4, 2);
    set_texture_stage_state(1, 5, 1);
    set_texture_stage_state(2, 1, 1);
    set_texture_stage_state(2, 4, 1);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[19].declaration);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
}
#undef DEVICE_CALL

}  // namespace rasterizer_shader_environment_self_illumination_draw_single_stream_impl

namespace rasterizer_shader_environment_self_illumination_draw_two_stream_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#undef DEVICE_CALL
#define DEVICE_CALL(offset) ((*(void ***)rasterizer_device)[(offset) / 4])

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_self_illumination_draw_two_stream. The original
 * author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_self_illumination_draw_two_stream.c.txt.
 *
 * @address 0x51fad0
 */
void rasterizer_shader_environment_self_illumination_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    datum_index self_illumination;
    BitmapData *bitmap = 0;
    uint32_t colour;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    render_device().set_render_state(0xf, (raw[0x28] & 1) != 0 && console_debug_toggle_68941c != 0);
    render_device().set_vertex_shader(0);

    colour = 0xffffff00u | (uint32_t)(int32_t)(*(float *)&((struct ShaderEnvironment *)raw)->material_color * 255.0f);
    colour = (colour << 8) | ((uint32_t)(int32_t)(*(float *)(raw + 0x110) * 255.0f) & 0xff);
    colour = (colour << 8) | ((uint32_t)(int32_t)(*(float *)(raw + 0x114) * 255.0f) & 0xff);
    render_device().set_render_state(0x3c, colour);

    self_illumination = (raw[0x28] & 2) ? k_datum_index_none : *(datum_index *)&((struct ShaderEnvironment *)raw)->bump_map.tag_id;
    if (console_debug_toggle_689409 != 0 && self_illumination != k_datum_index_none) {
        int32_t count = *(int32_t *)((uint8_t *)halo::cache::globals().tag_instances[self_illumination & 0xffff].data + 0x60);

        if (count > 0) {
            bitmap = bitmap_group_get_bitmap_data(self_illumination, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)&((struct BitmapData *)bitmap)->type != 0) {
                bitmap = 0;
            }
        }
    }
    if (bitmap == 0) {
        datum_index fallback = *(datum_index *)((uint8_t *)rasterizer_globals_data + 0xb8);

        if (fallback != k_datum_index_none) {
            uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[fallback & 0xffff].data;

            if (tag != 0 && *(int32_t *)(tag + 0x60) > 3) {
                bitmap = (BitmapData *)(*(uint8_t **)(tag + 0x64) + 0x90);
            }
        }
    }
    if (bitmap != 0) {
        rasterizer_bind_texture_d3d9(0, bitmap);
        rasterizer_bound_bitmap_size_a[0] = *(int16_t *)&((struct BitmapData *)bitmap)->width;
        rasterizer_bound_bitmap_size_a[1] = *(int16_t *)&((struct BitmapData *)bitmap)->height;
    }

    {
        uint32_t texture;

        if (rasterizer_environment_lightmap != 0) {
            halo::cache::texture_cache_get(rasterizer_environment_lightmap, 1, 1);
            texture = *(uint32_t *)((uint8_t *)rasterizer_environment_lightmap + 0x28);
        } else {
            texture = (uint32_t)rasterizer_capture_surfaces[0];
        }
        render_device().set_texture(1, texture);
    }
    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 0);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(1, 1, 7);
    set_texture_stage_state(1, 2, 2);
    set_texture_stage_state(1, 3, 1);
    set_texture_stage_state(1, 4, 2);
    set_texture_stage_state(1, 5, 1);
    set_texture_stage_state(2, 1, 1);
    set_texture_stage_state(2, 4, 1);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[13].declaration);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot,
        first_primitive, (rasterizer_vertex_buffer *)((uint8_t *)vertex_buffer + (unknown_006e0a04 == 0 ? 20 : 0)));
}
#undef DEVICE_CALL

}  // namespace rasterizer_shader_environment_self_illumination_draw_two_stream_impl

/**
 * Direct3D 9 back end function rasterizer_shader_environment_set_lightmap. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_set_lightmap.c.txt.
 *
 * Registers: EAX = lightmap
 *
 * @address 0x520910
 */
void rasterizer_shader_environment_set_lightmap(BitmapData *lightmap)
{
    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f8 != 0 &&
        console_debug_toggle_6893fa != 0 && render_force_flag == 0 &&
        rasterizer_active_environment_effect != 0 &&
        rasterizer_active_environment_effect->effect != 0) {
        if (lightmap != 0) {
            rasterizer_bind_texture_d3dx(0, lightmap, rasterizer_active_environment_effect);
            rasterizer_environment_lightmap_missing = 0;
            return;
        }
        rasterizer_environment_lightmap_missing = 1;
    }
}

namespace rasterizer_shader_environment_technique_draw_impl {







/**
 * Direct3D 9 back end function rasterizer_shader_environment_technique_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_technique_draw.c.txt.
 *
 * Registers: EAX -> vertex_buffer, ECX -> shader
 *
 * @address 0x520970
 */
void rasterizer_shader_environment_technique_draw(rasterizer_vertex_buffer *vertex_buffer, const ShaderEnvironment *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    const uint8_t *raw = (const uint8_t *)shader;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float constants[12];
    float scale[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f8 == 0 || console_debug_toggle_6893fa == 0 ||
        render_force_flag != 0 || rasterizer_environment_lightmap_missing != 0) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->perpendicular_brightness > 0.0f) && !(((struct ShaderEnvironment *)raw)->parallel_brightness > 0.0f)) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->lightmap_brightness_scale < 1.0f)) {
        return;
    }

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    if (render_device().set_vertex_shader_constant_f(0xa, constants, 3) < 0) {
        return;
    }

    effect_slot = rasterizer_active_environment_effect;
    if (effect_slot == 0 || effect_slot->effect == 0) {
        return;
    }
    effect = (void *)effect_slot->effect;
    scale[0] = ((struct ShaderEnvironment *)raw)->lightmap_brightness_scale;
    scale[1] = scale[0];
    scale[2] = scale[0];
    scale[3] = scale[0];
    render_device().set_pixel_shader_constant_f(1, scale, 1);
    render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + 1);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_technique_draw_impl

namespace rasterizer_shader_environment_technique_multipurpose_set_states_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_shader_environment_technique_multipurpose_set_states. The original
 * author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_technique_multipurpose_set_states.c.txt.
 *
 * @address 0x520790
 */
void rasterizer_shader_environment_technique_multipurpose_set_states(void)
{

    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f8 != 0 &&
        console_debug_toggle_6893fa != 0 && render_force_flag == 0) {
        render_device().set_render_state(0x16, 3);
        render_device().set_render_state(0xa8, 8);
        render_device().set_render_state(0x1b, 1);
        render_device().set_render_state(0x13, 7);
        render_device().set_render_state(0x14, 1);
        render_device().set_render_state(0xab, 1);
        render_device().set_render_state(0xf, 0);
        render_device().set_render_state(7, 1);
        render_device().set_render_state(0x17, 3);
        render_device().set_render_state(0xe, 0);
        render_device().set_render_state(0x1c, 0);

        render_device().set_sampler_state(0, 1, 3);
        render_device().set_sampler_state(0, 2, 3);
        render_device().set_sampler_state(0, 5, 2);
        render_device().set_sampler_state(0, 6, 2);
        render_device().set_sampler_state(0, 7, 2);
    }
    rasterizer_active_environment_effect = &rasterizer_effects[36];
}

}  // namespace rasterizer_shader_environment_technique_multipurpose_set_states_impl

namespace rasterizer_shader_environment_technique_ps2_set_states_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_shader_environment_technique_ps2_set_states. The original author
 * notes are in docs/original/rasterizer/rasterizer_shader_environment_technique_ps2_set_states.c.txt.
 *
 * @address 0x5212d0
 */
void rasterizer_shader_environment_technique_ps2_set_states(void)
{

    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f6 == 0 ||
        rasterizer_caps.pixel_shader_version <= 0xffff0103) {
        return;
    }

    render_device().set_render_state(0x16, 3);
    render_device().set_render_state(0xa8, 7);
    render_device().set_render_state(0x1b, 1);
    render_device().set_render_state(0x13, 7);
    render_device().set_render_state(0x14, 2);
    render_device().set_render_state(0xab, 1);
    render_device().set_render_state(0xf, 1);
    render_device().set_render_state(0x18, 0);
    render_device().set_render_state(7, 1);
    render_device().set_render_state(0x17, 3);
    render_device().set_render_state(0xe, 0);
    render_device().set_render_state(0x1c, 0);

    render_device().set_sampler_state(0, 1, 1);
    render_device().set_sampler_state(0, 2, 1);
    render_device().set_sampler_state(0, 5, 2);
    render_device().set_sampler_state(0, 6, 2);
    render_device().set_sampler_state(0, 7, 2);
    render_device().set_sampler_state(1, 1, 3);
    render_device().set_sampler_state(1, 2, 3);
    render_device().set_sampler_state(1, 3, 3);
    render_device().set_sampler_state(1, 5, 2);
    render_device().set_sampler_state(1, 6, 2);
    render_device().set_sampler_state(1, 7, 2);
    render_device().set_sampler_state(2, 1, 3);
    render_device().set_sampler_state(2, 2, 3);
    render_device().set_sampler_state(2, 3, 3);
    render_device().set_sampler_state(2, 5, 2);
    render_device().set_sampler_state(2, 6, 2);
    render_device().set_sampler_state(2, 7, 2);
    render_device().set_sampler_state(3, 1, 3);
    render_device().set_sampler_state(3, 2, 3);
    render_device().set_sampler_state(3, 3, 3);
    render_device().set_sampler_state(3, 5, 2);
    render_device().set_sampler_state(3, 6, 2);
    render_device().set_sampler_state(3, 7, 2);
}

}  // namespace rasterizer_shader_environment_technique_ps2_set_states_impl

namespace rasterizer_shader_environment_technique_self_illumination_set_states_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_shader_environment_technique_self_illumination_set_states. The
 * original author notes are in
 * docs/original/rasterizer/rasterizer_shader_environment_technique_self_illumination_set_states.c.txt.
 *
 * @address 0x520b90
 */
void rasterizer_shader_environment_technique_self_illumination_set_states(void)
{

    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f9 == 0 ||
        rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1) {
        return;
    }

    render_device().set_render_state(0x16, 3);
    render_device().set_render_state(0xa8, 7);
    render_device().set_render_state(0x1b, 1);
    render_device().set_render_state(0x13, 7);
    render_device().set_render_state(0x14, 2);
    render_device().set_render_state(0xab, 1);
    render_device().set_render_state(0xf, 0);
    render_device().set_render_state(7, 1);
    render_device().set_render_state(0x17, 3);
    render_device().set_render_state(0xe, 0);
    render_device().set_render_state(0x1c, 0);

    render_device().set_sampler_state(0, 1, 1);
    render_device().set_sampler_state(0, 2, 1);
    render_device().set_sampler_state(0, 5, 2);
    render_device().set_sampler_state(0, 6, 2);
    render_device().set_sampler_state(0, 7, 2);
    render_device().set_sampler_state(1, 1, 3);
    render_device().set_sampler_state(1, 2, 3);
    render_device().set_sampler_state(1, 3, 3);
    render_device().set_sampler_state(1, 5, 2);
    render_device().set_sampler_state(1, 6, 1);
    render_device().set_sampler_state(1, 7, 1);
    render_device().set_sampler_state(2, 1, 3);
    render_device().set_sampler_state(2, 2, 3);
    render_device().set_sampler_state(2, 3, 3);
    render_device().set_sampler_state(2, 5, 2);
    render_device().set_sampler_state(2, 6, 1);
    render_device().set_sampler_state(2, 7, 1);
    render_device().set_sampler_state(3, 1, 3);
    render_device().set_sampler_state(3, 2, 3);
    render_device().set_sampler_state(3, 5, 2);
    render_device().set_sampler_state(3, 6, 2);
    render_device().set_sampler_state(3, 7, 2);
}

}  // namespace rasterizer_shader_environment_technique_self_illumination_set_states_impl

}  // namespace halo::rasterizer
