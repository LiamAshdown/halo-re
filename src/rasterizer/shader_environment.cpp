/**
 * @file src/rasterizer/shader_environment.cpp
 * ShaderEnvironment draw passes.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "halo/rasterizer/tag_access.hpp"
#include "halo/render/shader_types.hpp"
#include "halo/rasterizer/constants.hpp"
#include "internal/shader_access.hpp"
#include "internal/state.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/shaders/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include <stdio.h>
#include <cstddef>
#include "halo/tags/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/render/api.hpp"

namespace {

constexpr uint16_t k_senv_alpha_tested = static_cast<uint16_t>(halo::tags::shader_environment_tag_flag::alpha_tested);
constexpr uint16_t k_senv_bump_map_is_specular_mask =
    static_cast<uint16_t>(halo::tags::shader_environment_tag_flag::bump_map_is_specular_mask);
constexpr uint16_t k_senv_true_atmospheric_fog = static_cast<uint16_t>(halo::tags::shader_environment_tag_flag::true_atmospheric_fog);
constexpr uint16_t k_senv_dynamic_mirror = static_cast<uint16_t>(halo::tags::shader_environment_reflection_tag_flag::dynamic_mirror);

}  // namespace

static_assert(offsetof(ShaderEnvironment, bump_map_scale_xy) == 0x138, "environment bump scale");
static_assert(offsetof(ShaderEnvironment, perpendicular_color) == 0x2a8, "environment perpendicular color");
static_assert(offsetof(ShaderEnvironment, parallel_color) == 0x2b4, "environment parallel color");
static_assert(offsetof(ShaderEnvironment, reflection_flags) == 0x2d0, "environment reflection flags");
static_assert(offsetof(ShaderEnvironment, reflection_type) == 0x2d2, "environment reflection type");

static_assert(sizeof(ShaderEnvironment) == 0x344);

static inline ShaderEnvironment *senv(const void *shader)
{
    return (ShaderEnvironment *)shader;
}

/** Copies the width and height pair the texture binders report for a bound bitmap. */
static void copy_bitmap_size(int16_t *destination, const int16_t *source)
{
    destination[0] = source[0];
    destination[1] = source[1];
}




namespace halo::rasterizer {

static uint8_t build_stage(int32_t *table, int32_t count, int effect_index, const char *format, uint8_t skip_middle)
{
    char name[128];
    int32_t i;

    for (i = 0; i < count; i++) {
        int32_t suffix = (skip_middle && i >= 6) ? i + 6 : i;
        void *technique;

        sprintf(name, format, rasterizer_shader_technique_name_suffixes[suffix]);
        technique = rasterizer_shader_technique_for_name(rasterizer_effects[effect_index].effect, name);
        table[i] = (int32_t)(uintptr_t)technique;
        if (technique == NULL) {
            return 0;
        }
    }
    return 1;
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_build_technique_table.
 *
 * @address 0x526930
 */
uint8_t rasterizer_shader_environment_build_technique_table(void)
{
    uint8_t ps_1_4 = rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_4;
    int32_t short_count = ps_1_4 ? 0xc : 6;
    int32_t long_count = ps_1_4 ? 0x18 : 0xc;
    uint8_t ok;

    ok = build_stage(environment_techniques_no, short_count, 116, "EnvironmentNo%s", 1);
    ok = ok && build_stage(environment_techniques_self_illumination, long_count, 117, "SelfIllumination%s", 0);
    ok = ok && build_stage(environment_techniques_change_color, long_count, 118, "ChangeColor%s", 0);
    ok = ok && build_stage(environment_techniques_multipurpose, long_count, 119, "Multipurpose%s", 0);
    ok = ok && build_stage(environment_techniques_reflection, long_count, 120, "Reflection%s", 0);
    ok = ok && build_stage(environment_techniques_plain, short_count, 121, "No%s", 1);
    if (rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_1 && rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4 && ok) {
        ok = build_stage(&environment_techniques_plain[6], short_count, 121, "No%sSelfIllumination", 0);
    }
    return ok;
}


/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_dispatch.
 *
 * @address 0x52b050
 */
void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, Shader *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    rasterizer_model_draw_context *context;
    Shader *overlay;

    halo::interface::debug_fp_dispatch_note(halo::rasterizer::fields::models_enabled, rasterizer_active_model_mode, shader->shader_type,
        primitive_count, reinterpret_cast<void *>(shader_environment_draw), reinterpret_cast<void *>(shader_environment_draw_simple),
        rasterizer_active_model_context ? rasterizer_active_model_context->group_parameters.shader : NULL);

    if (!halo::rasterizer::fields::models_enabled) {
        return;
    }
    context = rasterizer_active_model_context;
    overlay = context->group_parameters.shader;
    if (overlay != NULL) {
        int16_t source = shader_cast<ShaderTransparentPlasma>(overlay)->intensity_source;
        const float *function_values = context->group_parameters.function_values;
        uint8_t hidden = (overlay->shader_type == static_cast<int16_t>(halo::render::shader_type_id::transparent_plasma) &&
                          source >= 1 && source <= 4 &&
                          function_values != NULL && function_values[source - 1] == 0.0f);

        if (!hidden) {
            transparent_geometry_group *group =
                rasterizer_transparent_geometry_group_build(NULL, overlay, frame, index_buffer, dynamic_index_slot,
                                                            primitive_count, vertex_buffer, dynamic_vertex_slot,
                                                            &context->center);

            context = rasterizer_active_model_context;
            if (group != NULL) {
                group->lighting_extra = static_cast<render_animation *>(
                    chimera__rasterizer_memory_alloc(&context->group_parameters.change_colors, 8));
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
        if (shader->shader_type == static_cast<int16_t>(halo::render::shader_type_id::environment)) {
            shader_environment_draw_simple(
                shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
        } else {
            shader_environment_draw(
                shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
        }
    }
}


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_combine_stages(Shader *shader, int16_t frame, const float *matrix)
{
    chimera__rasterizer_set_texture(halo::tag_id_bits(senv(shader)->base_map.tag_id), 0, 0, 1, frame);
    render_device().set_transform(0x10, matrix);
    set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::diffuse);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_fixed_function.
 *
 * @address 0x527ae0
 */
void rasterizer_shader_environment_draw_fixed_function(Shader *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    float matrix[16];

    if (halo::rasterizer::fields::models_enabled == 0) {
        return;
    }
    if (rasterizer_active_model_context->flags & _model_draw_flag_8_bit) {
        set_render_state(halo::d3d9::rs::z_enable, 0);
    } else {
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_write_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        rasterizer_clear_decal_zbias();
    }
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, senv(shader)->shader_environment_flags & k_senv_alpha_tested);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled != 0);

    if (rasterizer_effects[116].effect == 0) {
        rasterizer_clear_decal_zbias();
        return;
    }
    memset(matrix, 0, sizeof matrix);
    matrix[0] = rasterizer_active_model_context->base_map_u_scale;
    matrix[5] = rasterizer_active_model_context->base_map_v_scale;
    matrix[15] = 1.0f;
    chimera__rasterizer_set_texture((senv(shader)->shader_environment_flags & k_senv_alpha_tested) ? halo::tag_id_bits(senv(shader)->bump_map.tag_id) : halo::k_dword_none, 1, 0, 1, frame);

    if (rasterizer_active_model_context->flags & _model_draw_fixed_function_fog_bit) {
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[14].declaration);
        set_combine_stages(shader, frame, matrix);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
        rasterizer_clear_decal_zbias();
        return;
    }
    {
        rasterizer_vertex_buffer processed = *vertex_buffer;
        void *handle = NULL;

        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[4].declaration);
        if (index_buffer != 0) {
            handle = rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer);
        }
        processed.hardware_buffer = handle;
        processed.type = 0xf;
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[15].declaration);
        set_combine_stages(shader, frame, matrix);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
    }
    rasterizer_clear_decal_zbias();
}

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

