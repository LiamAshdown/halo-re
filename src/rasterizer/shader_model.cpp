/**
 * @file src/rasterizer/shader_model.cpp
 * ShaderModel draw passes, model draw state and object shadows.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/shaders/api.hpp"
#include "halo/math/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/libm.hpp"

static_assert(sizeof(ShaderModel) == 0x1b8);

static inline ShaderModel *smodel(const void *shader)
{
    return (ShaderModel *)shader;
}




namespace halo::rasterizer {


/**
 * Builds the weighted skinning matrix palette for up to rasterizer_maximum_skinning_nodes of a model's blended
 * nodes, and optionally uploads it as vertex shader constants (register 0x1d).
 *
 * Registers: unaff_EDI -> rasterizer_node_matrices, stack -> upload
 *
 * @address 0x518b40
 */
void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes)
{
    int16_t count = (nodes->node_count < rasterizer_maximum_skinning_nodes)
                        ? nodes->node_count : rasterizer_maximum_skinning_nodes;
    int16_t i;

    for (i = 0; i < count; i++) {
        real_matrix4x3 *m = &((real_matrix4x3 *)nodes->matrices)[i];
        rasterizer_skinning_matrix *out = &rasterizer_skinning_palette[i];
        float scale = m->scale;

        out->rows[0][0] = scale * m->forward.i;
        out->rows[0][1] = scale * m->left.i;
        out->rows[0][2] = scale * m->up.i;
        out->rows[0][3] = m->position.x;

        out->rows[1][0] = scale * m->forward.j;
        out->rows[1][1] = scale * m->left.j;
        out->rows[1][2] = scale * m->up.j;
        out->rows[1][3] = m->position.y;

        out->rows[2][0] = scale * m->forward.k;
        out->rows[2][1] = scale * m->left.k;
        out->rows[2][2] = scale * m->up.k;
        out->rows[2][3] = m->position.z;
    }

    if (upload != 0) {
        render_device().set_vertex_shader_constant_f(0x1d, rasterizer_skinning_palette, (uint32_t)(count * 3));
    }
}

/**
 * Uploads a model's per-node-part skeleton transforms (gathered from the skinning palette by index byte) as
 * vertex-shader constants for hardware skinning, clamped to k_rasterizer_maximum_node_parts entries.
 *
 * Registers: EAX -> node_part_count, ESI -> node_part_indices
 *
 * @address 0x526cf0
 */
void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices)
{
    int32_t count;
    rasterizer_skinning_matrix parts[k_rasterizer_maximum_node_parts];
    int32_t i;

    count = (node_part_count > 0x15) ? k_rasterizer_maximum_node_parts : node_part_count;

    for (i = 0; i < count; i++) {
        parts[i] = rasterizer_skinning_palette[node_part_indices[i]];
    }

    rasterizer_node_part_indices = node_part_indices;
    rasterizer_node_part_count = node_part_count;

    render_device().set_vertex_shader_constant_f(0x1d, parts, (uint32_t)(count * 3));
}

namespace rasterizer_model_draw_prepare_states_impl {


static inline void clamp01_x87(float &x)
{
    if (x < 0.0f) {
        x = 0.0f;
    } else if (x > 1.0f) {
        x = 1.0f;
    }
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

/**
 * Configures the fixed-function texture-coordinate filtering and, once per model, skinning/ lighting/planar-
 * fog/frustum-z render states used before drawing model geometry.
 *
 * Registers: ESI -> context, stack -> mode
 *
 * @address 0x526f50
 */
void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t mode)
{
    set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    set_sampler_state(1, halo::d3d9::ss::address_u, 1);
    set_sampler_state(1, halo::d3d9::ss::address_v, 1);
    set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);

    if (halo::d3d9::k_pixel_shader_version_1_0 < rasterizer_caps.pixel_shader_version) {
        set_sampler_state(2, halo::d3d9::ss::address_u, 1);
        set_sampler_state(2, halo::d3d9::ss::address_v, 1);
        set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
        set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
        set_sampler_state(2, halo::d3d9::ss::mip_filter, 2);
        set_sampler_state(3, halo::d3d9::ss::address_u, 3);
        set_sampler_state(3, halo::d3d9::ss::address_v, 3);
        set_sampler_state(3, halo::d3d9::ss::address_w, 3);
        set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
        set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
        set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);
    }

    if (halo::rasterizer::fields::models_enabled != 0) {
        if ((int8_t)context->flags < 0 && mode == 0) {
            rasterizer_set_shader_stage_config(1);
            chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
        }

        rasterizer_model_scratch_valid = 0;
        halo::rasterizer::fields::model_draw_mode = mode;
        rasterizer_active_model_context = context;

        if (halo::rasterizer::fields::active_camouflage_enabled == 0 || rasterizer_window.type != 1 ||
            context->group_parameters.mode != 1 || !(context->group_parameters.blend_factor > 0.0f)) {
            if (context->group_parameters.mode == 2) {
                rasterizer_active_model_mode = 2;
            } else {
                chimera__rasterizer_set_model_skinning((uint8_t)(~(context->flags >> 8) & 1),
                                                        (rasterizer_node_matrices *)&context->node_matrices);
                rasterizer_prepare_lighting_constants(&context->lighting);
                rasterizer_active_model_mode = 0;
            }
        } else {
            rasterizer_active_model_mode = 1;
        }

        if (rasterizer_window.fog.planar_mode == 0 || (context->flags & 4) != 0 ||
            ((context->flags & 0x40) != 0 &&
             !((rasterizer_window.camera.position.x * rasterizer_window.fog.plane.normal.i +
                rasterizer_window.camera.position.y * rasterizer_window.fog.plane.normal.j +
                rasterizer_window.camera.position.z * rasterizer_window.fog.plane.normal.k) -
                   rasterizer_window.fog.plane.d < 0.0f))) {
            halo::rasterizer::fields::planar_fog_vertex_shader_active = 0;
        } else {
            halo::rasterizer::fields::planar_fog_vertex_shader_active = 1;
        }
        halo::rasterizer::fields::model_begin_cleared_flag = 0;

        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
            if ((context->flags & 0x200) != 0) {
                float world_matrix[16];
                float *m = (float *)context->node_matrices;

                world_matrix[0] = m[1];
                world_matrix[1] = m[2];
                world_matrix[2] = m[3];
                world_matrix[3] = 0.0f;
                world_matrix[4] = m[4];
                world_matrix[5] = m[5];
                world_matrix[6] = m[6];
                world_matrix[7] = 0.0f;
                world_matrix[8] = m[7];
                world_matrix[9] = m[8];
                world_matrix[10] = m[9];
                world_matrix[11] = 0.0f;
                world_matrix[12] = m[10] * m[0];
                world_matrix[13] = m[11] * m[0];
                world_matrix[14] = m[12] * m[0];
                world_matrix[15] = 1.0f;

                {
                    render_device().set_transform(0x100, world_matrix);
                }
                return;
            }
        } else {
            float inv_depth = 1.0f / rasterizer_window.fog.planar_maximum_depth;
            float inv_distance = 1.0f / rasterizer_window.fog.planar_maximum_distance;
            float density_from_depth;
            float density_from_distance;
            float blend;
            float plane_distance;
            float density_limit;

            density_from_depth = 1.0f - (rasterizer_window.fog.plane.d * inv_depth +
                                          -(rasterizer_window.fog.plane.normal.k * inv_depth) * context->center.z +
                                          -(rasterizer_window.fog.plane.normal.j * inv_depth) * context->center.y +
                                          -(inv_depth * rasterizer_window.fog.plane.normal.i) * context->center.x);
            clamp01_x87(density_from_depth);

            density_from_distance = 1.0f -
                (rasterizer_window.camera.forward.j * inv_distance * context->center.y +
                 rasterizer_window.camera.forward.k * inv_distance * context->center.z +
                 rasterizer_window.camera.forward.i * inv_distance * context->center.x -
                 (rasterizer_window.camera.position.x * rasterizer_window.camera.forward.i +
                  rasterizer_window.camera.position.y * rasterizer_window.camera.forward.j +
                  rasterizer_window.camera.position.z * rasterizer_window.camera.forward.k) * inv_distance);
            clamp01_x87(density_from_distance);

            blend = density_from_depth + density_from_distance;
            if (1.0f < blend) blend = 1.0f;
            blend = (1.0f - blend) * (1.0f - blend);

            plane_distance = -(((rasterizer_window.camera.position.x * rasterizer_window.fog.plane.normal.i +
                                 rasterizer_window.camera.position.y * rasterizer_window.fog.plane.normal.j +
                                 rasterizer_window.camera.position.z * rasterizer_window.fog.plane.normal.k) -
                                rasterizer_window.fog.plane.d) * inv_depth);
            clamp01_x87(plane_distance);

            density_limit = rasterizer_window.fog.planar_maximum_density;
            clamp01_x87(density_limit);

            halo::rasterizer::fields::planar_fog_attenuation = 1.0f - density_limit *
                (plane_distance * ((1.0f - density_from_distance) * (1.0f - density_from_distance) - blend) + blend);
        }
    }
}

}  // namespace rasterizer_model_draw_prepare_states_impl

