/**
 * @file src/rasterizer/transparent_geometry.cpp
 * The transparent geometry group queue: pools, depth sort and group drawing.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/shader_access.hpp"
#include "halo/core/bit_cast.hpp"
#include <cstring>
#include "internal/state.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/shaders/api.hpp"
#include "halo/math/api.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/shaders/shaders.hpp"




namespace halo::rasterizer {

namespace {

constexpr int16_t k_particle_effect_nonlinear_tint = 0x5a;
constexpr int16_t k_particle_effect_linear_tint = 0x60;

/** High half of the packed vertex type reference a group reports (low half is the vertex type). */
constexpr uint32_t k_vertex_type_reference_flag = ~0xffffu;

}  // namespace

/**
 * Returns whether the current mode and driver capabilities allow transparent-decal/shader-based geometry-group
 * rendering to proceed.
 *
 * @address 0x519ac0
 */
int rasterizer_transparent_decals_enabled(void)
{
    if (rasterizer_window.type != 1 || halo::rasterizer::fields::active_camouflage_enabled == 0 ||
        (rasterizer_caps_flag_688 == 0 &&
         (rasterizer_caps_flag_68a == 0 && rasterizer_caps.pixel_shader_version > halo::d3d9::k_pixel_shader_version_1_0))) {
        return 0;
    }
    return 1;
}

/**
 * Direct3D 9 back end function rasterizer_transparent_geometry_group_build. The original author notes are in
 * docs/original/rasterizer/rasterizer_transparent_geometry_group_build.c.txt.
 *
 * @address 0x52b180
 */
transparent_geometry_group * rasterizer_transparent_geometry_group_build(transparent_geometry_group_link *link, Shader *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot, const real_point3d *position)
{
    rasterizer_model_draw_context *context;
    transparent_geometry_group *allocated = NULL;
    transparent_geometry_group *group;
    uint8_t skip;
    uint8_t test_immediate;
    uint32_t flags;

    if (!halo::rasterizer::fields::models_enabled || !console_debug_toggle_6893ed) {
        return NULL;
    }
    Shader *tag = shader;
    const ShaderModel *model_shader = shader_cast<ShaderModel>(tag);
    skip = (tag != NULL && tag->shader_type == 4 && halo::test_flag(model_shader->shader_model_flags, halo::tags::shader_model_tag_flag::alpha_blended_decal));
    if (rasterizer_active_model_mode != 1) {
        test_immediate = 1;
    } else {
        test_immediate = (tag != NULL && tag->shader_type == 4 && model_shader->shader_model_flags != 0);
    }
    if (skip) {
        if (link != NULL) {
            link->group_index = -1;
            link->previous_group_index = 0;
            link->next_group_index = 0;
        }
        return NULL;
    }

    bool use_immediate_group = false;

    context = rasterizer_active_model_context;
    flags = context->flags;
    if (test_immediate) {
        if (halo::shaders::shader_view(tag).is_decal()) {
            flags |= _group_flag_0_bit | _group_immediate_bit;
        }
        if (flags & _group_immediate_bit) {
            group = &transparent_geometry_group_environment_immediate;
            group->sorted_index = -1;
            use_immediate_group = true;
        }
    }
    if (!use_immediate_group) {
        if (rasterizer_active_model_mode == 1 && tag != NULL && tag->shader_type != 4) {
            group = transparent_geometry_group_allocate_secondary();
        } else {
            group = transparent_geometry_group_allocate();
            allocated = group;
        }
        if (link != NULL) {
            link->group_index = transparent_geometry_group_index_from_pointer(group);
            link->previous_group_index = (uint32_t)(uintptr_t)&group->previous_group_index;
            link->next_group_index = (uint32_t)(uintptr_t)&group->next_group_index;
        }
        if (group == NULL) {
            transparent_geometry_group_overflow_c = 1;
            return allocated;
        }
    }

    group->flags = flags;
    group->object_index = context->object_index;
    if (flags & _group_node_parts_bit) {
        group->node_part_indices = rasterizer_node_part_indices;
        group->node_part_count = rasterizer_node_part_count;
    } else {
        group->node_part_indices = 0;
        group->node_part_count = 0;
    }
    if (context->group_parameters.mode == 0) {
        group->sort_key = 0;
        group->position = *position;
    } else {
        group->sort_key = context->group_parameters.sort_key;
        group->position = context->group_parameters.position;
    }
    group->shader_permutation = (uint16_t)frame;
    group->shader = tag;
    group->parameters = context->group_parameters;
    group->index_buffer = index_buffer;
    group->primitive_count = primitive_count;
    group->dynamic_index_slot = dynamic_index_slot;
    group->dynamic_vertex_slot = dynamic_vertex_slot;
    group->vertex_buffer = vertex_buffer;
    group->first_index = 0;
    group->lightmap_bitmap = 0;
    group->tint.alpha = 0.0f;
    group->tint.red = 0.0f;
    group->tint.green = 0.0f;
    group->tint.blue = 0.0f;
    group->depth = -(rasterizer_window.camera.forward.i * (group->position.x - rasterizer_window.camera.position.x) +
                     rasterizer_window.camera.forward.j * (group->position.y - rasterizer_window.camera.position.y) +
                     rasterizer_window.camera.forward.k * (group->position.z - rasterizer_window.camera.position.z));
    group->base_map_u_scale = context->base_map_u_scale;
    group->base_map_v_scale = context->base_map_v_scale;
    group->previous_group_index = -1;
    group->next_group_index = -1;
    if (rasterizer_active_model_mode == 1 && tag->shader_type != 4) {
        group->parent_sort_key = context->group_parameters.sort_key;
    } else {
        group->parent_sort_key = 0;
    }
    group->first_person = model_render_first_person;

    if (flags & _group_immediate_bit) {

        group->node_matrices = context->node_matrices;
        group->node_count = context->node_count;
        group->lighting = &context->lighting;
        group->lighting_extra = reinterpret_cast<render_animation *>(&context->change_colors);
        transparent_geometry_group_last_drawn_key = 0;
        rasterizer_secondary_groups_drawn = 0;
        rasterizer_transparent_geometry_group_draw(group, 0);
        rasterizer_render_states_dirty = 1;
        return allocated;
    }

    if (!rasterizer_model_scratch_valid) {
        rasterizer_model_scratch_node_matrices =
            chimera__rasterizer_memory_alloc(context->node_matrices,
                                             (uint32_t)(context->node_count * 0x34));
        rasterizer_model_scratch_node_count = context->node_count;
        rasterizer_model_scratch_lighting = chimera__rasterizer_memory_alloc(&context->lighting, 0x74);
        rasterizer_model_scratch_function_source = chimera__rasterizer_memory_alloc(&context->change_colors, 8);
        rasterizer_model_scratch_valid = 1;
    }
    group->node_matrices = static_cast<real_matrix4x3 *>(rasterizer_model_scratch_node_matrices);
    group->lighting = static_cast<render_lighting *>(rasterizer_model_scratch_lighting);
    group->node_count = rasterizer_model_scratch_node_count;
    group->lighting_extra = static_cast<render_animation *>(rasterizer_model_scratch_function_source);
    return allocated;
}









static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static void set_vertex_declaration(void *declaration)
{
    render_device().set_vertex_declaration(declaration);
}

static void set_vertex_shader(void *shader)
{
    render_device().set_vertex_shader(shader);
}