static void environment_set_vector(void *effect, void *handle, float x, float y, float z, float w)
{
    float vector[4];

    vector[0] = x;
    vector[1] = y;
    vector[2] = z;
    vector[3] = w;
    render_device().effect_set_vector(effect, handle, vector);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_pixel_shader.
 *
 * @address 0x528050
 */
void rasterizer_shader_environment_draw_pixel_shader(Shader *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    rasterizer_model_draw_context *context;
    float relative[3];
    uint16_t pixel_shader_fog = senv(shader)->shader_environment_flags & k_senv_true_atmospheric_fog;
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

    if (!halo::rasterizer::fields::models_enabled) {
        return;
    }
    context = rasterizer_active_model_context;
    relative[0] = context->center.x - rasterizer_camera_position[0];
    relative[1] = context->center.y - rasterizer_camera_position[1];
    relative[2] = context->center.z - rasterizer_camera_position[2];
    if (context->flags & _model_draw_flag_8_bit) {
        environment_set_render_state(halo::d3d9::rs::z_enable, 0);
    } else {
        environment_set_render_state(halo::d3d9::rs::z_enable, 1);
        environment_set_render_state(halo::d3d9::rs::z_write_enable, 1);
        environment_set_render_state(halo::d3d9::rs::z_func, 4);
        rasterizer_clear_decal_zbias();
    }
    environment_set_render_state(halo::d3d9::rs::cull_mode, 3);
    environment_set_render_state(halo::d3d9::rs::color_write_enable, 7);
    environment_set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    environment_set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    environment_set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    environment_set_render_state(halo::d3d9::rs::blend_op, 1);
    environment_set_render_state(halo::d3d9::rs::alpha_test_enable, senv(shader)->shader_environment_flags & k_senv_alpha_tested);
    environment_set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    if (rasterizer_device_version < halo::d3d9::k_pixel_shader_version_1_4) {
        environment_set_render_state(halo::d3d9::rs::fog_enable, 0);
    } else {
        environment_set_render_state(halo::d3d9::rs::fog_enable, (senv(shader)->shader_environment_flags >> 2) & 1);
    }

    if (pixel_shader_fog) {
        vertex_shader = 0x1c;
    } else if (halo::rasterizer::fields::planar_fog_vertex_shader_active) {
        vertex_shader = 0x19;
    } else if (context->lighting.point_light_count > 0) {
        vertex_shader = 0x1a;
    } else if (halo::tag_id_bits(senv(shader)->reflection_cube_map.tag_id) != k_datum_index_none) {
        vertex_shader = 0x1c;
    } else if (context->node_count <= 1) {
        vertex_shader = 0x1d;
    } else {
        vertex_shader = 0x1c;
    }

    effect = environment_effect_slot.effect;
    if (effect == 0) {
        rasterizer_clear_decal_zbias();
        return;
    }
    render_device().effect_set_technique(effect, (rasterizer_device_version >= halo::d3d9::k_pixel_shader_version_1_4 && !pixel_shader_fog) ?
            environment_techniques_ps14[senv(shader)->detail_map_function] :
            (uint32_t)environment_techniques_no[senv(shader)->detail_map_function]);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->base_map.tag_id), 0, 0, 1, frame, &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->primary_detail_map.tag_id), 0, 1, 2, frame, &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b((senv(shader)->shader_environment_flags & k_senv_alpha_tested) ? halo::tag_id_bits(senv(shader)->bump_map.tag_id) : halo::k_dword_none, 0, 2, 1, frame,
        &environment_effect_slot);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->reflection_cube_map.tag_id), 2, 3, 0, frame, &environment_effect_slot);

    a[0] = senv(shader)->perpendicular_brightness * context->lighting.reflection_tint.alpha;
    a[1] = senv(shader)->perpendicular_color.red * context->lighting.reflection_tint.red;
    a[2] = senv(shader)->perpendicular_color.green * context->lighting.reflection_tint.green;
    a[3] = senv(shader)->perpendicular_color.blue * context->lighting.reflection_tint.blue;
    b[0] = senv(shader)->parallel_brightness * context->lighting.reflection_tint.alpha;
    b[1] = senv(shader)->parallel_color.red * context->lighting.reflection_tint.red;
    b[2] = senv(shader)->parallel_color.green * context->lighting.reflection_tint.green;
    b[3] = senv(shader)->parallel_color.blue * context->lighting.reflection_tint.blue;
    c10_c12[0] = senv(shader)->primary_detail_map_scale;
    c10_c12[1] = senv(shader)->primary_detail_map_scale;
    c10_c12[2] = 1.0f;
    c10_c12[3] = 1.0f;
    c10_c12[4] = context->base_map_u_scale;
    c10_c12[5] = 0.0f;
    c10_c12[6] = 0.0f;
    c10_c12[7] = 0.0f;
    c10_c12[8] = 0.0f;
    c10_c12[9] = context->base_map_v_scale;
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
    if (!rasterizer_fog_enabled || (!pixel_shader_fog && (context->flags & _model_draw_flag_4_bit))) {
        fog[0] = 1.0f;
        fog[1] = fog[2] = fog[3] = 0.0f;
        negative[0] = negative[1] = negative[2] = 0.0f;
    } else if (pixel_shader_fog) {
        if (rasterizer_device_version < halo::d3d9::k_pixel_shader_version_1_4) {
            fog[0] = 1.0f;
            fog[1] = fog[2] = fog[3] = 0.0f;
            environment_set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_rgb_float_to_int(&rasterizer_fog_atmospheric_color.red));
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
            if (rasterizer_device_version < halo::d3d9::k_pixel_shader_version_1_4) {
                if (vertex_shader != 0x19) {
                    environment_set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_rgb_float_to_int(&rasterizer_fog_atmospheric_color.red));
                } else {
                    for (i = 0; i < 3; i++) {
                        add[i] = environment_clamp01(add[i] - halo::rasterizer::fields::planar_fog_attenuation * negative[i]);
                    }
                    environment_set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_pack_argb_from_real((ColorARGB *)fog));
                }
            }
        }
    }

    if (environment_effect_slot.constant_handles != 0) {
        void **handles = environment_effect_slot.constant_handles;

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


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_draw_single_stream.
 *
 * @address 0x5276c0
 */
void rasterizer_shader_environment_draw_single_stream(Shader *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    float matrix[16];

    if (halo::rasterizer::fields::models_enabled == 0) {
        return;
    }
    if (rasterizer_active_model_context->flags & _model_draw_flag_8_bit) {
        set_render_state(halo::d3d9::rs::z_enable, 0);
    } else {
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_write_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        rasterizer_clear_decal_zbias();
    }
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, senv(shader)->shader_environment_flags & k_senv_alpha_tested);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled != 0);

    memset(matrix, 0, sizeof matrix);
    matrix[0] = rasterizer_active_model_context->base_map_u_scale;
    matrix[5] = rasterizer_active_model_context->base_map_v_scale;
    matrix[10] = 0.0f;
    matrix[15] = 1.0f;

    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

    if (rasterizer_active_model_context->flags & _model_draw_fixed_function_fog_bit) {
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[14].declaration);
        chimera__rasterizer_set_texture(halo::tag_id_bits(senv(shader)->base_map.tag_id), 0, 0, 1, frame);
        render_device().set_transform(0x10, matrix);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
        rasterizer_clear_decal_zbias();
        return;
    }
    {
        rasterizer_vertex_buffer processed = *vertex_buffer;
        void *handle = NULL;

        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[4].declaration);
        if (index_buffer != 0) {
            handle = rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer);
        }
        processed.hardware_buffer = handle;
        processed.type = 0xf;
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[15].declaration);
        chimera__rasterizer_set_texture(halo::tag_id_bits(senv(shader)->base_map.tag_id), 0, 0, 1, frame);
        render_device().set_transform(0x10, matrix);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
            dynamic_vertex_slot);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
        rasterizer_clear_decal_zbias();
    }
}

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
 * Direct3D 9 back end function rasterizer_shader_environment_dynamic_mirror_draw.
 *
 * Registers: EAX -> shader
 *
 * @address 0x520e50
 */