namespace rasterizer_model_draw_restore_states_impl {


/**
 * Performs end-of-pass cleanup for the shader_environment renderer, restoring shared render state and clearing
 * the active-object pointer.
 *
 * @address 0x52b530
 */
void rasterizer_model_draw_restore_states(void)
{
    rasterizer_model_draw_context *context = (rasterizer_model_draw_context *)rasterizer_active_model_context;

    if (halo::rasterizer::fields::models_enabled == 0) {
        return;
    }

    if ((int8_t)context->flags < 0 && halo::rasterizer::fields::model_draw_mode == 0) {
        rasterizer_set_shader_stage_config(2);
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1 && (context->flags & 0x200) != 0) {
        float identity[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        render_device().set_transform(0x100, identity);
    }

    rasterizer_active_model_context = 0;
}

}  // namespace rasterizer_model_draw_restore_states_impl

namespace rasterizer_object_shadow_begin_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_object_shadow_begin. The original author notes are in
 * docs/original/rasterizer/rasterizer_object_shadow_begin.c.txt.
 *
 * @address 0x530ff0
 */
uint8_t rasterizer_object_shadow_begin(const real_matrix4x3 *projection, const ColorRGB *color, float radius, float *out_radius)
{
    float constants[5][4];
    d3d_surface_desc desc;
    d3d_viewport viewport;
    uint32_t clear_color;
    void *surface;
    float scale;
    uint32_t stage;

    if (rasterizer_window.type != 1) {
        return 1;
    }
    if (rasterizer_caps_flag_689 != 0 || halo::rasterizer::fields::object_shadows_enabled == 0) {
        if (out_radius != NULL) {
            *out_radius = 0.0f;
        }
        return 1;
    }
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture | halo::d3d9::ta::alpha_replicate);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    for (stage = 0; stage < 4; stage++) {
        render_device().set_texture(stage, 0);
    }

    scale = 1.0f / radius;
    constants[0][0] = scale * projection->forward.i;
    constants[0][1] = scale * projection->forward.j;
    constants[0][2] = scale * projection->forward.k;
    constants[0][3] = -((projection->position.x * projection->forward.i + projection->position.y * projection->forward.j +
                         projection->position.z * projection->forward.k) * scale);
    constants[1][0] = scale * projection->left.i;
    constants[1][1] = scale * projection->left.j;
    constants[1][2] = scale * projection->left.k;
    constants[1][3] = -((projection->position.x * projection->left.i + projection->position.y * projection->left.j +
                         projection->position.z * projection->left.k) * scale);
    constants[2][0] = 0.0f; constants[2][1] = 0.0f; constants[2][2] = 0.0f; constants[2][3] = 0.5f;
    constants[3][0] = 0.0f; constants[3][1] = 0.0f; constants[3][2] = 0.0f; constants[3][3] = 1.0f;
    constants[4][0] = 0.0f; constants[4][1] = 0.0f; constants[4][2] = 0.0f; constants[4][3] = 0.0f;
    render_device().set_vertex_shader_constant_f(0xd, &constants[0][0], 5);

    clear_color = console_debug_toggle_68941f ? 0x88888888 : 0;
    surface = (void *)(uintptr_t)rasterizer_render_targets[3].surface;
    render_device().set_render_target(0, (uint32_t)(uintptr_t)surface);
    rasterizer_active_render_target = 3;
    render_device().surface_get_desc(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    render_device().set_viewport(&viewport);
    render_device().clear(0, NULL, 1, clear_color, 1.0f, 0);
    rasterizer_set_shader_stage_config(0);

    rasterizer_object_shadow_projection = *projection;
    rasterizer_object_shadow_color = *color;
    rasterizer_object_shadow_radius = radius;
    if (out_radius != NULL) {
        *out_radius = radius;
    }
    rasterizer_object_shadow_model_context = NULL;
    rasterizer_object_shadow_prepared = 0;
    rasterizer_object_shadow_model_active = 0;
    rasterizer_object_shadow_window_restored = 0;
    return 1;
}

}  // namespace rasterizer_object_shadow_begin_impl

namespace rasterizer_object_shadow_blur_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_quad_vertex(int i, float x, float y, float u, float v)
{
    rasterizer_object_shadow_blur_quad[i].x = x;
    rasterizer_object_shadow_blur_quad[i].y = y;
    rasterizer_object_shadow_blur_quad[i].z = 0.0f;
    rasterizer_object_shadow_blur_quad[i].color = 0;
    rasterizer_object_shadow_blur_quad[i].u = u;
    rasterizer_object_shadow_blur_quad[i].v = v;
}

static void set_line_vertex(int i, float x, float y)
{
    rasterizer_object_shadow_border_lines[i].x = x;
    rasterizer_object_shadow_border_lines[i].y = y;
    rasterizer_object_shadow_border_lines[i].z = 0.0f;
    rasterizer_object_shadow_border_lines[i].rhw = 1.0f;
    rasterizer_object_shadow_border_lines[i].diffuse = 0;
    rasterizer_object_shadow_border_lines[i].u = 0.0f;
    rasterizer_object_shadow_border_lines[i].v = 0.0f;
}