static void draw_effect_passes(void *effect, transparent_geometry_group *group)
{
    uint32_t passes;
    uint32_t pass;

    render_device().effect_begin(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        render_device().effect_pass(effect, pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    render_device().effect_end(effect);
}

static void set_depth_prepass_states(uint32_t cull_mode, uint32_t texture_factor)
{
    set_render_state(halo::d3d9::rs::cull_mode, cull_mode);
    set_render_state(halo::d3d9::rs::color_write_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 1);
    set_render_state(halo::d3d9::rs::z_write_enable, 1);
    set_render_state(halo::d3d9::rs::z_func, 4);
    set_render_state(halo::d3d9::rs::texture_factor, texture_factor);
    render_device().set_pixel_shader(0);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::tfactor);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::tfactor);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
}

static void set_group_skinning(const transparent_geometry_group *flags_group,
                               const transparent_geometry_group *source)
{
    rasterizer_node_matrices nodes;

    if (source->node_matrices != 0 && source->node_count != 0) {
        nodes.matrices = source->node_matrices;
        nodes.node_count = source->node_count;
    } else {
        nodes.matrices = k_render_identity_matrix_ptr;
        nodes.node_count = 1;
    }
    chimera__rasterizer_set_model_skinning((uint8_t)((flags_group->flags & _group_node_parts_bit) == 0), &nodes);
}

static int16_t shader_type_of(const Shader *shader)
{
    return static_cast<int16_t>(shader->shader_type);
}

static void draw_particle_effect_shader(transparent_geometry_group *group, const Shader *shader)
{
    const shader_effect *fx = shader_cast<shader_effect>(shader);
    uint8_t has_texture_animation;
    int16_t effect_index;
    rasterizer_effect_slot *effect;
    BitmapData *bitmap;
    float view_matrix[3][4];
    float texture_matrix[4][4];
    int i, j;

    has_texture_animation = 0;
    if (has_tag(fx->secondary_map) && fx->anchor != 2) {
        has_texture_animation = 1;
    }
    effect_index = halo::test_flag(fx->flags, halo::tags::particle_shader_tag_flag::nonlinear_tint) ? k_particle_effect_nonlinear_tint : k_particle_effect_linear_tint;
    if ((group->flags & _group_flag_4_bit) == 0) {
        switch (fx->framebuffer_blend_function) {
        case 0: effect_index += 2; break;
        case 1: case 5: effect_index += 4; break;
        case 2: effect_index += 3; break;
        case 3: case 4: case 6: effect_index += 1; break;
        case 7: effect_index += 5; break;
        default: break;
        }
    }
    effect = &rasterizer_effects[effect_index];
    if (effect->effect == 0) {
        return;
    }

    bitmap = group->lightmap_bitmap;
    if (bitmap != NULL && ((struct BitmapData *)bitmap)->hardware_texture != 0) {
        uint8_t map_flags = static_cast<uint8_t>(fx->map_flags);
        uint32_t filter = halo::test_flag(fx->map_flags, halo::tags::is_unfiltered_flag_tag_flag::unfiltered) ? 1 : 2;

        rasterizer_bind_texture_d3d9(0, bitmap);
        set_sampler_state(0, halo::d3d9::ss::address_u, (map_flags & 2) | 1);
        set_sampler_state(0, halo::d3d9::ss::address_v, ((map_flags & 4) | 2) >> 1);
        set_sampler_state(0, halo::d3d9::ss::mag_filter, filter);
        set_sampler_state(0, halo::d3d9::ss::min_filter, filter);
        set_sampler_state(0, halo::d3d9::ss::mip_filter, filter);
    }
    set_render_state(halo::d3d9::rs::cull_mode, 1);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    chimera__rasterizer_set_framebuffer_blend_function(fx->framebuffer_blend_function);

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            view_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            texture_matrix[i][j] = (i == j && i < 2) ? 1.0f : 0.0f;
        }
    }
    if (group->flags & _group_flag_20_bit) {

        const real_matrix4x3 *view_to_world = &rasterizer_window.frustum.view_to_world;
        const real_point3d *camera = &rasterizer_window.camera.position;

        view_matrix[0][0] = view_to_world->forward.i;
        view_matrix[0][1] = view_to_world->left.i;
        view_matrix[0][2] = view_to_world->up.i;
        view_matrix[0][3] = camera->x;
        view_matrix[1][0] = view_to_world->forward.j;
        view_matrix[1][1] = view_to_world->left.j;
        view_matrix[1][2] = view_to_world->up.j;
        view_matrix[1][3] = camera->y;
        view_matrix[2][0] = view_to_world->forward.k;
        view_matrix[2][1] = view_to_world->left.k;
        view_matrix[2][2] = view_to_world->up.k;
        view_matrix[2][3] = camera->z;
    }
    if (has_texture_animation) {
        halo::shaders::shader_texture_animation_evaluate(group->lighting_extra, const_cast<shader_texture_animation *>(&fx->texture_animation),
                                          texture_matrix[2], texture_matrix[3],
                                          group->base_map_u_scale, group->base_map_v_scale, 0.0f, 0.0f, 0.0f,
                                          (float)rasterizer_time.time);
    }
    render_device().set_vertex_shader_constant_f(0x1a, &view_matrix[0][0], 3);
    render_device().set_vertex_shader_constant_f(0x0d, &texture_matrix[0][0], 4);
    set_vertex_declaration(rasterizer_vertex_declarations[6].declaration);
    set_vertex_shader(rasterizer_vertex_shaders[effect->vertex_shader_index].shader);
    draw_effect_passes(effect->effect, group);
}

static void draw_glass_shader(transparent_geometry_group *group, const Shader *shader)
{
    const ShaderTransparentGlass *glass = shader_cast<ShaderTransparentGlass>(shader);
    int16_t reflection_type = glass->reflection_type;

    set_render_state(halo::d3d9::rs::cull_mode, halo::test_flag(glass->shader_transparent_glass_flags, halo::tags::shader_transparent_glass_tag_flag::two_sided) ? 1 : 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_ref, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);

    if (reflection_type == 2 && (rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1)) {

        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
            rasterizer_glass_draw_procedures.reflection(group, 2);
        }
        return;
    }
    if (has_tag(glass->background_tint_map) ||
        glass->background_tint_color.red != 0.0f ||
        glass->background_tint_color.green != 0.0f ||
        glass->background_tint_color.blue != 0.0f) {
        rasterizer_glass_draw_procedures.tint(group);
    }
    if ((glass->perpendicular_brightness > 0.0f || glass->parallel_brightness > 0.0f) &&
        (has_tag(glass->reflection_map) || reflection_type == 2)) {
        if (reflection_type == 0 &&
            (halo::test_flag(glass->shader_transparent_glass_flags, halo::tags::shader_transparent_glass_tag_flag::bump_map_is_specular_mask) || !has_tag(glass->bump_map))) {
            reflection_type = 1;
        }
        rasterizer_glass_draw_procedures.reflection(group, reflection_type);
    }
    if (has_tag(glass->diffuse_map) || has_tag(glass->diffuse_detail_map)) {
        rasterizer_glass_draw_procedures.diffuse(group);
    }
}