void rasterizer_shader_environment_dynamic_mirror_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
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

    if (halo::rasterizer::fields::rasterizer_debug_mode_word != 0 || console_debug_toggle_6893f9 == 0 ||
        rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1) {
        return;
    }

    reflection_type = (uint16_t)shader->reflection_type;
    if (reflection_type == 0 || reflection_type == 2) {
        if ((shader->shader_environment_flags & k_senv_bump_map_is_specular_mask) != 0) {
            reflection_type = 1;
        }
        if (halo::tag_id_bits(shader->bump_map.tag_id) == halo::k_dword_none) {
            reflection_type = 1;
        }
    }
    if ((shader->reflection_flags & k_senv_dynamic_mirror) == 0) {
        return;
    }
    if (!(shader->perpendicular_brightness > 0.0f) && !(shader->parallel_brightness > 0.0f)) {
        return;
    }

    switch ((int16_t)reflection_type) {
    case 0:
    case 2:
        effect_index = 0x25;
        break;
    case 1:
        effect_index = (shader->shader_environment_flags & k_senv_bump_map_is_specular_mask) != 0 ? 0x27 : 0x26;
        break;
    default:
        effect_index = 0;
        break;
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = effect_slot->effect;
    if (effect == 0) {
        return;
    }

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    normalization_tag = halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id);
    bump_map_tag = halo::tag_id_bits(shader->bump_map.tag_id);
    if (bump_map_tag == halo::k_dword_none) {
        chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    } else {
        BitmapData *bump_bitmap = 0;

        if (halo::rasterizer::fields::bump_mapping_enabled != 0) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
            int32_t count = (int32_t)bitmap->bitmap_data.count;

            if (count > 0) {
                bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
                if (bump_bitmap->type != 0) {
                    bump_bitmap = 0;
                }
            }
        }
        if (bump_bitmap == 0) {
            uint32_t default_tag = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

            if (default_tag != halo::k_dword_none) {
                Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & halo::k_slot_mask].data;

                if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                    bump_bitmap = tag_block_element<BitmapData>(bitmap->bitmap_data, 3);
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

    render_device().effect_set_texture(effect, effect_slot->texture_handles[3], rasterizer_render_targets[2].texture);

    constants[0] = shader->bump_map_scale_xy.x;
    constants[1] = shader->bump_map_scale_xy.y;
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
    vectors[3] = (shader->shader_environment_flags & k_senv_bump_map_is_specular_mask) != 0 ? -1.0f : 0.0f;
    vectors[4] = shader->perpendicular_color.red;
    vectors[5] = shader->perpendicular_color.green;
    vectors[6] = shader->perpendicular_color.blue;
    vectors[7] = shader->perpendicular_brightness;
    vectors[8] = shader->parallel_color.red;
    vectors[9] = shader->parallel_color.green;
    vectors[10] = shader->parallel_color.blue;
    vectors[11] = shader->parallel_brightness;
    if (effect_slot->constant_handles != 0) {
        void **handles = effect_slot->constant_handles;

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
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_draw.
 *
 * @address 0x51e2a0
 */
void rasterizer_shader_environment_lightmap_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    rasterizer_effect_slot *slot;
    int16_t index;
    int16_t size[4][2];
    float su1 = 1.0f, sv1 = 1.0f, su2 = 1.0f, sv2 = 1.0f, su3 = 1.0f, sv3 = 1.0f;
    float constants[12];
    void *effect;
    uint32_t passes;
    uint32_t pass;

    if (!halo::rasterizer::fields::rasterizer_environment_diffuse_textures) {
        return;
    }
    index = (int16_t)(static_cast<uint16_t>(senv(shader)->shader_environment_type) * 3 + static_cast<uint16_t>(senv(shader)->detail_map_function));
    index = (int16_t)((uint16_t)(index * 3) + static_cast<uint16_t>(senv(shader)->micro_detail_map_function) + 5);
    slot = &rasterizer_effects[index];
    if (slot->effect == 0) {
        return;
    }
    copy_bitmap_size(size[0], rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->base_map.tag_id), 0, 0, 1, frame, slot));
    copy_bitmap_size(size[1], rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->primary_detail_map.tag_id), 0, 1, 2, frame, slot));
    copy_bitmap_size(size[2], rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->secondary_detail_map.tag_id), 0, 2, 2, frame, slot));
    copy_bitmap_size(size[3], rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(senv(shader)->micro_detail_map.tag_id), 0, 3, 2, frame, slot));
    if (halo::test_flag(senv(shader)->diffuse_flags, halo::tags::shader_environment_diffuse_tag_flag::rescale_detail_maps)) {
        float base_width = (float)size[0][0];
        float base_height = (float)size[0][1];

        su1 = base_width / (float)size[1][0];
        sv1 = base_height / (float)size[1][1];
        su2 = base_width / (float)size[2][0];
        sv2 = base_height / (float)size[2][1];
        su3 = base_width / (float)size[3][0];
        sv3 = base_height / (float)size[3][1];
    }
    constants[0] = su1 * senv(shader)->primary_detail_map_scale;
    constants[1] = sv1 * senv(shader)->primary_detail_map_scale;
    constants[2] = su2 * senv(shader)->secondary_detail_map_scale;
    constants[3] = sv2 * senv(shader)->secondary_detail_map_scale;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = su3 * senv(shader)->micro_detail_map_scale;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = sv3 * senv(shader)->micro_detail_map_scale;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>((const ShaderEnvironment *)shader));

    render_device().set_vertex_shader_constant_f(10, constants, 3);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[slot->vertex_shader_index].shader);

    effect = slot->effect;
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