/**
 * Direct3D 9 back end function rasterizer_object_shadow_blur. The original author notes are in
 * docs/original/rasterizer/rasterizer_object_shadow_blur.c.txt.
 *
 * @address 0x530830
 */
void rasterizer_object_shadow_blur(void)
{

    static const float k_offsets[8][4] = {
        { 1.0f, 0.0f, 0.0f, -0.00390625f }, { 0.0f, 1.0f, 0.0f, -0.00390625f },
        { 1.0f, 0.0f, 0.0f,  0.00390625f }, { 0.0f, 1.0f, 0.0f,  0.00390625f },
        { 1.0f, 0.0f, 0.0f, -0.00390625f }, { 0.0f, 1.0f, 0.0f,  0.00390625f },
        { 1.0f, 0.0f, 0.0f,  0.00390625f }, { 0.0f, 1.0f, 0.0f, -0.00390625f },
    };
    void *effect;
    void *surface;
    d3d_surface_desc desc;
    d3d_viewport viewport;
    uint32_t passes;
    uint32_t stage;
    uint32_t pass;

    if (rasterizer_caps_flag_689 != 0 || halo::rasterizer::fields::object_shadows_enabled == 0 || halo::rasterizer::fields::shadow_convolution_enabled == 0) {
        return;
    }
    effect = rasterizer_effects[45].effect;
    if (effect == NULL) {
        return;
    }
    for (stage = 0; stage < 4; stage++) {
        effect = rasterizer_effects[45].effect;
        render_device().effect_set_texture(effect, rasterizer_effects[45].texture_handles[stage], rasterizer_render_targets[3].texture);
        set_sampler_state(stage, halo::d3d9::ss::address_u, 3);
        set_sampler_state(stage, halo::d3d9::ss::address_v, 3);
        set_sampler_state(stage, halo::d3d9::ss::mag_filter, 2);
        set_sampler_state(stage, halo::d3d9::ss::min_filter, 2);
        set_sampler_state(stage, halo::d3d9::ss::mip_filter, 1);
    }
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    render_device().set_vertex_shader_constant_f(0xd, &k_offsets[0][0], 8);

    surface = (void *)(uintptr_t)rasterizer_render_targets[4].surface;
    render_device().set_render_target(0, (uint32_t)(uintptr_t)surface);
    rasterizer_active_render_target = 4;
    render_device().surface_get_desc(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    render_device().set_viewport(&viewport);

    set_quad_vertex(0, -1.0078125f, 1.0078125f, 0.0f, 0.0f);
    set_quad_vertex(1, 0.9921875f, 1.0078125f, 1.0f, 0.0f);
    set_quad_vertex(2, 0.9921875f, -0.9921875f, 1.0f, 1.0f);
    set_quad_vertex(3, -1.0078125f, -0.9921875f, 0.0f, 1.0f);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[0].shader);
    effect = rasterizer_effects[45].effect;
    render_device().effect_begin(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        effect = rasterizer_effects[45].effect;
        render_device().effect_pass(effect, pass);
        render_device().draw_primitive_up(6, 2, rasterizer_object_shadow_blur_quad, sizeof(rasterizer_dynamic_screen_vertex));
    }
    effect = rasterizer_effects[45].effect;
    render_device().effect_end(effect);

    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);

    set_line_vertex(0, -1.0f, 0.0f);
    set_line_vertex(1, 128.0f, 0.0f);
    set_line_vertex(2, 127.0f, -1.0f);
    set_line_vertex(3, 127.0f, 128.0f);
    set_line_vertex(4, 128.0f, 127.0f);
    set_line_vertex(5, -1.0f, 127.0f);
    set_line_vertex(6, 0.0f, 128.0f);
    set_line_vertex(7, 0.0f, -1.0f);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::diffuse);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    render_device().set_pixel_shader(0);
    render_device().set_vertex_shader(0);
    render_device().set_fvf(0x144);
    render_device().draw_primitive_up(2, 4, rasterizer_object_shadow_border_lines, sizeof(rasterizer_screen_vertex));
    render_device().set_fvf(0);
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_object_shadow_blur_impl

namespace rasterizer_object_shadow_model_draw_impl {


static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_object_shadow_model_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_object_shadow_model_draw.c.txt.
 *
 * @address 0x531350
 */
void rasterizer_object_shadow_model_draw(const ShaderModel *shader, int16_t frame, rasterizer_index_buffer *index_buffer, rasterizer_vertex_buffer *vertex_buffer)
{
    float constants[3][4];
    rasterizer_model_draw_context *context;

    if (rasterizer_window.type != 1 || rasterizer_caps_flag_689 != 0 || halo::rasterizer::fields::object_shadows_enabled == 0) {
        return;
    }
    if (shader->base.shader_type != 4) {
        return;
    }
    render_device().set_render_state(halo::d3d9::rs::cull_mode, (shader->shader_model_flags & 2) ? 1 : 3);
    if ((shader->shader_model_flags & 4) == 0) {
        chimera__rasterizer_set_texture(halo::tag_id_bits(shader->base_map.tag_id), 0, 0, 1, frame);
        set_sampler_state(0, halo::d3d9::ss::address_u, 1);
        set_sampler_state(0, halo::d3d9::ss::address_v, 1);
        set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
        set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
        set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    }
    context = rasterizer_object_shadow_model_context;
    constants[0][0] = shader->detail_map_scale;
    constants[0][1] = shader->detail_map_v_scale * shader->detail_map_scale;
    constants[0][2] = 1.0f;
    constants[0][3] = 1.0f;
    constants[1][0] = 1.0f; constants[1][1] = 0.0f; constants[1][2] = 0.0f; constants[1][3] = 0.0f;
    constants[2][0] = 0.0f; constants[2][1] = 1.0f; constants[2][2] = 0.0f; constants[2][3] = 0.0f;
    halo::shaders::shader_texture_animation_evaluate(reinterpret_cast<render_animation *>(&context->change_colors), reinterpret_cast<shader_texture_animation *>(const_cast<FunctionOut_t *>(&shader->u_animation_source)), constants[1], constants[2],
                                      context->base_map_u_scale * shader->map_u_scale,
                                      context->base_map_v_scale * shader->map_v_scale, 0.0f, 0.0f, 0.0f,
                                      (float)rasterizer_time.time);
    render_device().set_vertex_shader_constant_f(0xa, &constants[0][0], 3);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_buffer->type].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[33].shader);
    render_device().set_pixel_shader(0);
    rasterizer_dynamic_geometry_chain_draw(index_buffer->count, vertex_buffer, index_buffer);
}

}  // namespace rasterizer_object_shadow_model_draw_impl

namespace rasterizer_object_shadow_structure_draw_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_clamped_linear_sampler(uint32_t sampler)
{
    render_device().set_sampler_state(sampler, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::mip_filter, 2);
}

static float dot_position(const real_vector3d *v)
{
    const real_point3d *p = &rasterizer_object_shadow_projection.position;
    return p->x * v->i + p->y * v->j + p->z * v->k;
}

/**
 * Direct3D 9 back end function rasterizer_object_shadow_structure_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_object_shadow_structure_draw.c.txt.
 *
 * @address 0x531570
 */