static void draw_meter_shader(transparent_geometry_group *group, const Shader *shader, int16_t vertex_type)
{
    const ShaderTransparentMeter *meter = shader_cast<ShaderTransparentMeter>(shader);
    int16_t variant = 0;
    float meter_brightness, flash_brightness, value, gradient;
    float flash[3];
    float scaled_gradient;
    float pixel_constants[6][4];
    float vertex_constants[3][4];
    void *effect;
    int i;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        return;
    }
    if (vertex_type == 0 || vertex_type == 2) {
        variant = 0;
    } else if (vertex_type == 4) {
        variant = 1;
    }
    if (rasterizer_effects[111].effect == 0) {
        return;
    }
    meter_brightness = 1.0f;
    flash_brightness = 1.0f;
    value = 1.0f;
    gradient = 1.0f;
    set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration);
    set_vertex_shader(rasterizer_vertex_shaders[57 + variant].shader);

    if (group->lighting_extra != 0) {
        const float *function_values = animation_function_values(group->lighting_extra);

        if (function_values != NULL) {
            int16_t source;

            source = meter->meter_brightness_source;
            if (source >= 1 && source <= 4) meter_brightness = function_values[source - 1];
            source = meter->flash_brightness_source;
            if (source >= 1 && source <= 4) flash_brightness = function_values[source - 1];
            source = meter->value_source;
            if (source >= 1 && source <= 4) value = function_values[source - 1];
            source = meter->gradient_source;
            if (source >= 1 && source <= 4) gradient = function_values[source - 1];
        }
    }
    if (console_debug_toggle_6893eb) {
        float animated = (float)halo::math::periodic_function_evaluate(2, rasterizer_time.time / console_debug_meter_period);

        meter_brightness = (console_debug_meter_values[0] < 0.0f) ? animated : console_debug_meter_values[0];
        flash_brightness = (console_debug_meter_values[1] < 0.0f) ? animated : console_debug_meter_values[1];
        value = (console_debug_meter_values[2] < 0.0f) ? animated : console_debug_meter_values[2];
        gradient = (console_debug_meter_values[3] < 0.0f) ? animated : console_debug_meter_values[3];
    }

    flash[0] = flash_brightness * meter->flash_color.red;
    flash[1] = flash_brightness * meter->flash_color.green;
    flash[2] = flash_brightness * meter->flash_color.blue;
    scaled_gradient = gradient * 8.0f;
    if (!(scaled_gradient > 1.0f)) {
        scaled_gradient = 1.0f;
    }

    pixel_constants[0][0] = flash[0];
    pixel_constants[0][1] = flash[1];
    pixel_constants[0][2] = flash[2];
    pixel_constants[0][3] = 1.0f;
    pixel_constants[1][0] = meter->gradient_max_color.red;
    pixel_constants[1][1] = meter->gradient_max_color.green;
    pixel_constants[1][2] = meter->gradient_max_color.blue;
    pixel_constants[1][3] = 1.0f / scaled_gradient;
    pixel_constants[2][0] = meter->gradient_min_color.red;
    pixel_constants[2][1] = meter->gradient_min_color.green;
    pixel_constants[2][2] = meter->gradient_min_color.blue;
    pixel_constants[2][3] = value;
    pixel_constants[3][0] = meter->background_color.red;
    pixel_constants[3][1] = meter->background_color.green;
    pixel_constants[3][2] = meter->background_color.blue;
    pixel_constants[3][3] = 1.0f;
    pixel_constants[4][0] = meter->meter_tint_color.red;
    pixel_constants[4][1] = meter->meter_tint_color.green;
    pixel_constants[4][2] = meter->meter_tint_color.blue;
    pixel_constants[4][3] = 1.0f;
    if (halo::test_flag(meter->meter_flags, halo::tags::shader_transparent_meter_tag_flag::flash_color_is_negative)) {
        pixel_constants[5][0] = -flash[0];
        pixel_constants[5][1] = -flash[1];
        pixel_constants[5][2] = -flash[2];
        pixel_constants[5][3] = -1.0f;
    } else {
        pixel_constants[5][0] = flash[0];
        pixel_constants[5][1] = flash[1];
        pixel_constants[5][2] = flash[2];
        pixel_constants[5][3] = 1.0f;
    }
    if (halo::test_flag(meter->meter_flags, halo::tags::shader_transparent_meter_tag_flag::tint_mode_2)) {
        pixel_constants[0][3] = meter->meter_transparency;
        pixel_constants[3][3] = meter->background_transparency;
        pixel_constants[4][0] *= meter_brightness;
        pixel_constants[4][1] *= meter_brightness;
        pixel_constants[4][2] *= meter_brightness;
        pixel_constants[4][3] = meter->meter_transparency;
    } else {
        pixel_constants[0][3] = meter_brightness;
        pixel_constants[3][3] = 0.0f;
        pixel_constants[4][3] = meter_brightness;
    }

    rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(meter->map.tag_id), 0, 0, 1,
                                          (int16_t)group->shader_permutation, &rasterizer_effects[111]);
    set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    const uint32_t meter_filter = halo::test_flag(meter->meter_flags, halo::tags::shader_transparent_meter_tag_flag::unfiltered) ? 1 : 2;
    set_sampler_state(0, halo::d3d9::ss::mag_filter, meter_filter);
    set_sampler_state(0, halo::d3d9::ss::min_filter, meter_filter);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, meter_filter);
    set_render_state(halo::d3d9::rs::cull_mode, halo::test_flag(meter->meter_flags, halo::tags::shader_transparent_meter_tag_flag::two_sided) ? 1 : 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);

    for (i = 0; i < 4; i++) {
        vertex_constants[0][i] = 1.0f;
    }
    vertex_constants[1][0] = group->base_map_u_scale;
    vertex_constants[1][1] = 0.0f;
    vertex_constants[1][2] = 0.0f;
    vertex_constants[1][3] = 0.0f;
    vertex_constants[2][0] = 0.0f;
    vertex_constants[2][1] = group->base_map_v_scale;
    vertex_constants[2][2] = 0.0f;
    vertex_constants[2][3] = 0.0f;
    render_device().set_vertex_shader_constant_f(10, &vertex_constants[0][0], 3);
    if (console_debug_toggle_6893eb && debug_print_enabled_flag != 0) {
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    }
    render_device().set_pixel_shader_constant_f(0, &pixel_constants[0][0], 6);
    effect = rasterizer_effects[111].effect;
    draw_effect_passes(effect, group);
}

/**
 * Direct3D 9 back end function rasterizer_transparent_geometry_group_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_transparent_geometry_group_draw.c.txt.
 *
 * @address 0x533850
 */