/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_draw_single_stream.
 *
 * @address 0x51e8f0
 */
void rasterizer_shader_environment_lightmap_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    d3d_call3_fn set_texture_stage_state;

    if (halo::rasterizer::fields::rasterizer_environment_diffuse_textures == 0) {
        return;
    }
    chimera__rasterizer_set_texture(halo::tag_id_bits(((struct ShaderEnvironment *)shader)->base_map.tag_id), 0, 0, 1, frame);
    render_device().set_vertex_shader(0);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[19].declaration);
    render_device().set_pixel_shader(0);
    set_texture_stage_state = halo::d3d9::device_function<d3d_call3_fn>(rasterizer_device, halo::d3d9::device_method::set_texture_stage_state);
    set_texture_stage_state(rasterizer_device, 0, 1, 2);
    set_texture_stage_state(rasterizer_device, 0, 2, 2);
    set_texture_stage_state(rasterizer_device, 0, 4, 2);
    set_texture_stage_state(rasterizer_device, 0, 5, 1);
    set_texture_stage_state(rasterizer_device, halo::d3d9::ts::color_op, 1, 1);
    set_texture_stage_state(rasterizer_device, halo::d3d9::ts::color_op, 4, 1);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
}

}  // namespace rasterizer_shader_environment_lightmap_draw_single_stream_impl

namespace rasterizer_shader_environment_lightmap_draw_two_stream_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_draw_two_stream.
 *
 * @address 0x51e570
 */
void rasterizer_shader_environment_lightmap_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
    int16_t effect_index;
    int16_t *base_size;
    int16_t base_width, base_height;

    if (halo::rasterizer::fields::rasterizer_environment_diffuse_textures == 0) {
        return;
    }
    effect_index = (int16_t)((uint16_t)(((uint16_t)(env->shader_environment_type * 3) + env->detail_map_function) * 3) +
        env->micro_detail_map_function + 5);
    if (rasterizer_effects[effect_index].effect == 0) {
        return;
    }

    base_size = chimera__rasterizer_set_texture(halo::tag_id_bits(env->base_map.tag_id), 0, 0, 1, frame);
    base_width = base_size[0];
    base_height = base_size[1];
    render_device().set_vertex_shader(0);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[12].declaration);
    render_device().set_pixel_shader(0);

    if (halo::tag_id_bits<int32_t>(env->primary_detail_map.tag_id) != -1) {
        int16_t *detail_size = chimera__rasterizer_set_texture(halo::tag_id_bits(env->primary_detail_map.tag_id), 1, 0, 2, frame);
        float matrix[16];

        memset(matrix, 0, sizeof matrix);
        matrix[0] = (float)(int32_t)base_width / (float)(int32_t)detail_size[0] * env->primary_detail_map_scale;
        matrix[5] = (float)(int32_t)base_height / (float)(int32_t)detail_size[1] * env->primary_detail_map_scale;
        matrix[10] = 1.0f;
        matrix[15] = 1.0f;
        set_texture_stage_state(1, halo::d3d9::ts::texture_transform_flags, 2);
        render_device().set_transform(0x11, matrix);
        set_texture_stage_state(1, halo::d3d9::ts::texcoord_index, 0);
        set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
        set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
        set_texture_stage_state(1, halo::d3d9::ts::texture_transform_flags, 0);
        set_texture_stage_state(1, halo::d3d9::ts::texcoord_index, 1);
        return;
    }
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
}

}  // namespace rasterizer_shader_environment_lightmap_draw_two_stream_impl