void rasterizer_object_shadow_structure_draw(rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    const real_matrix4x3 *m = &rasterizer_object_shadow_projection;
    void *effect;
    uint32_t passes;
    uint32_t pass;

    if (rasterizer_window.type != 1 || rasterizer_caps_flag_689 != 0 || halo::rasterizer::fields::object_shadows_enabled == 0) {
        return;
    }
    if (rasterizer_effects[47].effect == 0) {
        return;
    }
    if (!rasterizer_object_shadow_prepared) {
        float inverse_radius = 1.0f / rasterizer_object_shadow_radius;
        float quarter_inverse = 1.0f / (rasterizer_object_shadow_radius * 4.0f);
        float double_inverse = 1.0f / (rasterizer_object_shadow_radius * 0.5f);
        float up_dot;
        float vs[5][4];
        float ps[4];

        if (halo::rasterizer::fields::shadow_convolution_enabled) {
            rasterizer_object_shadow_blur();
        }
        rasterizer_render_target_bind_texture_stage((int16_t)(halo::rasterizer::fields::shadow_convolution_enabled ? 4 : 3), 0);
        set_clamped_linear_sampler(0);
        chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(rasterizer_globals_data->linear_corner_fade.tag_id), 1, 0);
        set_clamped_linear_sampler(1);
        set_render_state(halo::d3d9::rs::cull_mode, 3);
        set_render_state(halo::d3d9::rs::color_write_enable, 0xf);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::zero);
        set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_color);
        set_render_state(halo::d3d9::rs::blend_op, 1);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
        set_render_state(halo::d3d9::rs::alpha_ref, 0);
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 3);
        set_render_state(halo::d3d9::rs::z_write_enable, 0);
        set_render_state(halo::d3d9::rs::fog_enable, 0);

        vs[0][0] = m->forward.i * inverse_radius * 0.5f;
        vs[0][1] = m->forward.j * inverse_radius * 0.5f;
        vs[0][2] = m->forward.k * inverse_radius * 0.5f;
        vs[0][3] = (1.0f - dot_position(&m->forward) * inverse_radius) * 0.5f;
        vs[1][0] = m->left.i * inverse_radius * -0.5f;
        vs[1][1] = m->left.j * inverse_radius * -0.5f;
        vs[1][2] = m->left.k * inverse_radius * -0.5f;
        vs[1][3] = (dot_position(&m->left) * inverse_radius + 1.0f) * 0.5f;

        up_dot = dot_position(&m->up);
        vs[2][0] = m->up.i * quarter_inverse;
        vs[2][1] = m->up.j * quarter_inverse;
        vs[2][2] = m->up.k * quarter_inverse;
        vs[2][3] = -(up_dot * quarter_inverse);
        vs[3][0] = -(m->up.i * double_inverse);
        vs[3][1] = -(m->up.j * double_inverse);
        vs[3][2] = -(m->up.k * double_inverse);
        vs[3][3] = up_dot * double_inverse;

        vs[4][0] = m->up.i;
        vs[4][1] = m->up.j;
        vs[4][2] = m->up.k;
        vs[4][3] = 0.0f;
        render_device().set_vertex_shader_constant_f(0xd, &vs[0][0], 5);
        ps[0] = 1.0f - rasterizer_object_shadow_color.red;
        ps[1] = 1.0f - rasterizer_object_shadow_color.green;
        ps[2] = 1.0f - rasterizer_object_shadow_color.blue;
        ps[3] = 1.0f;
        render_device().set_pixel_shader_constant_f(0, ps, 1);
        rasterizer_object_shadow_prepared = 1;
    }
    if (!rasterizer_object_shadow_window_restored) {
        rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
        rasterizer_object_shadow_window_restored = 1;
    }
    rasterizer_set_shader_stage_config(2);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_buffer->type].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[19].shader);
    effect = rasterizer_effects[47].effect;
    render_device().effect_begin(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        effect = rasterizer_effects[47].effect;
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot,
                                                                   first_primitive);
    }
    effect = rasterizer_effects[47].effect;
    render_device().effect_end(effect);
}

}  // namespace rasterizer_object_shadow_structure_draw_impl

namespace rasterizer_shader_model_draw_fixed_function_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_stage(uint32_t stage, uint32_t color_op, uint32_t color_arg1, uint32_t color_arg2,
                      uint32_t alpha_op, uint32_t alpha_arg1, uint32_t alpha_arg2)
{
    set_texture_stage_state(stage, halo::d3d9::ts::color_op, color_op);
    set_texture_stage_state(stage, halo::d3d9::ts::color_arg1, color_arg1);
    set_texture_stage_state(stage, halo::d3d9::ts::color_arg2, color_arg2);
    set_texture_stage_state(stage, halo::d3d9::ts::alpha_op, alpha_op);
    set_texture_stage_state(stage, halo::d3d9::ts::alpha_arg1, alpha_arg1);
    set_texture_stage_state(stage, halo::d3d9::ts::alpha_arg2, alpha_arg2);
}

static void set_transform(uint32_t state, const float *matrix)
{
    render_device().set_transform(state, matrix);
}

/**
 * Direct3D 9 back end function rasterizer_shader_model_draw_fixed_function. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_model_draw_fixed_function.c.txt.
 *
 * @address 0x529230
 */