void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached)
{
    transparent_geometry_group *pool;
    transparent_geometry_group *cursor;
    Shader *shader;
    uint8_t draw_secondary_groups = 0;
    uint32_t immediate;
    int16_t vertex_type;
    int16_t pass;
    int32_t key;

    if (group->parent_sort_key != 0 && !attached) {
        return;
    }
    if (!transparent_geometry_group_test_drawn_bit(group)) {
        return;
    }
    pool = transparent_geometry_groups;
    if (group >= pool && group < pool + transparent_geometry_group_count) {
        int16_t index = (int16_t)(group - pool);

        if (index != -1) {
            transparent_geometry_group_drawn_bits[index >> 5] |= 1u << (index & 0x1f);
        }
    }
    if (group->previous_group_index != -1) {
        rasterizer_transparent_geometry_group_draw(&pool[group->previous_group_index], attached);
    }

    if (group->parameters.mode == 2) {
        key = group->sort_key;
        if (key != transparent_geometry_group_last_drawn_key && !attached) {
            set_depth_prepass_states(1, 0);
            set_vertex_declaration(rasterizer_vertex_declarations[4].declaration);
            set_vertex_shader(rasterizer_depth_prepass_vertex_shader);
            cursor = group;
            do {
                int16_t next;

                if (cursor->sort_key != key || cursor->parameters.mode != 2) {
                    break;
                }
                shader = cursor->shader;
                if (shader == NULL ||
                    !((shader_type_of(shader) == 5 || shader_type_of(shader) == 6 || shader_type_of(shader) == 7) &&
                      halo::test_flag(shader_cast<ShaderTransparentChicago>(shader)->shader_transparent_chicago_flags, halo::tags::shader_transparent_generic_tag_flag::ignore_effect))) {

                    set_group_skinning(group, cursor);
                    if (group->flags & _group_node_parts_bit) {
                        chimera__rasterizer_set_up_node_parts(cursor->node_part_count,
                                                              cursor->node_part_indices);
                    }
                    if (group->lighting != 0) {
                        rasterizer_prepare_lighting_constants(cursor->lighting);
                    }
                    rasterizer_transparent_geometry_group_draw_vertices(cursor, 0);
                }
                next = (int16_t)((int16_t)cursor->sorted_index + 1);
                if (next < transparent_geometry_group_count) {
                    cursor = &transparent_geometry_groups[transparent_geometry_group_sorted_indices[next]];
                } else {
                    cursor = NULL;
                }
            } while (cursor != NULL);
        }
    }

    if (rasterizer_transparent_decals_enabled()) {

        if ((group->flags & _group_immediate_bit) == 0 && group->parameters.mode == 1) {
            key = group->sort_key;
            shader = group->shader;
            if (key != transparent_geometry_group_last_drawn_key && shader != NULL &&
                shader_type_of(shader) == 4 && !attached) {
                set_depth_prepass_states(3, halo::d3d9::k_color_white);
                cursor = group;
                do {
                    if (cursor->sort_key != key || cursor->parameters.mode != 1) {
                        break;
                    }
                    if (cursor->shader != 0) {
                        rasterizer_geometry_part_draw(cursor);
                    }
                    cursor = transparent_geometry_group_get_next_sorted(cursor);
                } while (cursor != NULL);
                set_render_state(halo::d3d9::rs::color_write_enable, 7);
            }
        }
    }

    if ((group->flags & _group_immediate_bit) == 0 && rasterizer_window.type == 1 && !attached) {
        shader = group->shader;
        if (console_debug_toggle_689422) {
            if (shader != NULL && shader_type_of(shader) == 4 && group->parameters.mode == 1 &&
                group->sort_key != transparent_geometry_group_last_drawn_key) {
                rasterizer_render_target_capture_frame();
            }
        } else if (shader == NULL || (shader_type_of(shader) != 8 && !halo::shaders::shader_view(shader).draw_before_water())) {
            rasterizer_render_target_capture_frame();
        }
    }

    immediate = group->flags & _group_immediate_bit;
    if (immediate == 0 && rasterizer_window.type == 1 && group->parameters.mode == 1) {
        shader = group->shader;
        if (shader != NULL && shader_type_of(shader) == 4 && !attached) {
            transparent_geometry_group *next = transparent_geometry_group_get_next_sorted(group);

            if (next == NULL || next->parameters.mode != 1 || next->sort_key != group->sort_key ||
                next->shader == 0 || shader_type_of(next->shader) != 4) {
                draw_secondary_groups = 1;
            }
        }
    }

    if (group->shader == 0) {
        group->callback(group->first_index, group->primitive_count);
    } else {
        vertex_type = -1;
        if (group->vertex_buffer != 0) {
            vertex_type = group->vertex_buffer->type;
        } else if (group->dynamic_vertex_slot != -1) {
            vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
        }
        if (immediate == 0) {
            set_group_skinning(group, group);
            if (group->flags & _group_node_parts_bit) {
                chimera__rasterizer_set_up_node_parts(group->node_part_count,
                                                      group->node_part_indices);
            }
            if (group->lighting != 0) {
                rasterizer_prepare_lighting_constants(group->lighting);
            }
        }
        if (group->flags & _group_flag_8_bit) {
            if (rasterizer_window.type == 1) {
                chimera__rasterizer_set_frustum_z_func(k_decal_frustum_z_near, k_decal_frustum_z_far);
            }
            set_render_state(halo::d3d9::rs::z_enable, 0);
        } else {
            set_render_state(halo::d3d9::rs::z_enable, 1);
            set_render_state(halo::d3d9::rs::z_write_enable, 0);
            set_render_state(halo::d3d9::rs::z_func, 4);
            if (halo::shaders::shader_view(group->shader).is_decal()) {
                chimera__transparent_decal_zbias();
            } else {
                rasterizer_clear_decal_zbias();
            }
        }

        for (pass = 0; pass < 2; pass++) {
            if ((group->flags & _group_sort_first_bit) != 0) {

                if (group->parameters.mode == 1) {
                    if (pass > 0) {
                        break;
                    }
                    chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
                } else if (pass == 0) {
                    shader = group->shader;
                    if (shader != NULL && shader_type_of(shader) == 1 && halo::test_flag(shader_cast<shader_effect>(shader)->flags, halo::tags::particle_shader_tag_flag::don_t_overdraw_fp_weapon)) {
                        continue;
                    }
                    rasterizer_set_shader_stage_config(3);
                } else {
                    rasterizer_set_shader_stage_config(2);
                    set_render_state(halo::d3d9::rs::z_enable, 0);
                }
            } else if (pass > 0) {
                break;
            }

            shader = group->shader;
            switch (shader_type_of(shader)) {
            case 1:
                draw_particle_effect_shader(group, shader);
                break;
            case 4:
                if (group->parameters.mode == 1) {
                    if (rasterizer_secondary_groups_drawn) {
                        return;
                    }
                    rasterizer_transparent_geometry_group_draw_active_camouflage(group);
                }
                break;
            case 6:
                rasterizer_shader_transparent_chicago_draw(group, attached);
                break;
            case 7:
                rasterizer_shader_transparent_chicago_extended_draw(group, attached);
                break;
            case 8:
                rasterizer_water_draw_procedure(group);
                break;
            case 9:
                draw_glass_shader(group, shader);
                break;
            case 10:
                draw_meter_shader(group, shader, vertex_type);
                break;
            case 11:
                rasterizer_shader_transparent_plasma_draw(group);
                break;
            default:
                break;
            }
        }

        if ((group->flags & _group_flag_8_bit) && rasterizer_window.type == 1) {
            chimera__rasterizer_set_frustum_z_func(0.0f, 0.0f);
        }
        if ((group->flags & _group_sort_first_bit) != 0 && group->parameters.mode == 1) {
            chimera__rasterizer_set_frustum_z_func(0.0f, 0.0f);
        }
        rasterizer_clear_decal_zbias();
    }

    if (!attached) {
        transparent_geometry_group_last_drawn_key = group->sort_key;
    }
    if (group->next_group_index != -1) {
        rasterizer_transparent_geometry_group_draw(&transparent_geometry_groups[group->next_group_index], attached);
    }
    if (draw_secondary_groups && (int16_t)transparent_geometry_group_secondary_count > 0) {
        uint16_t remaining = (uint16_t)transparent_geometry_group_secondary_count;
        transparent_geometry_group *secondary = transparent_geometry_groups_secondary;

        do {
            if (secondary->parent_sort_key == group->sort_key && secondary->parameters.mode == 1) {
                rasterizer_transparent_geometry_group_draw(secondary, 1);
                if (debug_print_enabled_flag != 0) {
                    rasterizer_secondary_groups_drawn = 1;
                }
            }
            secondary++;
        } while (--remaining != 0);
    }
}