namespace rasterizer_shader_environment_lightmap_specular_draw_impl {


/**
 * Direct3D 9 back end function rasterizer_shader_environment_lightmap_specular_draw.
 *
 * Registers: EAX -> shader
 *
 * @address 0x521f90
 */
void rasterizer_shader_environment_lightmap_specular_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
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

    if (halo::rasterizer::fields::rasterizer_debug_mode_word != 0 || halo::rasterizer::fields::specular_lightmap_enabled == 0 || render_force_flag != 0 ||
        rasterizer_lightmap_bitmap_missing != 0 || rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4) {
        return;
    }
    if (!(env->brightness > 0.0f)) {
        return;
    }
    specular_flags = env->specular_flags;
    if (!halo::test_flag(specular_flags, halo::tags::shader_environment_specular_tag_flag::lightmap_is_specular)) {
        return;
    }
    effect_slot = halo::test_flag(env->shader_environment_flags, halo::tags::shader_environment_tag_flag::bump_map_is_specular_mask) ? &rasterizer_effects[42] : &rasterizer_effects[43];
    effect = effect_slot->effect;
    if (effect == 0) {
        return;
    }
    specular_exponent = halo::test_flag(specular_flags, halo::tags::shader_environment_specular_tag_flag::overbright) ? 4.0f : 2.0f;

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    bump_map_tag = halo::tag_id_bits(env->bump_map.tag_id);
    bump_bitmap = 0;
    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && bump_map_tag != halo::k_dword_none) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (bump_bitmap->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (default_tag != halo::k_dword_none) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & halo::k_slot_mask].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = tag_block_element<BitmapData>(bitmap->bitmap_data, 3);
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
    normalization_tag = halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 3, 0, effect_slot);

    constants[0] = env->bump_map_scale_xy.x;
    constants[1] = env->bump_map_scale_xy.y;
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

    pixel_constants[0] = env->brightness;
    pixel_constants[1] = pixel_constants[0];
    pixel_constants[2] = pixel_constants[0];
    pixel_constants[3] = pixel_constants[0];
    pixel_constants[4] = env->perpendicular_color.red;
    pixel_constants[5] = env->perpendicular_color.green;
    pixel_constants[6] = env->perpendicular_color.blue;
    pixel_constants[7] = 1.0f;
    pixel_constants[8] = env->parallel_color.red;
    pixel_constants[9] = env->parallel_color.green;
    pixel_constants[10] = env->parallel_color.blue;
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

    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && bump_map_tag != halo::k_dword_none) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (bump_bitmap->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (default_tag == halo::k_dword_none) {
            return;
        }
        {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & halo::k_slot_mask].data;

            if (bitmap == 0 || (int32_t)bitmap->bitmap_data.count <= 3) {
                return;
            }
            bump_bitmap = tag_block_element<BitmapData>(bitmap->bitmap_data, 3);
        }
    }
    rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
    rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
    rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_projected_light_draw.
 *
 * @address 0x521900
 */
void rasterizer_shader_environment_projected_light_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float specular_exponent;
    uint32_t normalization_tag;
    float constants[12];
    float vector[4];
    uint32_t pass_count;
    uint32_t pass;

    if (halo::rasterizer::fields::rasterizer_debug_mode_word != 0 || halo::rasterizer::fields::specular_projected_light_enabled == 0 ||
        rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4) {
        return;
    }
    if (!(env->brightness > 0.0f) || !(rasterizer_projected_light_luminance > 0.0f)) {
        return;
    }
    effect_slot = halo::test_flag(env->shader_environment_flags, halo::tags::shader_environment_tag_flag::bump_map_is_specular_mask) ? &rasterizer_effects[40] : &rasterizer_effects[41];
    effect = effect_slot->effect;
    if (effect == 0) {
        return;
    }
    specular_exponent = halo::test_flag(env->specular_flags, halo::tags::shader_environment_specular_tag_flag::overbright) ? 4.0f : 2.0f;

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[effect_slot->vertex_shader_index + rasterizer_projected_light_shader_variant].shader);
    render_device().set_vertex_shader_constant_f(0xd, (const float *)&rasterizer_projected_light, 5);

    rasterizer_bind_bump_map(halo::tag_id_bits(env->bump_map.tag_id), frame, effect_slot);
    if (rasterizer_projected_light_has_cube_map == 1) {
        rasterizer_resolve_and_cache_submap_b(rasterizer_projected_light_cube_map, 2, 1, 1, 0, effect_slot);
    } else {
        chimera__rasterizer_set_texture_direct_d3dx(rasterizer_projected_light_cube_map, 1, 0, effect_slot);
    }
    normalization_tag = halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 2, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, 3, 0, effect_slot);

    constants[0] = env->bump_map_scale_xy.x;
    constants[1] = env->bump_map_scale_xy.y;
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
        void **handles = effect_slot->constant_handles;
        float light = rasterizer_projected_light_luminance * env->brightness;

        vector[0] = light;
        vector[1] = light;
        vector[2] = light;
        vector[3] = light;
        render_device().effect_set_vector(effect, handles[0], vector);
        vector[0] = env->perpendicular_color.red;
        vector[1] = env->perpendicular_color.green;
        vector[2] = env->perpendicular_color.blue;
        vector[3] = 1.0f;
        render_device().effect_set_vector(effect, handles[1], vector);
        vector[0] = env->parallel_color.red;
        vector[1] = env->parallel_color.green;
        vector[2] = env->parallel_color.blue;
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
 * Direct3D 9 back end function rasterizer_shader_environment_reflection_draw.
 *
 * @address 0x5202f0
 */