void rasterizer_shader_model_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    rasterizer_model_draw_context *context = rasterizer_active_model_context;
    uint8_t decal = (smodel(shader)->shader_model_flags >> 3) & 1;
    uint16_t flags;
    uint8_t cull = 1;
    ColorRGB color;
    float texture_matrix[4][4];
    float alpha;
    uint32_t factor;
    int16_t source;
    int i, j;

    if (context->flags & 8) {
        set_render_state(halo::d3d9::rs::z_enable, 0);
    } else {
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_write_enable, decal ? 0 : 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        if (decal) {
            rasterizer_apply_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }
    flags = smodel(shader)->shader_model_flags;
    if (flags & 2) {
        if (flags & 0x20) {
            float dx, dy, dz;

            context = rasterizer_active_model_context;
            cull = 0;
            dx = context->center.x - rasterizer_window.camera.position.x;
            dy = context->center.y - rasterizer_window.camera.position.y;
            dz = context->center.z - rasterizer_window.camera.position.z;
            if (!((float)halo::libm::sqrt(dx * dx + dy * dy + dz * dz) > 8.0f)) {
                cull = 1;
            }
        } else {
            cull = 1;
        }
    }
    set_render_state(halo::d3d9::rs::cull_mode, cull ? 3 : 1);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, (decal || rasterizer_camouflage_fade_active) ? 1 : 0);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, (!rasterizer_camouflage_fade_active && !(smodel(shader)->shader_model_flags & 4)) ? 1 : 0);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled ? 1 : 0);

    context = rasterizer_active_model_context;
    source = *(int16_t *)&smodel(shader)->change_color_source;
    if (source > 0 && source < 5) {
        color = ((const ColorRGB *)(uintptr_t)context->change_colors)[source - 1];
    } else {
        color = *global_white_color;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            texture_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    halo::shaders::shader_texture_animation_evaluate(reinterpret_cast<render_animation *>(&context->change_colors), reinterpret_cast<shader_texture_animation *>(shader + 0xfc), texture_matrix[0], texture_matrix[1],
                                      context->base_map_u_scale * smodel(shader)->map_u_scale,
                                      context->base_map_v_scale * smodel(shader)->map_v_scale, 0.0f, 0.0f, 0.0f,
                                      (float)rasterizer_time.time);
    if (smodel(shader)->shader_model_flags & 2) {
        set_render_state(halo::d3d9::rs::cull_mode, 1);
    }
    alpha = rasterizer_camouflage_fade_active ? rasterizer_camouflage_fade : 1.0f;
    factor = (uint32_t)(int32_t)(alpha * 255.0f) << 8;
    factor = (factor | ((uint32_t)(int32_t)(color.red * 255.0f) & 0xff)) << 8;
    factor = (factor | ((uint32_t)(int32_t)(color.green * 255.0f) & 0xff)) << 8;
    factor = factor | ((uint32_t)(int32_t)(color.blue * 255.0f) & 0xff);
    set_render_state(halo::d3d9::rs::texture_factor, factor);

    if (rasterizer_active_model_context->flags & 0x200) {
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[14].declaration);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        chimera__rasterizer_set_texture(halo::tag_id_bits(smodel(shader)->base_map.tag_id), 0, 0, 1, frame);
        set_transform(0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        set_stage(0, 4, 2, 0, 4, 2, 3);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
    } else {
        rasterizer_vertex_buffer processed = *vertex_buffer;

        render_device().set_vertex_declaration(rasterizer_vertex_declarations[4].declaration);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        processed.hardware_buffer = index_buffer != NULL ? rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer) : NULL;
        processed.type = _rasterizer_vertex_type_model_processed;
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[15].declaration);
        set_transform(0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);

        source = *(int16_t *)&smodel(shader)->change_color_source;
        if (source > 0 && source != 2) {

            set_texture_stage_state(1, halo::d3d9::ts::texcoord_index, 0);
            set_transform(0x11, &texture_matrix[0][0]);
            set_texture_stage_state(1, halo::d3d9::ts::texture_transform_flags, 2);
            set_render_state(halo::d3d9::rs::fog_color, 0xff000000);
            chimera__rasterizer_set_texture(halo::tag_id_bits(smodel(shader)->base_map.tag_id), 0, 0, 1, frame);
            chimera__rasterizer_set_texture(halo::tag_id_bits(smodel(shader)->multipurpose_map.tag_id), 1, 0, 1, frame);
            set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
            set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
            if (rasterizer_camouflage_fade_active) {
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
                set_render_state(halo::d3d9::rs::blend_op, 1);
                set_stage(0, 4, 0, 2, 4, 3, 2);
                set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);
                set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
                set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            } else {
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::zero);
                set_render_state(halo::d3d9::rs::blend_op, 1);
                set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::diffuse);
                set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::texture);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::diffuse);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
                set_stage(1, 4, 1, 3, 4, 1, 2);
            }
            set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
            set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                      dynamic_vertex_slot);

            set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_rgb_float_to_int((const float *)(&rasterizer_window.fog.atmospheric_color)));
            set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
            set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
            set_render_state(halo::d3d9::rs::blend_op, 1);
            set_render_state(halo::d3d9::rs::z_write_enable, 0);
            set_render_state(halo::d3d9::rs::z_func, 3);
            set_stage(0, 4, 2, 0, 4, 3, 2);
            set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
            set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture | halo::d3d9::ta::complement | halo::d3d9::ta::alpha_replicate);
            set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
            set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                      dynamic_vertex_slot);
            set_texture_stage_state(1, halo::d3d9::ts::texcoord_index, 1);
            set_texture_stage_state(1, halo::d3d9::ts::texture_transform_flags, 0);
        } else {
            chimera__rasterizer_set_texture(halo::tag_id_bits(smodel(shader)->base_map.tag_id), 0, 0, 1, frame);
            set_stage(0, 4, 2, 0, 4, 2, 3);
            set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                      dynamic_vertex_slot);
        }
    }
    set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
    rasterizer_clear_decal_zbias();
}

}  // namespace rasterizer_shader_model_draw_fixed_function_impl

namespace rasterizer_shader_model_draw_limited_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_model_draw_limited. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_model_draw_limited.c.txt.
 *
 * @address 0x528be0
 */
void rasterizer_shader_model_draw_limited(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    rasterizer_model_draw_context *context = rasterizer_active_model_context;
    uint8_t decal = (smodel(shader)->shader_model_flags >> 3) & 1;
    uint16_t flags;
    uint8_t cull = 1;
    ColorRGB color;
    float texture_matrix[4][4];
    float alpha;
    uint32_t factor;
    int i, j;

    if (context->flags & 8) {
        set_render_state(halo::d3d9::rs::z_enable, 0);
    } else {
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_write_enable, decal ? 0 : 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        if (decal) {
            rasterizer_apply_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }
    flags = smodel(shader)->shader_model_flags;
    if (flags & 2) {
        if (flags & 0x20) {
            float dx, dy, dz;

            context = rasterizer_active_model_context;
            cull = 0;
            dx = context->center.x - rasterizer_window.camera.position.x;
            dy = context->center.y - rasterizer_window.camera.position.y;
            dz = context->center.z - rasterizer_window.camera.position.z;
            if (!((float)halo::libm::sqrt(dx * dx + dy * dy + dz * dz) > 8.0f)) {
                cull = 1;
            }
        } else {
            cull = 1;
        }
    }
    set_render_state(halo::d3d9::rs::cull_mode, cull ? 3 : 1);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, (decal || rasterizer_camouflage_fade_active) ? 1 : 0);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, (!rasterizer_camouflage_fade_active && !(smodel(shader)->shader_model_flags & 4)) ? 1 : 0);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled ? 1 : 0);

    context = rasterizer_active_model_context;
    {
        int16_t source = *(int16_t *)&smodel(shader)->change_color_source;

        if (source > 0 && source < 5) {
            const ColorRGB *colors = (const ColorRGB *)(uintptr_t)context->change_colors;

            color = colors[source - 1];
        } else {
            color = *global_white_color;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            texture_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    halo::shaders::shader_texture_animation_evaluate(reinterpret_cast<render_animation *>(&context->change_colors), reinterpret_cast<shader_texture_animation *>(shader + 0xfc), texture_matrix[0], texture_matrix[1],
                                      context->base_map_u_scale * smodel(shader)->map_u_scale,
                                      context->base_map_v_scale * smodel(shader)->map_v_scale, 0.0f, 0.0f, 0.0f,
                                      (float)rasterizer_time.time);
    if (smodel(shader)->shader_model_flags & 2) {
        set_render_state(halo::d3d9::rs::cull_mode, 1);
    }
    alpha = rasterizer_camouflage_fade_active ? rasterizer_camouflage_fade : 1.0f;
    factor = (uint32_t)(int32_t)(alpha * 255.0f) << 8;
    factor = (factor | ((uint32_t)(int32_t)(color.red * 255.0f) & 0xff)) << 8;
    factor = (factor | ((uint32_t)(int32_t)(color.green * 255.0f) & 0xff)) << 8;
    factor = factor | ((uint32_t)(int32_t)(color.blue * 255.0f) & 0xff);
    set_render_state(halo::d3d9::rs::texture_factor, factor);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::tfactor);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

    if (rasterizer_active_model_context->flags & 0x200) {

        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[14].declaration);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        chimera__rasterizer_set_texture(halo::tag_id_bits(smodel(shader)->base_map.tag_id), 0, 0, 1, frame);
        render_device().set_transform(0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
    } else {

        rasterizer_vertex_buffer processed = *vertex_buffer;

        render_device().set_vertex_declaration(rasterizer_vertex_declarations[4].declaration);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        processed.hardware_buffer = index_buffer != NULL ? rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer) : NULL;
        processed.type = _rasterizer_vertex_type_model_processed;
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[15].declaration);
        render_device().set_transform(0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 2);
        chimera__rasterizer_set_texture(halo::tag_id_bits(smodel(shader)->base_map.tag_id), 0, 0, 1, frame);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                  dynamic_vertex_slot);
        set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
    }
    rasterizer_clear_decal_zbias();
}

}  // namespace rasterizer_shader_model_draw_limited_impl