namespace rasterizer_transparent_geometry_group_draw_active_camouflage_impl {










static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void rasterizer_set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

static float real_lerp(float from, float to, float t)
{
    return (1.0f - t) * from + to * t;
}

/**
 * Direct3D 9 back end function rasterizer_transparent_geometry_group_draw_active_camouflage. The original
 * author notes are in
 * docs/original/rasterizer/rasterizer_transparent_geometry_group_draw_active_camouflage.c.txt.
 *
 * @address 0x519f70
 */
void rasterizer_transparent_geometry_group_draw_active_camouflage(transparent_geometry_group *group)
{
    const Shader *shader;
    float amount;

    if (halo::rasterizer::fields::active_camouflage_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }
    amount = group->parameters.blend_factor;
    shader = group->shader;
    if ((group->flags & _group_sort_first_bit) != 0) {
        chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
    }

    if (rasterizer_caps_flag_688 == 0 && rasterizer_caps_flag_68a == 0 &&
        rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_1) {
        void *effect = rasterizer_effects[105].effect;

        if (effect != 0) {
            const GlobalsRasterizerData *data = rasterizer_globals_data;
            float t = group->parameters.distortion_factor;
            float half_height;
            float constants[12];
            uint32_t passes;
            int16_t vertex_type;

            vertex_type = (int16_t)transparent_geometry_group_get_vertex_type_reference(group);
            render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration);

            rasterizer_set_render_state(halo::d3d9::rs::cull_mode, (~(uint32_t)shader_cast<ShaderModel>(shader)->shader_model_flags & 2) | 1);
            rasterizer_set_render_state(halo::d3d9::rs::color_write_enable, 7);
            rasterizer_set_render_state(halo::d3d9::rs::z_enable, 1);
            rasterizer_set_render_state(halo::d3d9::rs::z_write_enable, 1);
            rasterizer_set_render_state(halo::d3d9::rs::z_func, 4);
            rasterizer_set_render_state(halo::d3d9::rs::fog_enable, 0);
            rasterizer_set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
            chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(data->active_camouflage_distortion.tag_id), 0, 0);
            rasterizer_set_sampler_state(0, halo::d3d9::ss::address_u, 3);
            rasterizer_set_sampler_state(0, halo::d3d9::ss::address_v, 3);
            rasterizer_set_sampler_state(0, halo::d3d9::ss::address_w, 3);
            rasterizer_set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
            rasterizer_set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
            rasterizer_set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
            render_device().set_texture(2, rasterizer_render_targets[2].texture);
            rasterizer_set_sampler_state(2, halo::d3d9::ss::address_u, 3);
            rasterizer_set_sampler_state(2, halo::d3d9::ss::address_v, 3);
            rasterizer_set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
            rasterizer_set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
            rasterizer_set_sampler_state(2, halo::d3d9::ss::mip_filter, 1);
            render_device().set_vertex_shader(rasterizer_vertex_shaders[30].shader);

            half_height = (float)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top) * 0.5f;

            constants[0] = (1.0f / real_lerp(data->refraction_amount, data->hyper_stealth_refraction, t)) * amount;
            constants[1] = real_lerp(data->distance_falloff, data->hyper_stealth_distance_falloff, t);
            if (rasterizer_widescreen_camouflage_scale != 0) {
                constants[2] = (float)(rasterizer_window.camera.viewport_bounds.right -
                                       rasterizer_window.camera.viewport_bounds.left) * 0.5f;
                constants[3] = half_height;
            } else {
                constants[2] = 1.0f;
                constants[3] = 1.0f;
            }

            constants[4] = 0.0f;
            constants[5] = 0.0f;
            constants[6] = 0.0f;
            constants[7] = 0.0f;

            constants[8] = real_lerp(data->tint_color.red, data->hyper_stealth_tint_color.red, t);
            constants[9] = real_lerp(data->tint_color.green, data->hyper_stealth_tint_color.green, t);
            constants[10] = real_lerp(data->tint_color.blue, data->hyper_stealth_tint_color.blue, t);
            constants[11] = 0.0f;
            render_device().set_vertex_shader_constant_f(10, constants, 3);

            constants[0] = rasterizer_window.frustum.view_to_world.forward.i;
            constants[1] = rasterizer_window.frustum.view_to_world.forward.j;
            constants[2] = rasterizer_window.frustum.view_to_world.forward.k;
            constants[3] = 1.0f;
            constants[4] = rasterizer_window.frustum.view_to_world.left.i;
            constants[5] = rasterizer_window.frustum.view_to_world.left.j;
            constants[6] = rasterizer_window.frustum.view_to_world.left.k;
            constants[7] = 3.0f;
            render_device().set_vertex_shader_constant_f(0x1b, constants, 2);

            constants[0] = rasterizer_window.camera.position.x;
            constants[1] = rasterizer_window.camera.position.y;
            constants[2] = rasterizer_window.camera.position.z;
            constants[3] = 2.0f;
            constants[4] = rasterizer_window.camera.forward.i;
            constants[5] = rasterizer_window.camera.forward.j;
            constants[6] = rasterizer_window.camera.forward.k;
            constants[7] = 0.5f;
            render_device().set_vertex_shader_constant_f(4, constants, 2);

            constants[0] = amount;
            constants[1] = amount;
            constants[2] = amount;
            constants[3] = amount;
            render_device().set_pixel_shader_constant_f(0, constants, 1);

            render_device().effect_begin(effect, &passes, 3);
            render_device().effect_pass(effect, data->flags & 1);
            rasterizer_transparent_geometry_group_draw_vertices(group, 0);
            render_device().effect_end(effect);
        }
        rasterizer_set_render_state(halo::d3d9::rs::z_func, 3);
    } else {
        if (amount > 0.9f) {
            amount = 0.9f;
        }
        rasterizer_set_render_state(halo::d3d9::rs::z_func, 3);
    }

    if (amount < 1.0f) {
        rasterizer_model_draw_context context;

        context.flags = group->flags & (_group_sort_first_bit | _group_node_parts_bit);
        context.node_matrices = group->node_matrices;
        context.node_count = group->node_count;
        if (group->lighting != 0) {
            context.lighting = *group->lighting;
        } else {
            std::memset(&context.lighting, 0, sizeof(context.lighting));
        }
        if (group->lighting_extra != 0) {
            context.change_colors = group->lighting_extra->change_colors;
            context.function_values = group->lighting_extra->function_values;
        } else {
            context.change_colors = 0;
            context.function_values = 0;
        }
        {
            std::memset(&context.group_parameters, 0, sizeof(context.group_parameters));
        }
        context.center = group->position;
        context.base_map_u_scale = group->base_map_u_scale;
        context.base_map_v_scale = group->base_map_v_scale;

        rasterizer_set_render_state(halo::d3d9::rs::z_write_enable, 0);
        rasterizer_camouflage_fade_active = 1;
        rasterizer_camouflage_fade = 1.0f - amount;
        if (halo::rasterizer::fields::models_enabled != 0) {
            rasterizer_render_states_dirty = 1;
            halo::rasterizer::fields::sky_pass_active = 0;
            if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
                rasterizer_set_render_state(halo::d3d9::rs::lighting, 1);
            }
        }
        rasterizer_model_draw_prepare_states(&context, 1);
        rasterizer_shader_environment_draw_dispatch(group->dynamic_vertex_slot, group->shader, (int16_t)group->shader_permutation, group->index_buffer, group->dynamic_index_slot, group->primitive_count, group->vertex_buffer);
        rasterizer_model_draw_restore_states();
        halo::render::render_lighting_disable_workaround();
        rasterizer_camouflage_fade_active = 0;
    }

    if ((group->flags & _group_sort_first_bit) != 0) {
        chimera__rasterizer_set_frustum_z_func(0.0f, 0.0f);
    }
}

}  // namespace rasterizer_transparent_geometry_group_draw_active_camouflage_impl