void rasterizer_shader_environment_reflection_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
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

    if (halo::rasterizer::fields::rasterizer_debug_mode_word != 0 || halo::rasterizer::fields::specular_enabled == 0 ||
        rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        return;
    }

    reflection_type = env->reflection_type;
    if (reflection_type == 0 || reflection_type == 2) {
        if (halo::test_flag(env->shader_environment_flags, halo::tags::shader_environment_tag_flag::bump_map_is_specular_mask)) {
            reflection_type = 1;
        }
        if (halo::tag_id_bits(env->bump_map.tag_id) == halo::k_dword_none) {
            reflection_type = 1;
        }
    }
    if (!(env->perpendicular_brightness > 0.0f) && !(env->parallel_brightness > 0.0f)) {
        return;
    }
    if (halo::tag_id_bits(env->reflection_cube_map.tag_id) == halo::k_dword_none) {
        return;
    }

    switch (reflection_type) {
    case 0:
    case 2:
        effect_index = 0x20;
        break;
    case 1:
        effect_index = halo::test_flag(env->shader_environment_flags, halo::tags::shader_environment_tag_flag::bump_map_is_specular_mask) ? 0x22 : 0x21;
        break;
    default:
        effect_index = 0;
        break;
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = effect_slot->effect;
    if (effect == 0) {
        return;
    }

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[0].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    bump_map_tag = halo::tag_id_bits(env->bump_map.tag_id);
    bump_bitmap = 0;
    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && bump_map_tag != halo::k_dword_none) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (bump_bitmap->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (default_tag != halo::k_dword_none) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & halo::k_slot_mask].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = tag_block_element<BitmapData>(bitmap->bitmap_data, 3);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }
    chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id), 1, 0, effect_slot);
    chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id), 2, 0, effect_slot);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(env->reflection_cube_map.tag_id), 2, 3, 0, frame, effect_slot);

    constants[0] = env->bump_map_scale_xy.x;
    constants[1] = env->bump_map_scale_xy.y;
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
        void **handles = effect_slot->constant_handles;

        vector[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
        vector[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
        vector[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
        vector[3] = 0.0f;
        render_device().effect_set_vector(effect, handles[0], vector);

        vector[0] = env->perpendicular_color.red;
        vector[1] = env->perpendicular_color.green;
        vector[2] = env->perpendicular_color.blue;
        vector[3] = env->perpendicular_brightness;
        render_device().effect_set_vector(effect, handles[1], vector);

        vector[0] = env->parallel_color.red;
        vector[1] = env->parallel_color.green;
        vector[2] = env->parallel_color.blue;
        vector[3] = env->parallel_brightness;
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
 * Direct3D 9 back end function rasterizer_shader_environment_select_draw_functions.
 *
 * @address 0x52b630
 */
void rasterizer_shader_environment_select_draw_functions(void)
{
    if ((int32_t)rasterizer_caps.max_streams <= 1) {
        shader_environment_draw_simple = rasterizer_shader_environment_draw_single_stream;
        shader_environment_draw = rasterizer_shader_model_draw_limited;
        return;
    }
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        shader_environment_draw_simple = rasterizer_shader_environment_draw_fixed_function;
        shader_environment_draw = rasterizer_shader_model_draw_fixed_function;
        return;
    }
    shader_environment_draw_simple = rasterizer_shader_environment_draw_pixel_shader;
    shader_environment_draw = rasterizer_shader_model_draw_pixel_shader;
}

namespace rasterizer_shader_environment_self_illumination_draw_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


static void rasterizer_set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static float self_illumination_animation(WaveFunction_t function, float period, float phase)
{
    return halo::math::periodic_function_evaluate((periodic_function_t)function, (phase + rasterizer_time.time) / period);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_self_illumination_draw.
 *
 * @address 0x51f3e0
 */
void rasterizer_shader_environment_self_illumination_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
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

    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, halo::test_flag(shader->shader_environment_flags, halo::tags::shader_environment_tag_flag::alpha_tested) && halo::rasterizer::fields::environment_alpha_testing_enabled != 0 ? 1 : 0);

    map_tag = halo::tag_id_bits(env->map.tag_id);
    if (map_tag == halo::k_dword_none) {
        effect_index = (int16_t)(2 + (halo::rasterizer::fields::environment_effect_variant != 0 ? 1 : 0));
    } else {
        effect_index = (int16_t)(halo::rasterizer::fields::environment_effect_variant != 0 ? 1 : 0);
    }
    effect_slot = &rasterizer_effects[effect_index];
    effect = effect_slot->effect;
    if (effect == 0) {
        return;
    }

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[13].shader);

    bump_map_tag = (env->shader_environment_flags & k_senv_bump_map_is_specular_mask) != 0 ? halo::k_dword_none : halo::tag_id_bits(env->bump_map.tag_id);
    bump_bitmap = 0;
    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && bump_map_tag != halo::k_dword_none) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (bump_bitmap->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (default_tag != halo::k_dword_none) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & halo::k_slot_mask].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = tag_block_element<BitmapData>(bitmap->bitmap_data, 3);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }

    rasterizer_resolve_and_cache_submap_b(map_tag, 0, 1, 0, frame, effect_slot);
    if (halo::test_flag(env->self_illumination_flags, halo::tags::is_unfiltered_flag_tag_flag::unfiltered)) {
        rasterizer_set_sampler_state(1, halo::d3d9::ss::mag_filter, 1);
        rasterizer_set_sampler_state(1, halo::d3d9::ss::min_filter, 1);
        rasterizer_set_sampler_state(1, halo::d3d9::ss::mip_filter, 1);
    } else {
        rasterizer_set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
        rasterizer_set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
        rasterizer_set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);
    }

    if (rasterizer_environment_lightmap != 0) {
        rasterizer_bind_texture_d3dx(2, rasterizer_environment_lightmap, effect_slot);
    } else {
        render_device().effect_set_texture(effect, effect_slot->texture_handles[2], rasterizer_capture_surfaces[0]);
    }
    chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id), 3, 0,
                                                effect_slot);

    constants[0] = env->bump_map_scale_xy.x;
    constants[1] = env->bump_map_scale_xy.y;
    constants[2] = env->map_scale;
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
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[13].shader);

    primary = self_illumination_animation(env->primary_animation_function, env->primary_animation_period, env->primary_animation_phase);
    secondary = self_illumination_animation(env->secondary_animation_function, env->secondary_animation_period, env->secondary_animation_phase);
    plasma = self_illumination_animation(env->plasma_animation_function, env->plasma_animation_period, env->plasma_animation_phase);
    a = 1.0f - primary;
    b = 1.0f - secondary;
    primary_color[0] = primary * env->primary_on_color.red + a * env->primary_off_color.red;
    primary_color[1] = primary * env->primary_on_color.green + a * env->primary_off_color.green;
    primary_color[2] = primary * env->primary_on_color.blue + a * env->primary_off_color.blue;
    secondary_color[0] = secondary * env->secondary_on_color.red + b * env->secondary_off_color.red;
    secondary_color[1] = secondary * env->secondary_on_color.green + b * env->secondary_off_color.green;
    secondary_color[2] = secondary * env->secondary_on_color.blue + b * env->secondary_off_color.blue;

    material[0] = env->material_color.red;
    material[1] = env->material_color.green;
    material[2] = env->material_color.blue;
    material[3] = 1.0f;
    if (effect_slot->constant_handles != 0) {
        render_device().effect_set_vector(effect, effect_slot->constant_handles[0], material);
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
        vectors[12] = env->plasma_on_color.red;
        vectors[13] = env->plasma_on_color.green;
        vectors[14] = env->plasma_on_color.blue;
        vectors[15] = 1.0f;
        vectors[16] = env->plasma_off_color.red;
        vectors[17] = env->plasma_off_color.green;
        vectors[18] = env->plasma_off_color.blue;
        vectors[19] = 1.0f;
        if (effect_slot->constant_handles != 0) {
            void **handles = effect_slot->constant_handles;
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
                                                                    vertex_buffer + (halo::rasterizer::fields::environment_effect_variant == 0 ? 1 : 0));
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_environment_self_illumination_draw_impl

namespace rasterizer_shader_environment_self_illumination_draw_single_stream_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_self_illumination_draw_single_stream.
 *
 * @address 0x51fd80
 */
void rasterizer_shader_environment_self_illumination_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
    datum_index self_illumination;
    BitmapData *bitmap = 0;
    uint32_t colour;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, halo::test_flag(shader->shader_environment_flags, halo::tags::shader_environment_tag_flag::alpha_tested) && halo::rasterizer::fields::environment_alpha_testing_enabled != 0);
    render_device().set_vertex_shader(0);

    colour = halo::rasterizer::pack_opaque_color(env->material_color.red, shader->material_color.green, shader->material_color.blue);
    render_device().set_render_state(halo::d3d9::rs::texture_factor, colour);

    self_illumination = (env->shader_environment_flags & k_senv_bump_map_is_specular_mask) ? k_datum_index_none : halo::tag_id_bits(env->bump_map.tag_id);
    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && self_illumination != k_datum_index_none) {
        int32_t count = (int32_t)reinterpret_cast<Bitmap *>(halo::cache::globals().tag_instances[self_illumination & halo::k_slot_mask].data)->bitmap_data.count;

        if (count > 0) {
            bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(self_illumination, (int16_t)((int32_t)frame % count));
            if (bitmap->type != 0) {
                bitmap = 0;
            }
        }
    }
    if (bitmap == 0) {
        datum_index fallback = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (fallback != k_datum_index_none) {
            Bitmap *tag = reinterpret_cast<Bitmap *>(halo::cache::globals().tag_instances[fallback & halo::k_slot_mask].data);

            if (tag != 0 && (int32_t)tag->bitmap_data.count > 3) {
                bitmap = tag_block_element<BitmapData>(tag->bitmap_data, 3);
            }
        }
    }
    if (bitmap != 0) {
        rasterizer_bind_texture_d3d9(0, bitmap);
        rasterizer_bound_bitmap_size_a[0] = static_cast<int16_t>(bitmap->width);
        rasterizer_bound_bitmap_size_a[1] = static_cast<int16_t>(bitmap->height);
    }

    {
        void *texture;

        if (rasterizer_environment_lightmap != 0) {
            halo::cache::texture_cache_get(rasterizer_environment_lightmap, 1, 1);
            texture = bitmap_hardware_texture(*rasterizer_environment_lightmap);
        } else {
            texture = rasterizer_capture_surfaces[0];
        }
        render_device().set_texture(1, texture);
    }
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::add);
    set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
    set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[19].declaration);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
}

}  // namespace rasterizer_shader_environment_self_illumination_draw_single_stream_impl