namespace rasterizer_shader_model_draw_pixel_shader_impl {


static int32_t set_render_state(uint32_t state, uint32_t value)
{
    return render_device().set_render_state(state, value);
}

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static void set_effect_vector(rasterizer_effect_slot *slot, int handle, float x, float y, float z, float w)
{
    void **handles = slot->constant_handles;
    void *effect = slot->effect;

    rasterizer_model_effect_vector[0] = x;
    rasterizer_model_effect_vector[1] = y;
    rasterizer_model_effect_vector[2] = z;
    rasterizer_model_effect_vector[3] = w;
    render_device().effect_set_vector(effect, handles[handle], rasterizer_model_effect_vector);
}

/**
 * Direct3D 9 back end function rasterizer_shader_model_draw_pixel_shader. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_model_draw_pixel_shader.c.txt.
 *
 * @address 0x529e00
 */
void rasterizer_shader_model_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    const ShaderModel *model = (const ShaderModel *)shader;
    rasterizer_model_draw_context *context;
    rasterizer_effect_slot *slot;
    uint8_t decal = (smodel(shader)->shader_model_flags >> 3) & 1;
    uint8_t ok = 1;
    uint8_t cull = 1;
    uint16_t true_atmospheric_fog;
    float depth;
    float reflection;
    float scale;
    ColorRGB animated;
    ColorRGB change;
    ColorRGB fog_add = { 0.0f, 0.0f, 0.0f };
    ColorRGB fog_planar = { 0.0f, 0.0f, 0.0f };
    ColorRGB fog_negative = { 0.0f, 0.0f, 0.0f };
    float fog_keep = 1.0f;
    int16_t vertex_shader;
    float reflection_constants[2][4];
    float detail_constants[3][4];
    uint32_t passes;
    uint32_t pass;

    slot = rasterizer_shader_model_select_technique(model);
    if (slot == NULL || slot->effect == 0) {
        return;
    }
    context = rasterizer_active_model_context;
    depth = (context->center.y - rasterizer_window.camera.position.y) * rasterizer_window.camera.forward.j +
            (context->center.z - rasterizer_window.camera.position.z) * rasterizer_window.camera.forward.k +
            (context->center.x - rasterizer_window.camera.position.x) * rasterizer_window.camera.forward.i;
    if (model->reflection_cutoff_distance != 0.0f) {
        float t = (depth - model->reflection_cutoff_distance) /
                  (model->reflection_falloff_distance - model->reflection_cutoff_distance);

        reflection = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    } else {
        reflection = 1.0f;
    }