/**
 * Direct3D 9 back end function rasterizer_transparent_geometry_group_draw_vertices. The original author notes
 * are in docs/original/rasterizer/rasterizer_transparent_geometry_group_draw_vertices.c.txt.
 *
 * @address 0x533660
 */
void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag)
{
    rasterizer_index_buffer *index_buffer = group->index_buffer;
    rasterizer_vertex_buffer *vertex_buffer = group->vertex_buffer;

    if (index_buffer != NULL) {
        if (vertex_buffer != NULL) {
            rasterizer_dynamic_geometry_chain_draw(group->primitive_count, vertex_buffer, index_buffer);
        } else {
            rasterizer_dynamic_vertex_draw_indexed(index_buffer, group->primitive_count, group->dynamic_vertex_slot);
        }
        return;
    }
    if (vertex_buffer != NULL) {
        if (flag) {
            chimera__rasterizer_draw_dynamic_triangles_static_vertices2(group->primitive_count, vertex_buffer,
                                                                        group->dynamic_index_slot, group->first_index,
                                                                        vertex_buffer + 1);
        } else {
            chimera__rasterizer_draw_dynamic_triangles_static_vertices(group->primitive_count, vertex_buffer,
                                                                       group->dynamic_index_slot, group->first_index);
        }
        return;
    }
    if (group->dynamic_index_slot >= 0) {
        rasterizer_dynamic_index_cache_draw(group->dynamic_index_slot, group->first_index, group->primitive_count,
                                            group->dynamic_vertex_slot);
    } else {
        int16_t kind = (int16_t)-(int16_t)group->dynamic_index_slot;
        int16_t count;

        if (kind == 3 || kind == 4) {
            count = (int16_t)(group->primitive_count / (kind - 2));
        } else {
            count = 1;
        }
        rasterizer_dynamic_vertex_draw(0, count, group->dynamic_vertex_slot, kind);
    }
}

/**
 * Allocates (or, for `flags` bit 1 already set, reuses the single static "immediate" record) a
 * transparent_geometry_group, fills every field from the caller's arguments and the current camera
 * position/forward axis, and either draws it right away (immediate groups, or ScenarioStructureBSPMaterial-
 * style shaders with derived-shader flag bit 3 set) or leaves it queued for the frame's depth sort.
 *
 * Registers: params as declared; EAX = world_position
 *
 * @address 0x522300
 */
void rasterizer_transparent_geometry_group_new(Shader *shader, int16_t shader_permutation, BitmapData *lightmap_bitmap, uint32_t dynamic_index_slot, uint32_t first_index, uint32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, ColorARGB *tint, render_lighting *lighting, uint32_t flags, real_point3d *world_position)
{
    transparent_geometry_group *group;
    float dx, dy, dz;
    ColorARGB zero_tint;

    if (console_debug_toggle_6893fb == 0) {
        return;
    }

    dx = world_position->x - rasterizer_window.camera.position.x;
    dy = world_position->y - rasterizer_window.camera.position.y;
    dz = world_position->z - rasterizer_window.camera.position.z;

    if (tint != 0) {
        flags = flags | _group_flag_0_bit;
    }
    if (halo::shaders::shader_view(shader).is_decal() != 0) {
        flags = flags | _group_flag_0_bit | _group_immediate_bit | _group_flag_4_bit;
    }

    if ((flags & _group_immediate_bit) == 0) {
        group = 0;
        if (transparent_geometry_group_count < k_rasterizer_maximum_transparent_groups) {
            group = &transparent_geometry_groups[transparent_geometry_group_count];
            group->sorted_index = transparent_geometry_group_count;
            transparent_geometry_group_count = transparent_geometry_group_count + 1;
        }
        if (group == 0) {
            if (transparent_geometry_group_overflow_b == 0) {
                transparent_geometry_group_overflow_b = 1;
            }
            return;
        }
    } else {
        group = &transparent_geometry_group_immediate;
        group->sorted_index = -1;
    }

    group->shader_permutation = shader_permutation;
    group->dynamic_index_slot = dynamic_index_slot;
    group->first_index = first_index;
    group->flags = flags;
    group->primitive_count = primitive_count;
    group->vertex_buffer = vertex_buffer;
    group->object_index = 0;
    group->sort_key = 0;
    group->shader = shader;
    group->index_buffer = NULL;
    group->parameters.mode = 0;
    group->dynamic_vertex_slot = -1;
    group->lightmap_bitmap = lightmap_bitmap;

    group->depth = -(rasterizer_window.camera.forward.i * dx + rasterizer_window.camera.forward.j * dy +
                      rasterizer_window.camera.forward.k * dz);
    group->position = *world_position;

    if (tint == 0) {
        zero_tint.alpha = 0.0f;
        zero_tint.red = 0.0f;
        zero_tint.green = 0.0f;
        zero_tint.blue = 0.0f;
        tint = &zero_tint;
    }
    group->tint = *tint;

    group->base_map_u_scale = 1.0f;
    group->base_map_v_scale = 1.0f;
    group->previous_group_index = -1;
    group->next_group_index = -1;
    group->parent_sort_key = 0;
    group->first_person = 0;
    group->node_matrices = 0;
    group->node_count = 0;

    group->lighting = static_cast<render_lighting *>(chimera__rasterizer_memory_alloc(lighting, 0x74));
    group->lighting_extra = 0;

    if (shader->shader_type == 8) {
        halo::rasterizer::fields::transparent_group_created = 1;
    }
    if (shader->shader_type == 8 && halo::test_flag(shader_cast<ShaderTransparentWater>(shader)->water_flags, halo::tags::shader_transparent_water_tag_flag::draw_before_fog)) {
        group->flags = group->flags | _group_immediate_bit;
        rasterizer_transparent_geometry_group_draw(group, 0);
        transparent_geometry_group_set_drawn_bit(group, 1);
        group->flags = group->flags & ~static_cast<uint32_t>(_group_immediate_bit);
        return;
    }
    if ((flags & _group_immediate_bit) != 0) {
        rasterizer_transparent_geometry_group_draw(group, 0);
    }
}