namespace rasterizer_shader_environment_self_illumination_draw_two_stream_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_environment_self_illumination_draw_two_stream.
 *
 * @address 0x51fad0
 */
void rasterizer_shader_environment_self_illumination_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const ShaderEnvironment *env = shader;
    datum_index self_illumination;
    BitmapData *bitmap = 0;
    uint32_t colour;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, halo::test_flag(shader->shader_environment_flags, halo::tags::shader_environment_tag_flag::alpha_tested) && halo::rasterizer::fields::environment_alpha_testing_enabled != 0);
    render_device().set_vertex_shader(0);

    colour = halo::rasterizer::pack_opaque_color(env->material_color.red, shader->material_color.green, shader->material_color.blue);
    render_device().set_render_state(halo::d3d9::rs::texture_factor, colour);

    self_illumination = (env->shader_environment_flags & k_senv_bump_map_is_specular_mask) ? k_datum_index_none : halo::tag_id_bits(env->bump_map.tag_id);
    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && self_illumination != k_datum_index_none) {
        int32_t count = (int32_t)reinterpret_cast<Bitmap *>(halo::cache::globals().tag_instances[self_illumination & halo::k_slot_mask].data)->bitmap_data.count;

        if (count > 0) {
            bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(self_illumination, (int16_t)((int32_t)frame % count));
            if (bitmap->type != 0) {
                bitmap = 0;
            }
        }
    }
    if (bitmap == 0) {
        datum_index fallback = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (fallback != k_datum_index_none) {
            Bitmap *tag = reinterpret_cast<Bitmap *>(halo::cache::globals().tag_instances[fallback & halo::k_slot_mask].data);

            if (tag != 0 && (int32_t)tag->bitmap_data.count > 3) {
                bitmap = tag_block_element<BitmapData>(tag->bitmap_data, 3);
            }
        }
    }
    if (bitmap != 0) {
        rasterizer_bind_texture_d3d9(0, bitmap);
        rasterizer_bound_bitmap_size_a[0] = static_cast<int16_t>(bitmap->width);
        rasterizer_bound_bitmap_size_a[1] = static_cast<int16_t>(bitmap->height);
    }

    {
        void *texture;

        if (rasterizer_environment_lightmap != 0) {
            halo::cache::texture_cache_get(rasterizer_environment_lightmap, 1, 1);
            texture = bitmap_hardware_texture(*rasterizer_environment_lightmap);
        } else {
            texture = rasterizer_capture_surfaces[0];
        }
        render_device().set_texture(1, texture);
    }
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::add);
    set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
    set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[13].declaration);
    chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot,
        first_primitive, vertex_buffer + (halo::rasterizer::fields::environment_effect_variant == 0 ? 1 : 0));
}

}  // namespace rasterizer_shader_environment_self_illumination_draw_two_stream_impl