    if (context->flags & 8) {
        set_render_state(halo::d3d9::rs::z_enable, 0);
    } else {
        if (!rasterizer_camouflage_fade_active) {
            set_render_state(halo::d3d9::rs::z_enable, 1);
            set_render_state(halo::d3d9::rs::z_func, 4);
            set_render_state(halo::d3d9::rs::z_write_enable, decal ? 0 : 1);
        }
        if (decal) {
            rasterizer_apply_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }
    if (smodel(shader)->shader_model_flags & 2) {
        if (smodel(shader)->shader_model_flags & 0x20) {
            float distance = (float)halo::math::vector3d_distance(rasterizer_active_model_context->center,
                                                      rasterizer_window.camera.position);

            cull = 0;
            if (!(distance > 8.0f)) {
                cull = 1;
            }
        } else {
            cull = 1;
        }
    }
    set_render_state(halo::d3d9::rs::cull_mode, cull ? 3 : 1);
    if (!rasterizer_camouflage_fade_active) {
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
    }
    set_render_state(halo::d3d9::rs::alpha_blend_enable, (decal || rasterizer_camouflage_fade_active) ? 1 : 0);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, (!rasterizer_camouflage_fade_active && !decal && !(smodel(shader)->shader_model_flags & 4)) ? 1 : 0);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4) {
        set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled ? 1 : 0);
    } else {
        set_render_state(halo::d3d9::rs::fog_enable, (smodel(shader)->shader_model_flags >> 4) & 1);
    }
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(((struct ShaderModel *)shader)->base_map.tag_id), 0, 0, 1, frame, slot);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(((struct ShaderModel *)shader)->detail_map.tag_id), 0, 1, 2, frame, slot);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(((struct ShaderModel *)shader)->multipurpose_map.tag_id), 0, 2, 1, frame, slot);
    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(((struct ShaderModel *)shader)->reflection_cube_map.tag_id), 2, 3, 0, frame, slot);

    context = rasterizer_active_model_context;
    animated.red = 0.0f;
    animated.green = 0.0f;
    animated.blue = 0.0f;
    if (rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_4 || model->detail_mask == 0) {

        float phase;
        float value;
        ColorRGB delta;

        scale = 1.0f;
        if (shader[0x6c] & 1) {
            phase = 0.0f;
        } else {
            uint32_t seed = (context->object_index * 0x19660d + 0x3c6ef35f) >> 16;

            phase = (float)seed * 1.5259022e-05f;
        }
        delta.red = model->animation_color_upper_bound.red - model->animation_color_lower_bound.red;
        delta.green = model->animation_color_upper_bound.green - model->animation_color_lower_bound.green;
        delta.blue = model->animation_color_upper_bound.blue - model->animation_color_lower_bound.blue;
        value = (float)halo::math::periodic_function_evaluate((periodic_function_t)model->animation_function,
                                                  phase + rasterizer_time.time / model->animation_period);
        animated.red = delta.red * value + model->animation_color_lower_bound.red;
        animated.green = delta.green * value + model->animation_color_lower_bound.green;
        animated.blue = delta.blue * value + model->animation_color_lower_bound.blue;
        if (model->color_source > 0 && model->color_source < 5) {
            const ColorRGB *colors = (const ColorRGB *)(uintptr_t)context->change_colors;
            const ColorRGB *source = &colors[model->color_source - 1];

            animated.red *= source->red;
            animated.green *= source->green;
            animated.blue *= source->blue;
            if (model->detail_mask == 0 && model->color_source == 2 &&
                rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4) {
                float distance = (float)halo::libm::fabs(halo::math::vector3d_distance(context->center, rasterizer_window.camera.position));

                if (distance < 6.0f) {
                    scale = 1.0f - distance * 0.16666667f;
                }
            }
        }
        rasterizer_model_effect_vector[3] = 1.0f;
        rasterizer_model_effect_vector[0] = animated.red * scale;
        rasterizer_model_effect_vector[1] = animated.green * scale;
        rasterizer_model_effect_vector[2] = animated.blue * scale;
        if (slot->constant_handles != 0) {
            void **handles = slot->constant_handles;
            void *effect = slot->effect;

            render_device().effect_set_vector(effect, handles[4], rasterizer_model_effect_vector);
        }
    }
    context = rasterizer_active_model_context;
    if (model->change_color_source > 0 && model->change_color_source < 5) {
        change = ((const ColorRGB *)(uintptr_t)context->change_colors)[model->change_color_source - 1];
    } else {
        change = *global_white_color;
    }

    true_atmospheric_fog = *(uint16_t *)&((struct ShaderModel *)shader)->shader_model_flags & 0x10;
    if (true_atmospheric_fog) {
        vertex_shader = 0x1c;
    } else if (halo::rasterizer::fields::planar_fog_vertex_shader_active) {
        vertex_shader = 0x19;
    } else if (context->lighting.point_light_count > 0) {
        vertex_shader = 0x1a;
    } else if (context->node_count > 1) {
        vertex_shader = 0x1c;
    } else if (halo::tag_id_bits<int32_t>(((struct ShaderModel *)shader)->multipurpose_map.tag_id) != -1 &&
               (model->detail_mask != 0 ||
                animated.red != 0.0f || animated.green != 0.0f || animated.blue != 0.0f ||
                change.red != 1.0f || change.green != 1.0f || change.blue != 1.0f)) {
        vertex_shader = 0x1c;
    } else if (halo::tag_id_bits<int32_t>(((struct ShaderModel *)shader)->reflection_cube_map.tag_id) != -1 && reflection > 0.0f) {
        vertex_shader = 0x1c;
    } else {
        vertex_shader = 0x1d;
    }

    if (!rasterizer_fog_enabled) {

    } else if (true_atmospheric_fog) {
        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4) {
            set_render_state(halo::d3d9::rs::fog_enable, 1);
            set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_rgb_float_to_int((const float *)(&rasterizer_window.fog.atmospheric_color)));
        } else {

            fog_add = animated;
        }
    } else if (!(context->flags & 4)) {
        const render_fog *fog = &rasterizer_window.fog;
        float height = clamp01((fog->plane.normal.i * rasterizer_window.camera.position.x +
                                fog->plane.normal.k * rasterizer_window.camera.position.z +
                                fog->plane.normal.j * rasterizer_window.camera.position.y - fog->plane.d) /
                               fog->atmospheric_maximum_distance);
        float density = clamp01((depth - fog->atmospheric_minimum_distance) /
                                (fog->atmospheric_maximum_distance - fog->atmospheric_minimum_distance)) *
                        fog->atmospheric_maximum_density;

        if (fog->flags & 2) {
            height = 1.0f;
        }
        fog_keep = 1.0f - density;
        fog_planar.red = fog->planar_color.red -
                         (fog->atmospheric_color.red * (1.0f - height) + height * fog->planar_color.red) * density;
        fog_planar.green = fog->planar_color.green -
                           (fog->planar_color.green * height + (1.0f - height) * fog->atmospheric_color.green) * density;
        fog_planar.blue = fog->planar_color.blue -
                          (fog->planar_color.blue * height + (1.0f - height) * fog->atmospheric_color.blue) * density;
        fog_negative.red = clamp01(-fog_planar.red);
        fog_negative.green = clamp01(-fog_planar.green);
        fog_negative.blue = clamp01(-fog_planar.blue);
        fog_planar.red = clamp01(fog_planar.red);
        fog_planar.green = clamp01(fog_planar.green);
        fog_planar.blue = clamp01(fog_planar.blue);
        fog_add.red = density * fog->atmospheric_color.red;
        fog_add.green = fog->atmospheric_color.green * density;
        fog_add.blue = fog->atmospheric_color.blue * density;
        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4) {
            if (model->detail_mask != 0 && vertex_shader == 0x19) {
                ColorRGB fixed_function_fog;

                fixed_function_fog.red = clamp01(clamp01(fog_add.red - halo::rasterizer::fields::planar_fog_attenuation * fog_negative.red) + fog_planar.red);
                fixed_function_fog.green = clamp01(clamp01(fog_add.green - halo::rasterizer::fields::planar_fog_attenuation * fog_negative.green) +
                                                   fog_planar.green);
                fixed_function_fog.blue = clamp01(clamp01(fog_add.blue - halo::rasterizer::fields::planar_fog_attenuation * fog_negative.blue) +
                                                  fog_planar.blue);
                set_render_state(halo::d3d9::rs::fog_enable, 1);
                set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_rgb_float_to_int((const float *)&fixed_function_fog));
            } else {
                set_render_state(halo::d3d9::rs::fog_enable, 0);
            }
        }
    }
    if (slot->constant_handles != 0) {
        set_effect_vector(slot, 0, change.red, change.green, change.blue,
                          rasterizer_camouflage_fade_active ? rasterizer_camouflage_fade : 1.0f);
        set_effect_vector(slot, 1, fog_planar.red, fog_planar.green, fog_planar.blue, fog_keep);
        set_effect_vector(slot, 2, fog_negative.red, fog_negative.green, fog_negative.blue, 1.0f);
        set_effect_vector(slot, 3, fog_add.red, fog_add.green, fog_add.blue, 1.0f);
    }

    if (render_device().set_vertex_declaration(rasterizer_vertex_declarations[4].declaration) < 0) {
        ok = 0;
    }
    if (render_device().set_vertex_shader(rasterizer_vertex_shaders[vertex_shader].shader) < 0) {
        ok = 0;
    }

    context = rasterizer_active_model_context;
    {
        const ColorARGB *tint = &context->lighting.reflection_tint;
        float perpendicular[4], parallel[4];

        perpendicular[3] = reflection * model->perpendicular_brightness * tint->alpha;
        perpendicular[0] = model->perpendicular_tint_color.red * tint->red;
        perpendicular[1] = model->perpendicular_tint_color.green * tint->green;
        perpendicular[2] = model->perpendicular_tint_color.blue * tint->blue;
        parallel[3] = reflection * model->parallel_brightness * tint->alpha;
        parallel[0] = model->parallel_tint_color.red * tint->red;
        parallel[1] = model->parallel_tint_color.green * tint->green;
        parallel[2] = model->parallel_tint_color.blue * tint->blue;
        reflection_constants[0][0] = perpendicular[0] - parallel[0];
        reflection_constants[0][1] = perpendicular[1] - parallel[1];
        reflection_constants[0][2] = perpendicular[2] - parallel[2];
        reflection_constants[0][3] = perpendicular[3] - parallel[3];
        reflection_constants[1][0] = parallel[0];
        reflection_constants[1][1] = parallel[1];
        reflection_constants[1][2] = parallel[2];
        reflection_constants[1][3] = parallel[3];
    }
    detail_constants[0][0] = model->detail_map_scale;
    detail_constants[0][1] = model->detail_map_v_scale * model->detail_map_scale;
    detail_constants[0][2] = 1.0f;
    detail_constants[0][3] = 1.0f;
    detail_constants[1][0] = 1.0f;
    detail_constants[1][1] = 0.0f;
    detail_constants[1][2] = 0.0f;
    detail_constants[1][3] = 0.0f;
    detail_constants[2][0] = 0.0f;
    detail_constants[2][1] = 1.0f;
    detail_constants[2][2] = 0.0f;
    detail_constants[2][3] = 0.0f;
    halo::shaders::shader_texture_animation_evaluate(reinterpret_cast<render_animation *>(&context->change_colors), reinterpret_cast<shader_texture_animation *>(shader + 0xfc), detail_constants[1], detail_constants[2],
                                      context->base_map_u_scale * model->map_u_scale, context->base_map_v_scale * model->map_v_scale,
                                      0.0f, 0.0f, 0.0f, (float)rasterizer_time.time);
    detail_constants[2][2] = model->translucency;
    if (render_device().set_vertex_shader_constant_f(10, &detail_constants[0][0], 3) < 0) {
        ok = 0;
    }
    if (render_device().set_vertex_shader_constant_f(0xd, &reflection_constants[0][0], 2) < 0) {
        ok = 0;
    }
    if (rasterizer_model_ambient_reflection_tint != NULL &&
        (rasterizer_model_ambient_reflection_tint[0] > 0.0f || rasterizer_model_ambient_reflection_tint[1] > 0.0f ||
         rasterizer_model_ambient_reflection_tint[2] > 0.0f || rasterizer_model_ambient_reflection_tint[3] > 0.0f)) {
        float override_constants[2][4];

        override_constants[0][0] = 0.0f;
        override_constants[0][1] = 0.0f;
        override_constants[0][2] = 0.0f;
        override_constants[0][3] = 0.0f;
        override_constants[1][0] = rasterizer_model_ambient_reflection_tint[0];
        override_constants[1][1] = rasterizer_model_ambient_reflection_tint[1];
        override_constants[1][2] = rasterizer_model_ambient_reflection_tint[2];
        override_constants[1][3] = rasterizer_model_ambient_reflection_tint[3];
        if (render_device().set_vertex_shader_constant_f(0xd, &override_constants[0][0], 2) < 0) {
            ok = 0;
        }
    }
    if (ok) {
        render_device().effect_begin(slot->effect, &passes, 3);
        for (pass = 0; pass < passes; pass++) {
            render_device().effect_pass(slot->effect, pass);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                      dynamic_vertex_slot);
        }
        render_device().effect_end(slot->effect);

        if ((smodel(shader)->shader_model_flags & 2) && cull) {

            context = rasterizer_active_model_context;
            detail_constants[0][0] = model->detail_map_scale;
            detail_constants[0][1] = model->detail_map_v_scale * model->detail_map_scale;
            detail_constants[0][2] = 1.0f;
            detail_constants[0][3] = -1.0f;
            detail_constants[1][0] = 1.0f;
            detail_constants[1][1] = 0.0f;
            detail_constants[1][2] = 0.0f;
            detail_constants[1][3] = 0.0f;
            detail_constants[2][0] = 0.0f;
            detail_constants[2][1] = 1.0f;
            detail_constants[2][2] = 0.0f;
            detail_constants[2][3] = 0.0f;
            halo::shaders::shader_texture_animation_evaluate(reinterpret_cast<render_animation *>(&context->change_colors), reinterpret_cast<shader_texture_animation *>(shader + 0xfc), detail_constants[1], detail_constants[2],
                                              context->base_map_u_scale * model->map_u_scale,
                                              context->base_map_v_scale * model->map_v_scale, 0.0f, 0.0f, 0.0f,
                                              (float)rasterizer_time.time);
            detail_constants[2][2] = model->translucency;
            render_device().set_vertex_shader_constant_f(10, &detail_constants[0][0], 3);
            render_device().effect_begin(slot->effect, &passes, 3);
            for (pass = 0; pass < passes; pass++) {
                render_device().effect_pass(slot->effect, pass);
                set_render_state(halo::d3d9::rs::cull_mode, 2);

                if (index_buffer != NULL) {
                    if (vertex_buffer != NULL) {
                        rasterizer_dynamic_geometry_chain_draw(primitive_count, vertex_buffer, index_buffer);
                    } else {
                        rasterizer_dynamic_vertex_draw_indexed(index_buffer, primitive_count, dynamic_vertex_slot);
                    }
                } else if (vertex_buffer != NULL) {
                    chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer,
                                                                               dynamic_index_slot, 0);
                } else {
                    rasterizer_dynamic_index_cache_draw(dynamic_index_slot, 0, primitive_count, dynamic_vertex_slot);
                }
            }
            render_device().effect_end(slot->effect);
        }
    }

    rasterizer_clear_decal_zbias();
}

}  // namespace rasterizer_shader_model_draw_pixel_shader_impl