/**
 * Gated by the debug toggle at 0x00689400 (default 1, read nowhere else). Appends a new transparent_geometry_group built from a
 * dynamic vertex/index cache submission (as opposed to rasterizer_transparent_geometry_group_new's static-tag
 * submission), computing its depth sort key from the camera position/forward axis and bumping it by 0.25 for
 * shaders whose type is 1 with flag bit 0 set. Sets the sticky overflow flag once if the pool is full.
 *
 * Registers: stack params as declared; EDX = world_position, EDI = shader
 *
 * @address 0x51c830
 */
void rasterizer_transparent_object_append(BitmapData *lightmap_bitmap, int32_t dynamic_index_slot, int32_t dynamic_vertex_slot, int32_t primitive_count, uint32_t flags, real_point3d *world_position, Shader *shader)
{
    transparent_geometry_group *group;
    float dx, dy, dz;

    if (halo::rasterizer::fields::transparent_object_append_enabled == 0) {
        return;
    }

    dx = world_position->x - rasterizer_window.camera.position.x;
    dy = world_position->y - rasterizer_window.camera.position.y;
    dz = world_position->z - rasterizer_window.camera.position.z;

    if (transparent_geometry_group_count < k_rasterizer_maximum_transparent_groups) {
        transparent_geometry_groups[transparent_geometry_group_count].sorted_index =
            transparent_geometry_group_count;
        group = &transparent_geometry_groups[transparent_geometry_group_count];
        transparent_geometry_group_count = transparent_geometry_group_count + 1;

        group->flags = flags;
        group->dynamic_index_slot = dynamic_index_slot;
        group->primitive_count = primitive_count;
        group->dynamic_vertex_slot = dynamic_vertex_slot;
        group->lightmap_bitmap = lightmap_bitmap;
        group->object_index = 0;
        group->sort_key = 0;
        group->shader = shader;
        group->shader_permutation = 0;
        group->parameters.mode = 0;
        group->index_buffer = NULL;
        group->first_index = 0;
        group->vertex_buffer = NULL;
        group->depth = -(rasterizer_window.camera.forward.i * dx + rasterizer_window.camera.forward.j * dy +
                          rasterizer_window.camera.forward.k * dz);
        group->position = *world_position;
        group->tint.alpha = 0.0f;
        group->tint.red = 0.0f;
        group->tint.green = 0.0f;
        group->tint.blue = 0.0f;
        group->base_map_v_scale = 1.0f;
        group->base_map_u_scale = 1.0f;
        group->previous_group_index = -1;
        group->next_group_index = -1;
        group->parent_sort_key = 0;
        group->first_person = 0;

        if (shader->shader_type == 1 && halo::test_flag(shader_cast<shader_effect>(shader)->flags, halo::tags::particle_shader_tag_flag::sort_bias)) {
            group->depth = group->depth + 0.25f;
        }

        group->node_matrices = 0;
        group->node_count = 0;
        group->lighting = 0;
        group->lighting_extra = 0;
    } else if (transparent_geometry_group_overflow_a == 0) {
        transparent_geometry_group_overflow_a = 1;
    }
}

/**
 * Allocates the next free slot from the primary pool (max 384 entries), or NULL when full.
 *
 * @address 0x515230
 */
transparent_geometry_group * transparent_geometry_group_allocate(void)
{
    transparent_geometry_group *group;

    if (transparent_geometry_group_count >= k_rasterizer_maximum_transparent_groups) {
        return (transparent_geometry_group *)0;
    }
    group = &transparent_geometry_groups[transparent_geometry_group_count];
    group->sorted_index = transparent_geometry_group_count;
    transparent_geometry_group_count = transparent_geometry_group_count + 1;
    return group;
}

/**
 * Allocates the next free slot from the secondary pool (max 32 entries), or NULL when full.
 *
 * @address 0x515260
 */
transparent_geometry_group * transparent_geometry_group_allocate_secondary(void)
{
    transparent_geometry_group *group;

    if (transparent_geometry_group_secondary_count >= k_rasterizer_maximum_secondary_groups) {
        return (transparent_geometry_group *)0;
    }
    group = &transparent_geometry_groups_secondary[transparent_geometry_group_secondary_count];
    group->sorted_index = transparent_geometry_group_secondary_count;
    transparent_geometry_group_secondary_count = transparent_geometry_group_secondary_count + 1;
    return group;
}

static int32_t is_batched_shader(Shader *shader)
{
    if (shader == (Shader *)0) {
        return 0;
    }
    return (shader->shader_type == 5 || shader->shader_type == 6 || shader->shader_type == 7) &&
           halo::test_flag(shader_cast<ShaderTransparentChicago>(shader)->shader_transparent_chicago_flags, halo::tags::shader_transparent_generic_tag_flag::draw_before_water);
}

/**
 * __cdecl(short *a, short *b) qsort comparator: batched-shader groups (5/6/7 with the derived flag) always
 * sort before b / after a; a type-8 shader on either side also sorts first; otherwise back-to-front by depth,
 * then ascending sort_key, honoring _group_sort_first_bit; first_person groups always sort last (checked once
 * at the very end, matching the original's single shared exit).
 *
 * @address 0x5155b0
 */
int transparent_geometry_group_compare(int16_t *a, int16_t *b)
{
    transparent_geometry_group *ga = &transparent_geometry_groups[*a];
    transparent_geometry_group *gb = &transparent_geometry_groups[*b];
    Shader *shader_a = (Shader *)ga->shader;
    Shader *shader_b = (Shader *)gb->shader;
    int result;

    auto order = [&]() -> int {
        if (is_batched_shader(shader_a)) {
            return -1;
        }
        if (is_batched_shader(shader_b)) {
            return 1;
        }

        if (shader_a == (Shader *)0 || shader_a->shader_type != 8) {
            if (shader_b != (Shader *)0 && shader_b->shader_type == 8) {
                return 1;
            }
            if ((ga->flags & _group_sort_first_bit) == 0) {
                if ((gb->flags & _group_sort_first_bit) != 0) {
                    return -1;
                }
            } else {
                if ((gb->flags & _group_sort_first_bit) == 0) {
                    return 1;
                }
            }
            if (gb->depth < ga->depth) {
                return 1;
            }
            if (gb->depth <= ga->depth) {
                if (gb->sort_key < ga->sort_key) {
                    return 1;
                }
                if (gb->sort_key <= ga->sort_key) {
                    return 0;
                }
            }
        }
        return -1;
    };
    result = order();

    if (ga->first_person == 0) {
        if (gb->first_person == 0) {
            return result;
        }
    } else if (gb->first_person == 0) {
        return 1;
    }
    if (ga->first_person != 0) {
        return result;
    }
    return -1;
}

/**
 * Draws every active transparent-geometry group in depth-sorted order starting from the shared draw cursor,
 * re-sorting first (and resetting the cursor) when `resort` is set.
 *
 * Registers: stack -> resort
 *
 * @address 0x5154a0
 */