/**
 * Direct3D 9 back end function rasterizer_shader_environment_set_lightmap.
 *
 * Registers: EAX = lightmap
 *
 * @address 0x520910
 */
void rasterizer_shader_environment_set_lightmap(BitmapData *lightmap)
{
    if (halo::rasterizer::fields::rasterizer_debug_mode == 0 && halo::rasterizer::fields::environment_multipurpose_enabled != 0 &&
        halo::rasterizer::fields::specular_enabled != 0 && render_force_flag == 0 &&
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
 * Direct3D 9 back end function rasterizer_shader_environment_technique_draw.
 *
 * Registers: EAX -> vertex_buffer, ECX -> shader
 *
 * @address 0x520970
 */
void rasterizer_shader_environment_technique_draw(rasterizer_vertex_buffer *vertex_buffer, const ShaderEnvironment *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    const ShaderEnvironment *env = shader;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float constants[12];
    float scale[4];
    uint32_t pass_count;
    uint32_t pass;

    if (halo::rasterizer::fields::rasterizer_debug_mode_word != 0 || halo::rasterizer::fields::environment_multipurpose_enabled == 0 || halo::rasterizer::fields::specular_enabled == 0 ||
        render_force_flag != 0 || rasterizer_environment_lightmap_missing != 0) {
        return;
    }
    if (!(env->perpendicular_brightness > 0.0f) && !(env->parallel_brightness > 0.0f)) {
        return;
    }
    if (!(env->lightmap_brightness_scale < 1.0f)) {
        return;
    }

    constants[0] = env->bump_map_scale_xy.x;
    constants[1] = env->bump_map_scale_xy.y;
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
    effect = effect_slot->effect;
    scale[0] = env->lightmap_brightness_scale;
    scale[1] = scale[0];
    scale[2] = scale[0];
    scale[3] = scale[0];
    render_device().set_pixel_shader_constant_f(1, scale, 1);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[2].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

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
 * Direct3D 9 back end function rasterizer_shader_environment_technique_multipurpose_set_states.
 *
 * @address 0x520790
 */
void rasterizer_shader_environment_technique_multipurpose_set_states(void)
{

    if (halo::rasterizer::fields::rasterizer_debug_mode == 0 && halo::rasterizer::fields::environment_multipurpose_enabled != 0 &&
        halo::rasterizer::fields::specular_enabled != 0 && render_force_flag == 0) {
        render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
        render_device().set_render_state(halo::d3d9::rs::color_write_enable, 8);
        render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::dest_alpha);
        render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::zero);
        render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::z_func, 3);
        render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);
        render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);

        render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    }
    rasterizer_active_environment_effect = &rasterizer_effects[36];
}

}  // namespace rasterizer_shader_environment_technique_multipurpose_set_states_impl

namespace rasterizer_shader_environment_technique_ps2_set_states_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_shader_environment_technique_ps2_set_states.
 *
 * @address 0x5212d0
 */
void rasterizer_shader_environment_technique_ps2_set_states(void)
{

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || halo::rasterizer::fields::specular_projected_light_enabled == 0 ||
        rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_3) {
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
    render_device().set_render_state(halo::d3d9::rs::color_write_enable, 7);
    render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::dest_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0);
    render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::z_func, 3);
    render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);

    render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);
}

}  // namespace rasterizer_shader_environment_technique_ps2_set_states_impl

namespace rasterizer_shader_environment_technique_self_illumination_set_states_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_shader_environment_technique_self_illumination_set_states.
 *
 * @address 0x520b90
 */
void rasterizer_shader_environment_technique_self_illumination_set_states(void)
{

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || console_debug_toggle_6893f9 == 0 ||
        rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1) {
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
    render_device().set_render_state(halo::d3d9::rs::color_write_enable, 7);
    render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::dest_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::z_func, 3);
    render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);

    render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 1);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 1);
    render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 1);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);
}

}  // namespace rasterizer_shader_environment_technique_self_illumination_set_states_impl

}  // namespace halo::rasterizer