namespace rasterizer_shader_model_select_technique_impl {


/**
 * Direct3D 9 back end function rasterizer_shader_model_select_technique. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_model_select_technique.c.txt.
 *
 * @address 0x527500
 */
rasterizer_effect_slot * rasterizer_shader_model_select_technique(const ShaderModel *shader)
{
    uint16_t flags = shader->shader_model_flags;
    int32_t index = shader->detail_function;
    rasterizer_effect_slot *slot;
    int32_t technique;

    if (flags & 1) {
        index += 3;
    }
    if (rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_4 && !(flags & 0x10)) {
        index += (shader->detail_mask == 0) ? 6 : 0xc;
    }
    switch (shader->detail_mask) {
    case 0:
        slot = &rasterizer_effects[121];
        if (slot->effect == 0) {
            return NULL;
        }
        if (rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_1 && rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_4 &&
            shader->color_source == 2 &&
            halo::libm::fabs(halo::math::vector3d_distance(rasterizer_active_model_context->center, rasterizer_window.camera.position)) < 6.0) {
            index += 6;
        }
        technique = environment_techniques_plain[index];
        break;
    case 1: slot = &rasterizer_effects[120]; technique = environment_techniques_reflection[index + 6]; break;
    case 2: slot = &rasterizer_effects[120]; technique = environment_techniques_reflection[index]; break;
    case 3: slot = &rasterizer_effects[117]; technique = environment_techniques_self_illumination[index + 6]; break;
    case 4: slot = &rasterizer_effects[117]; technique = environment_techniques_self_illumination[index]; break;
    case 5: slot = &rasterizer_effects[118]; technique = environment_techniques_change_color[index + 6]; break;
    case 6: slot = &rasterizer_effects[118]; technique = environment_techniques_change_color[index]; break;
    case 7: slot = &rasterizer_effects[119]; technique = environment_techniques_multipurpose[index + 6]; break;
    case 8: slot = &rasterizer_effects[119]; technique = environment_techniques_multipurpose[index]; break;
    default:
        return NULL;
    }
    if (slot->effect == 0) {
        return NULL;
    }
    if (render_device().effect_set_technique(slot->effect, technique) < 0) {
        return NULL;
    }
    return slot;
}

}  // namespace rasterizer_shader_model_select_technique_impl

}  // namespace halo::rasterizer