void transparent_geometry_group_draw_all(uint8_t resort)
{
    uint8_t applied_frustum_z = 0;
    int32_t cursor;

    if (transparent_geometry_group_count <= 0) {
        return;
    }

    if (resort != 0) {
        transparent_geometry_group_sort();
        transparent_geometry_group_draw_cursor = 0;
    }

    cursor = transparent_geometry_group_draw_cursor;
    transparent_geometry_group_last_drawn_key = 0;
    rasterizer_secondary_groups_drawn = 0;

    if (cursor >= transparent_geometry_group_count) {
        return;
    }

    do {
        transparent_geometry_group *group =
            &transparent_geometry_groups[transparent_geometry_group_sorted_indices[cursor]];

        if (resort != 0) {
            Shader *shader = group->shader;
            if (shader == (Shader *)0 ||
                (shader->shader_type != 8 &&
                 ((shader->shader_type != 5 && shader->shader_type != 6 && shader->shader_type != 7) ||
                  !halo::test_flag(shader_cast<ShaderTransparentChicago>(shader)->shader_transparent_chicago_flags, halo::tags::shader_transparent_generic_tag_flag::draw_before_water)))) {
                break;
            }
        }

        if ((group->flags & _group_sort_first_bit) != 0 && applied_frustum_z == 0) {
            rasterizer_set_shader_stage_config(0);
            chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
            applied_frustum_z = 1;
        }

        rasterizer_transparent_geometry_group_draw(group, 0);
        transparent_geometry_group_draw_cursor = transparent_geometry_group_draw_cursor + 1;
        cursor = transparent_geometry_group_draw_cursor;
    } while (cursor < transparent_geometry_group_count);

    if (applied_frustum_z != 0) {
        chimera__rasterizer_set_frustum_z_func(0.0f, 0.0f);
    }
}

/**
 * Returns the transparent-geometry group that follows `group` in the current depth-sorted order, or NULL if
 * `group` is NULL or already last.
 *
 * Registers: EAX -> group
 *
 * @address 0x515290
 */
transparent_geometry_group * transparent_geometry_group_get_next_sorted(transparent_geometry_group *group)
{
    int32_t next_position;

    if (group == (transparent_geometry_group *)0) {
        return (transparent_geometry_group *)0;
    }
    next_position = (int16_t)(group->sorted_index + 1);
    if (next_position >= transparent_geometry_group_count) {
        return (transparent_geometry_group *)0;
    }
    return &transparent_geometry_groups[transparent_geometry_group_sorted_indices[next_position]];
}

/**
 * Returns a packed 0xffff0000 | vertex_type reference for `group`'s vertex source (its static vertex_buffer
 * when set, else its dynamic_vertex_slot's type), or 0xffffffff if neither is set.
 *
 * Registers: EDX -> group
 *
 * @address 0x515400
 */
uint32_t transparent_geometry_group_get_vertex_type_reference(transparent_geometry_group *group)
{
    rasterizer_vertex_buffer *vb = group->vertex_buffer;

    if (vb != (rasterizer_vertex_buffer *)0) {
        return k_vertex_type_reference_flag | (uint16_t)vb->type;
    }
    if (group->dynamic_vertex_slot != -1) {
        return k_vertex_type_reference_flag | (uint16_t)rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    return halo::k_dword_none;
}

/**
 * Converts a pointer into the primary transparent-geometry-group pool back into its slot index, or -1 if it is
 * out of range.
 *
 * Registers: ECX -> group
 *
 * @address 0x5152d0
 */
int32_t transparent_geometry_group_index_from_pointer(transparent_geometry_group *group)
{
    transparent_geometry_group *base = transparent_geometry_groups;

    if (base <= group && group < base + transparent_geometry_group_count) {
        return (int32_t)(group - base);
    }
    return -1;
}

/**
 * Sets (clear == 0) or clears (clear != 0) `group`'s drawn bit; a no-op if `group` is out of range.
 *
 * Registers: EAX -> group, stack -> clear
 *
 * @address 0x515370
 */
void transparent_geometry_group_set_drawn_bit(transparent_geometry_group *group, uint8_t clear)
{
    transparent_geometry_group *base = transparent_geometry_groups;
    int32_t index;

    if (!(base <= group && group < base + transparent_geometry_group_count)) {
        return;
    }
    index = (int32_t)(group - base);
    if (index == -1) {
        return;
    }
    if (clear == 0) {
        transparent_geometry_group_drawn_bits[index >> 5] |= (1u << (index & 0x1f));
    } else {
        transparent_geometry_group_drawn_bits[index >> 5] &= ~(1u << (index & 0x1f));
    }
}

/**
 * Re-sorts the active transparent-geometry groups by transparent_geometry_group_compare and records each
 * group's new sorted index.
 *
 * @address 0x5156d0
 */
void transparent_geometry_group_sort(void)
{
    int32_t i;

    for (i = 0; i < transparent_geometry_group_count; i++) {
        transparent_geometry_group_sorted_indices[i] = (int16_t)i;
    }

    qsort(transparent_geometry_group_sorted_indices, (size_t)transparent_geometry_group_count,
          sizeof(int16_t), (int (*)(const void *, const void *))transparent_geometry_group_compare);

    for (i = 0; i < transparent_geometry_group_count; i++) {
        transparent_geometry_groups[transparent_geometry_group_sorted_indices[i]].sorted_index = i;
    }
}

/**
 * Returns 1 when `group`'s drawn bit is clear (out of range counts as clear), 0 when it is set.
 *
 * Registers: EAX -> group
 *
 * @address 0x515310
 */
uint8_t transparent_geometry_group_test_drawn_bit(transparent_geometry_group *group)
{
    transparent_geometry_group *base = transparent_geometry_groups;
    int32_t index = -1;

    if (base <= group && group < base + transparent_geometry_group_count) {
        index = (int32_t)(group - base);
    }
    if (index == -1) {
        return 1;
    }
    return (uint8_t)(1 - ((transparent_geometry_group_drawn_bits[index >> 5] & (1u << (index & 0x1f))) != 0));
}

/**
 * Takes no arguments. Allocates the primary (384 entry) and secondary (32
 * entry) transparent geometry group pools and the primary pool's sorted-index buffer, then performs further
 * subsystem init via rasterizer_misc_vertex_buffer_create. Returns a nonzero low byte on success.
 *
 * @address 0x5151c0
 */
int32_t transparent_geometry_pool_initialize(void)
{
    transparent_geometry_group *secondary_pool;

    transparent_geometry_groups = static_cast<transparent_geometry_group *>(GlobalAlloc(0, k_rasterizer_maximum_transparent_groups * sizeof(transparent_geometry_group)));
    transparent_geometry_group_sorted_indices = static_cast<int16_t *>(GlobalAlloc(0, k_rasterizer_maximum_transparent_groups * sizeof(int16_t)));
    secondary_pool = static_cast<transparent_geometry_group *>(GlobalAlloc(0, k_rasterizer_maximum_secondary_groups * sizeof(transparent_geometry_group)));
    transparent_geometry_group_secondary_count = 0;
    transparent_geometry_group_count = 0;
    transparent_geometry_groups_secondary = secondary_pool;

    if (transparent_geometry_groups != (transparent_geometry_group *)0 &&
        transparent_geometry_group_sorted_indices != nullptr && secondary_pool != nullptr) {
        uint32_t result = rasterizer_misc_vertex_buffer_create();
        if ((uint8_t)result != 0) {
            return (int32_t)halo::rasterizer::replace_low_byte(result, 1);
        }
        return (int32_t)halo::rasterizer::replace_low_byte(result, 0);
    }
    return (int32_t)halo::rasterizer::replace_low_byte(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(secondary_pool)), 0);
}

}  // namespace halo::rasterizer
