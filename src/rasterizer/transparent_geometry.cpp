/**
 * @file src/rasterizer/transparent_geometry.cpp
 * The transparent geometry group queue: pools, depth sort and group drawing.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"

extern "C" {

extern uint8_t shader_is_decal(const Shader *shader);
extern uint8_t shader_draw_before_water(const void *shader);
extern void shader_texture_animation_evaluate(const void *function_source, const void *animation, float *out_u, float *out_v, float u_scale, float v_scale, float unused_z, float unused_w, float unused_5, float time);
extern real periodic_function_evaluate(periodic_function_t type, double time);
extern void render_lighting_disable_workaround(void);

}  // extern "C"

namespace halo::rasterizer {

/**
 * Returns whether the current mode and driver capabilities allow transparent-decal/shader-based geometry-group
 * rendering to proceed.
 *
 * @address 0x519ac0
 */
int rasterizer_transparent_decals_enabled(void)
{
    if (rasterizer_window.type != 1 || console_debug_toggle_689421 == 0 ||
        (rasterizer_caps_flag_688 == 0 &&
         (rasterizer_caps_flag_68a == 0 && rasterizer_caps.pixel_shader_version > 0xffff0100))) {
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
transparent_geometry_group * rasterizer_transparent_geometry_group_build(transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot, const real_point3d *position)
{
    rasterizer_model_draw_context *context;
    transparent_geometry_group *allocated = NULL;
    transparent_geometry_group *group;
    uint8_t skip;
    uint8_t test_immediate;
    uint32_t flags;

    if (!halo::rasterizer::globals::models_enabled || !console_debug_toggle_6893ed) {
        return NULL;
    }
    skip = (shader != NULL && *(int16_t *)&((struct Shader *)shader)->shader_type == 4 && (shader[0x28] & 8) != 0);
    if (rasterizer_active_model_mode != 1) {
        test_immediate = 1;
    } else {
        test_immediate = (shader != NULL && *(int16_t *)&((struct Shader *)shader)->shader_type == 4 && *(int16_t *)(shader + 0x28) != 0);
    }
    if (skip) {
        if (link != NULL) {
            link->group_index = -1;
            link->previous_group_index = 0;
            link->next_group_index = 0;
        }
        return NULL;
    }

    context = rasterizer_active_model_context;
    flags = context->flags;
    if (test_immediate) {
        if (shader_is_decal((const Shader *)shader)) {
            flags |= 3;
        }
        if (flags & 2) {
            group = &transparent_geometry_group_environment_immediate;
            group->sorted_index = -1;
            goto fill;
        }
    }
    if (rasterizer_active_model_mode == 1 && shader != NULL && *(int16_t *)&((struct Shader *)shader)->shader_type != 4) {
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

fill:
    group->flags = flags;
    group->object_index = context->object_index;
    if (flags & 0x100) {
        group->node_part_indices = (uint32_t)(uintptr_t)rasterizer_node_part_indices;
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
    group->shader = (uint32_t)(uintptr_t)shader;
    group->parameters = context->group_parameters;
    group->index_buffer = (uint32_t)(uintptr_t)index_buffer;
    group->primitive_count = primitive_count;
    group->dynamic_index_slot = dynamic_index_slot;
    group->dynamic_vertex_slot = dynamic_vertex_slot;
    group->vertex_buffer = (uint32_t)(uintptr_t)vertex_buffer;
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
    if (rasterizer_active_model_mode == 1 && *(int16_t *)&((struct Shader *)shader)->shader_type != 4) {
        group->parent_sort_key = context->group_parameters.sort_key;
    } else {
        group->parent_sort_key = 0;
    }
    group->first_person = model_render_first_person;

    if (flags & 2) {

        group->node_matrices = context->node_matrices;
        group->node_count = context->node_count;
        group->lighting = (uint32_t)(uintptr_t)&context->lighting;
        group->lighting_extra = (uint32_t)(uintptr_t)&context->change_colors;
        transparent_geometry_group_last_drawn_key = 0;
        rasterizer_secondary_groups_drawn = 0;
        rasterizer_transparent_geometry_group_draw(group, 0);
        rasterizer_render_states_dirty = 1;
        return allocated;
    }

    if (!rasterizer_model_scratch_valid) {
        rasterizer_model_scratch_node_matrices =
            chimera__rasterizer_memory_alloc((void *)(uintptr_t)context->node_matrices,
                                             (uint32_t)(context->node_count * 0x34));
        rasterizer_model_scratch_node_count = context->node_count;
        rasterizer_model_scratch_lighting = chimera__rasterizer_memory_alloc(&context->lighting, 0x74);
        rasterizer_model_scratch_function_source = chimera__rasterizer_memory_alloc(&context->change_colors, 8);
        rasterizer_model_scratch_valid = 1;
    }
    group->node_matrices = (uint32_t)(uintptr_t)rasterizer_model_scratch_node_matrices;
    group->lighting = (uint32_t)(uintptr_t)rasterizer_model_scratch_lighting;
    group->node_count = rasterizer_model_scratch_node_count;
    group->lighting_extra = (uint32_t)(uintptr_t)rasterizer_model_scratch_function_source;
    return allocated;
}








typedef void (*transparent_geometry_callback)(int32_t argument, int32_t count);

typedef void (*transparent_geometry_draw_procedure)(transparent_geometry_group *group);

typedef void (*transparent_geometry_draw_procedure2)(transparent_geometry_group *group, int16_t kind);


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

static void set_vertex_declaration(uint32_t declaration)
{
    render_device().set_vertex_declaration(declaration);
}

static void set_vertex_shader(uint32_t shader)
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
    set_render_state(0x16, cull_mode);
    set_render_state(0xa8, 0);
    set_render_state(0x1b, 0);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 1);
    set_render_state(0x0e, 1);
    set_render_state(0x17, 4);
    set_render_state(0x3c, texture_factor);
    render_device().set_pixel_shader(0);
    set_texture_stage_state(0, 1, 2);
    set_texture_stage_state(0, 2, 3);
    set_texture_stage_state(0, 4, 2);
    set_texture_stage_state(0, 5, 3);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
}

static void set_group_skinning(const transparent_geometry_group *flags_group,
                               const transparent_geometry_group *source)
{
    rasterizer_node_matrices nodes;

    if (source->node_matrices != 0 && source->node_count != 0) {
        nodes.matrices = source->node_matrices;
        nodes.node_count = source->node_count;
    } else {
        nodes.matrices = (uint32_t)(uintptr_t)k_render_identity_matrix_ptr;
        nodes.node_count = 1;
    }
    chimera__rasterizer_set_model_skinning((uint8_t)(~(uint8_t)(flags_group->flags >> 8) & 1), &nodes);
}

static int16_t shader_type_of(const uint8_t *shader)
{
    return *(int16_t *)&((struct Shader *)shader)->shader_type;
}

static void draw_particle_effect_shader(transparent_geometry_group *group, const uint8_t *shader)
{
    uint8_t has_texture_animation;
    int16_t effect_index;
    rasterizer_effect_slot *effect;
    BitmapData *bitmap;
    float view_matrix[3][4];
    float texture_matrix[4][4];
    int i, j;

    has_texture_animation = 0;
    if (*(const int32_t *)(shader + 0x58) != -1 && *(const int16_t *)(shader + 0x5c) != 2) {
        has_texture_animation = 1;
    }
    effect_index = (shader[0x28] & 2) ? 0x5a : 0x60;
    if ((group->flags & 4) == 0) {
        switch (*(const int16_t *)(shader + 0x2a)) {
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

    bitmap = (BitmapData *)(uintptr_t)group->lightmap_bitmap;
    if (bitmap != NULL && ((struct BitmapData *)bitmap)->hardware_texture != 0) {
        uint8_t map_flags = shader[0x2e];
        uint32_t filter = (map_flags & 1) ? 1 : 2;

        rasterizer_bind_texture_d3d9(0, bitmap);
        set_sampler_state(0, 1, (map_flags & 2) | 1);
        set_sampler_state(0, 2, ((map_flags & 4) | 2) >> 1);
        set_sampler_state(0, 5, filter);
        set_sampler_state(0, 6, filter);
        set_sampler_state(0, 7, filter);
    }
    set_render_state(0x16, 1);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x0f, 0);
    set_render_state(0x1c, 0);
    chimera__rasterizer_set_framebuffer_blend_function(*(const int16_t *)(shader + 0x2a));

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
    if (group->flags & 0x20) {

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
        shader_texture_animation_evaluate((const void *)(uintptr_t)group->lighting_extra, shader + 0x60,
                                          texture_matrix[2], texture_matrix[3],
                                          group->base_map_u_scale, group->base_map_v_scale, 0.0f, 0.0f, 0.0f,
                                          (float)rasterizer_time.time);
    }
    render_device().set_vertex_shader_constant_f(0x1a, &view_matrix[0][0], 3);
    render_device().set_vertex_shader_constant_f(0x0d, &texture_matrix[0][0], 4);
    set_vertex_declaration(rasterizer_vertex_declarations[6].declaration);
    set_vertex_shader(rasterizer_vertex_shaders[effect->vertex_shader_index].shader);
    draw_effect_passes((void *)(uintptr_t)effect->effect, group);
}

static void draw_glass_shader(transparent_geometry_group *group, const uint8_t *shader)
{
    int16_t reflection_type = *(const int16_t *)(shader + 0x8a);

    set_render_state(0x16, (shader[0x28] & 4) ? 1 : 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0xab, 1);
    set_render_state(0x18, 0);
    set_render_state(0x1c, 0);
    set_sampler_state(0, 1, 1);
    set_sampler_state(0, 2, 1);
    set_sampler_state(0, 5, 2);
    set_sampler_state(0, 6, 2);
    set_sampler_state(0, 7, 2);

    if (reflection_type == 2 && (rasterizer_window.has_mirror == 0 || rasterizer_window.type != 1)) {

        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            ((transparent_geometry_draw_procedure2)rasterizer_glass_draw_procedures[2])(group, 2);
        }
        return;
    }
    if (*(const int32_t *)(shader + 0x70) != -1 ||
        *(const float *)(shader + 0x54) != 0.0f ||
        *(const float *)(shader + 0x58) != 0.0f ||
        *(const float *)(shader + 0x5c) != 0.0f) {
        ((transparent_geometry_draw_procedure)rasterizer_glass_draw_procedures[1])(group);
    }
    if ((*(const float *)(shader + 0x8c) > 0.0f || *(const float *)(shader + 0x9c) > 0.0f) &&
        (*(const int32_t *)(shader + 0xb8) != -1 || reflection_type == 2)) {
        if (reflection_type == 0 &&
            ((shader[0x28] & 8) != 0 || *(const int32_t *)(shader + 0xcc) == -1)) {
            reflection_type = 1;
        }
        ((transparent_geometry_draw_procedure2)rasterizer_glass_draw_procedures[2])(group, reflection_type);
    }
    if (*(const int32_t *)(shader + 0x164) != -1 || *(const int32_t *)(shader + 0x178) != -1) {
        ((transparent_geometry_draw_procedure)rasterizer_glass_draw_procedures[0])(group);
    }
}

static void draw_meter_shader(transparent_geometry_group *group, const uint8_t *shader, int16_t vertex_type)
{
    int16_t variant = 0;
    float meter_brightness, flash_brightness, value, gradient;
    float flash[3];
    float scaled_gradient;
    float pixel_constants[6][4];
    float vertex_constants[3][4];
    void *effect;
    int i;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
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
        const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);

        if (function_values != NULL) {
            int16_t source;

            source = *(const int16_t *)(shader + 0xd8);
            if (source >= 1 && source <= 4) meter_brightness = function_values[source - 1];
            source = *(const int16_t *)(shader + 0xda);
            if (source >= 1 && source <= 4) flash_brightness = function_values[source - 1];
            source = *(const int16_t *)(shader + 0xdc);
            if (source >= 1 && source <= 4) value = function_values[source - 1];
            source = *(const int16_t *)(shader + 0xde);
            if (source >= 1 && source <= 4) gradient = function_values[source - 1];
        }
    }
    if (console_debug_toggle_6893eb) {
        float animated = (float)periodic_function_evaluate(2, rasterizer_time.time / console_debug_meter_period);

        meter_brightness = (console_debug_meter_values[0] < 0.0f) ? animated : console_debug_meter_values[0];
        flash_brightness = (console_debug_meter_values[1] < 0.0f) ? animated : console_debug_meter_values[1];
        value = (console_debug_meter_values[2] < 0.0f) ? animated : console_debug_meter_values[2];
        gradient = (console_debug_meter_values[3] < 0.0f) ? animated : console_debug_meter_values[3];
    }

    flash[0] = flash_brightness * *(const float *)(shader + 0xa0);
    flash[1] = flash_brightness * *(const float *)(shader + 0xa4);
    flash[2] = flash_brightness * *(const float *)(shader + 0xa8);
    scaled_gradient = gradient * 8.0f;
    if (!(scaled_gradient > 1.0f)) {
        scaled_gradient = 1.0f;
    }

    pixel_constants[0][0] = flash[0];
    pixel_constants[0][1] = flash[1];
    pixel_constants[0][2] = flash[2];
    pixel_constants[0][3] = 1.0f;
    pixel_constants[1][0] = *(const float *)(shader + 0x88);
    pixel_constants[1][1] = *(const float *)(shader + 0x8c);
    pixel_constants[1][2] = *(const float *)(shader + 0x90);
    pixel_constants[1][3] = 1.0f / scaled_gradient;
    pixel_constants[2][0] = *(const float *)(shader + 0x7c);
    pixel_constants[2][1] = *(const float *)(shader + 0x80);
    pixel_constants[2][2] = *(const float *)(shader + 0x84);
    pixel_constants[2][3] = value;
    pixel_constants[3][0] = *(const float *)(shader + 0x94);
    pixel_constants[3][1] = *(const float *)(shader + 0x98);
    pixel_constants[3][2] = *(const float *)(shader + 0x9c);
    pixel_constants[3][3] = 1.0f;
    pixel_constants[4][0] = *(const float *)(shader + 0xac);
    pixel_constants[4][1] = *(const float *)(shader + 0xb0);
    pixel_constants[4][2] = *(const float *)(shader + 0xb4);
    pixel_constants[4][3] = 1.0f;
    if (shader[0x28] & 4) {
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
    if (shader[0x28] & 8) {
        pixel_constants[0][3] = *(const float *)(shader + 0xb8);
        pixel_constants[3][3] = *(const float *)(shader + 0xbc);
        pixel_constants[4][0] *= meter_brightness;
        pixel_constants[4][1] *= meter_brightness;
        pixel_constants[4][2] *= meter_brightness;
        pixel_constants[4][3] = *(const float *)(shader + 0xb8);
    } else {
        pixel_constants[0][3] = meter_brightness;
        pixel_constants[3][3] = 0.0f;
        pixel_constants[4][3] = meter_brightness;
    }

    rasterizer_resolve_and_cache_submap_b(*(const uint32_t *)(shader + 0x58), 0, 0, 1,
                                          (int16_t)group->shader_permutation, &rasterizer_effects[111]);
    set_sampler_state(0, 1, 1);
    set_sampler_state(0, 2, 1);
    set_sampler_state(0, 5, (shader[0x28] & 0x10) ? 1 : 2);
    set_sampler_state(0, 6, (shader[0x28] & 0x10) ? 1 : 2);
    set_sampler_state(0, 7, (shader[0x28] & 0x10) ? 1 : 2);
    set_render_state(0x16, (*(const uint16_t *)(shader + 0x28) & 2) ? 1 : 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 2);
    set_render_state(0x14, 2);
    set_render_state(0xab, 1);
    set_render_state(0x0f, 0);
    set_render_state(0x1c, 0);

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
        set_render_state(0x1b, 0);
    }
    render_device().set_pixel_shader_constant_f(0, &pixel_constants[0][0], 6);
    effect = (void *)(uintptr_t)rasterizer_effects[111].effect;
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
    uint8_t *shader;
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
                shader = (uint8_t *)(uintptr_t)cursor->shader;
                if (shader == NULL ||
                    !((shader_type_of(shader) == 5 || shader_type_of(shader) == 6 || shader_type_of(shader) == 7) &&
                      ((shader[0x29] >> 5) & 1) != 0)) {

                    set_group_skinning(group, cursor);
                    if (group->flags & 0x100) {
                        chimera__rasterizer_set_up_node_parts(cursor->node_part_count,
                                                              (uint8_t *)(uintptr_t)cursor->node_part_indices);
                    }
                    if (group->lighting != 0) {
                        rasterizer_prepare_lighting_constants((render_lighting *)(uintptr_t)cursor->lighting);
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

        if ((group->flags & 2) == 0 && group->parameters.mode == 1) {
            key = group->sort_key;
            shader = (uint8_t *)(uintptr_t)group->shader;
            if (key != transparent_geometry_group_last_drawn_key && shader != NULL &&
                shader_type_of(shader) == 4 && !attached) {
                set_depth_prepass_states(3, 0xffffffff);
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
                set_render_state(0xa8, 7);
            }
        }
    }

    if ((group->flags & 2) == 0 && rasterizer_window.type == 1 && !attached) {
        shader = (uint8_t *)(uintptr_t)group->shader;
        if (console_debug_toggle_689422) {
            if (shader != NULL && shader_type_of(shader) == 4 && group->parameters.mode == 1 &&
                group->sort_key != transparent_geometry_group_last_drawn_key) {
                rasterizer_render_target_capture_frame();
            }
        } else if (shader == NULL || (shader_type_of(shader) != 8 && !shader_draw_before_water(shader))) {
            rasterizer_render_target_capture_frame();
        }
    }

    immediate = group->flags & 2;
    if (immediate == 0 && rasterizer_window.type == 1 && group->parameters.mode == 1) {
        shader = (uint8_t *)(uintptr_t)group->shader;
        if (shader != NULL && shader_type_of(shader) == 4 && !attached) {
            transparent_geometry_group *next = transparent_geometry_group_get_next_sorted(group);

            if (next == NULL || next->parameters.mode != 1 || next->sort_key != group->sort_key ||
                next->shader == 0 || shader_type_of((uint8_t *)(uintptr_t)next->shader) != 4) {
                draw_secondary_groups = 1;
            }
        }
    }

    if (group->shader == 0) {
        ((transparent_geometry_callback)(uintptr_t)group->index_buffer)(group->first_index, group->primitive_count);
        goto finish;
    }

    vertex_type = -1;
    if (group->vertex_buffer != 0) {
        vertex_type = *(int16_t *)(uintptr_t)group->vertex_buffer;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    if (immediate == 0) {
        set_group_skinning(group, group);
        if (group->flags & 0x100) {
            chimera__rasterizer_set_up_node_parts(group->node_part_count,
                                                  (uint8_t *)(uintptr_t)group->node_part_indices);
        }
        if (group->lighting != 0) {
            rasterizer_prepare_lighting_constants((render_lighting *)(uintptr_t)group->lighting);
        }
    }
    if (group->flags & 8) {
        if (rasterizer_window.type == 1) {
            chimera__rasterizer_set_frustum_z_func(0x3b800000, 0x45800000);
        }
        set_render_state(0x07, 0);
    } else {
        set_render_state(0x07, 1);
        set_render_state(0x0e, 0);
        set_render_state(0x17, 4);
        if (shader_is_decal((const Shader *)((void *)(uintptr_t)group->shader))) {
            chimera__transparent_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }

    for (pass = 0; pass < 2; pass++) {
        if ((int8_t)group->flags < 0) {

            if (group->parameters.mode == 1) {
                if (pass > 0) {
                    break;
                }
                chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
            } else if (pass == 0) {
                shader = (uint8_t *)(uintptr_t)group->shader;
                if (shader != NULL && shader_type_of(shader) == 1 && (shader[0x28] & 4) != 0) {
                    continue;
                }
                rasterizer_set_shader_stage_config(3);
            } else {
                rasterizer_set_shader_stage_config(2);
                set_render_state(0x07, 0);
            }
        } else if (pass > 0) {
            break;
        }

        shader = (uint8_t *)(uintptr_t)group->shader;
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
            ((transparent_geometry_draw_procedure)rasterizer_water_draw_procedure)(group);
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

    if ((group->flags & 8) && rasterizer_window.type == 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
    if ((int8_t)group->flags < 0 && group->parameters.mode == 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
    if (rasterizer_caps.raster_caps & 0x04000000) {
        set_render_state(0xc3, 0);
    }
    if (rasterizer_caps.raster_caps & 0x02000000) {
        set_render_state(0xaf, 0);
    }

finish:
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

    if (console_debug_toggle_689421 == 0 || rasterizer_window.type != 1) {
        return;
    }
    amount = group->parameters.blend_factor;
    shader = (const Shader *)group->shader;
    if ((int8_t)group->flags < 0) {
        chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
    }

    if (rasterizer_caps_flag_688 == 0 && rasterizer_caps_flag_68a == 0 &&
        rasterizer_caps.pixel_shader_version >= 0xffff0101) {
        void *effect = (void *)rasterizer_effects[105].effect;

        if (effect != 0) {
            const GlobalsRasterizerData *data = rasterizer_globals_data;
            float t = group->parameters.distortion_factor;
            float half_height;
            float constants[12];
            uint32_t passes;
            int16_t vertex_type;

            vertex_type = (int16_t)transparent_geometry_group_get_vertex_type_reference(group);
            render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[vertex_type].declaration);

            rasterizer_set_render_state(0x16, (~(uint32_t)*(uint16_t *)((const uint8_t *)shader + 0x28) & 2) | 1);
            rasterizer_set_render_state(0xa8, 7);
            rasterizer_set_render_state(7, 1);
            rasterizer_set_render_state(0xe, 1);
            rasterizer_set_render_state(0x17, 4);
            rasterizer_set_render_state(0x1c, 0);
            rasterizer_set_render_state(0x1b, 0);
            chimera__rasterizer_set_texture_direct_d3d9(*(const uint32_t *)&data->active_camouflage_distortion.tag_id, 0, 0);
            rasterizer_set_sampler_state(0, 1, 3);
            rasterizer_set_sampler_state(0, 2, 3);
            rasterizer_set_sampler_state(0, 3, 3);
            rasterizer_set_sampler_state(0, 5, 2);
            rasterizer_set_sampler_state(0, 6, 2);
            rasterizer_set_sampler_state(0, 7, 2);
            render_device().set_texture(2, (void *)rasterizer_render_targets[2].texture);
            rasterizer_set_sampler_state(2, 1, 3);
            rasterizer_set_sampler_state(2, 2, 3);
            rasterizer_set_sampler_state(2, 5, 2);
            rasterizer_set_sampler_state(2, 6, 2);
            rasterizer_set_sampler_state(2, 7, 1);
            render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[30].shader);

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
        rasterizer_set_render_state(0x17, 3);
    } else {
        if (amount > 0.9f) {
            amount = 0.9f;
        }
        rasterizer_set_render_state(0x17, 3);
    }

    if (amount < 1.0f) {
        rasterizer_model_draw_context context;

        context.flags = group->flags & (_group_sort_first_bit | _group_node_parts_bit);
        context.node_matrices = group->node_matrices;
        context.node_count = group->node_count;
        if (group->lighting != 0) {
            context.lighting = *(const render_lighting *)group->lighting;
        } else {
            uint32_t *words = (uint32_t *)&context.lighting;
            int32_t i;

            for (i = 0; i < 0x1d; i++) {
                words[i] = 0;
            }
        }
        if (group->lighting_extra != 0) {
            context.change_colors = ((const uint32_t *)group->lighting_extra)[0];
            context.function_values = ((const uint32_t *)group->lighting_extra)[1];
        } else {
            context.change_colors = 0;
            context.function_values = 0;
        }
        {
            uint32_t *words = (uint32_t *)&context.group_parameters;
            int32_t i;

            for (i = 0; i < 10; i++) {
                words[i] = 0;
            }
        }
        context.center = group->position;
        context.base_map_u_scale = group->base_map_u_scale;
        context.base_map_v_scale = group->base_map_v_scale;

        rasterizer_set_render_state(0xe, 0);
        rasterizer_camouflage_fade_active = 1;
        rasterizer_camouflage_fade = 1.0f - amount;
        if (halo::rasterizer::globals::models_enabled != 0) {
            rasterizer_render_states_dirty = 1;
            halo::rasterizer::globals::sky_pass_active = 0;
            if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
                rasterizer_set_render_state(0x89, 1);
            }
        }
        rasterizer_model_draw_prepare_states(&context, 1);
        rasterizer_shader_environment_draw_dispatch(group->dynamic_vertex_slot, (uint8_t *)(uintptr_t)group->shader, (int16_t)group->shader_permutation, (rasterizer_index_buffer *)(uintptr_t)group->index_buffer, group->dynamic_index_slot, group->primitive_count, (rasterizer_vertex_buffer *)(uintptr_t)group->vertex_buffer);
        rasterizer_model_draw_restore_states();
        render_lighting_disable_workaround();
        rasterizer_camouflage_fade_active = 0;
    }

    if ((int8_t)group->flags < 0) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
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
    rasterizer_index_buffer *index_buffer = (rasterizer_index_buffer *)(uintptr_t)group->index_buffer;
    rasterizer_vertex_buffer *vertex_buffer = (rasterizer_vertex_buffer *)(uintptr_t)group->vertex_buffer;

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
void rasterizer_transparent_geometry_group_new(Shader *shader, int16_t shader_permutation, uint32_t lightmap_bitmap, uint32_t dynamic_index_slot, uint32_t first_index, uint32_t primitive_count, uint32_t vertex_buffer, ColorARGB *tint, uint32_t lighting, uint32_t flags, real_point3d *world_position)
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
        flags = flags | 1;
    }
    if (shader_is_decal(shader) != 0) {
        flags = flags | 7;
    }

    if ((flags & 2) == 0) {
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
    group->shader = (uint32_t)shader;
    group->index_buffer = 0;
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

    group->lighting = (uint32_t)chimera__rasterizer_memory_alloc((void *)lighting, 0x74);
    group->lighting_extra = 0;

    if (shader->shader_type == 8) {
        halo::rasterizer::globals::transparent_group_created = 1;
    }
    if (shader->shader_type == 8 && (*((uint8_t *)shader + 0x28) & 8) != 0) {
        group->flags = group->flags | 2;
        rasterizer_transparent_geometry_group_draw(group, 0);
        transparent_geometry_group_set_drawn_bit(group, 1);
        group->flags = group->flags & 0xfffffffd;
        return;
    }
    if ((flags & 2) != 0) {
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
void rasterizer_transparent_object_append(uint32_t lightmap_bitmap, int32_t dynamic_index_slot, int32_t dynamic_vertex_slot, int32_t primitive_count, uint32_t flags, real_point3d *world_position, Shader *shader)
{
    transparent_geometry_group *group;
    float dx, dy, dz;

    if (halo::rasterizer::globals::transparent_object_append_enabled == 0) {
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
        group->shader = (uint32_t)shader;
        group->shader_permutation = 0;
        group->parameters.mode = 0;
        group->index_buffer = 0;
        group->first_index = 0;
        group->vertex_buffer = 0;
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

        if (shader->shader_type == 1 && (*((uint8_t *)shader + 0x28) & 1) != 0) {
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

    if (transparent_geometry_group_count >= 0x180) {
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

    if (transparent_geometry_group_secondary_count >= 0x20) {
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
           ((((uint8_t *)shader)[0x29] >> 4 & 1) != 0);
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

    if (is_batched_shader(shader_a)) {
        result = -1;
        goto first_person_check;
    }
    if (is_batched_shader(shader_b)) {
        result = 1;
        goto first_person_check;
    }

    if (shader_a == (Shader *)0 || shader_a->shader_type != 8) {
        if (shader_b != (Shader *)0 && shader_b->shader_type == 8) {
            result = 1;
            goto first_person_check;
        }
        if ((ga->flags & 0x80) == 0) {
            if ((gb->flags & 0x80) != 0) {
                result = -1;
                goto first_person_check;
            }
        } else {
            if ((gb->flags & 0x80) == 0) {
                result = 1;
                goto first_person_check;
            }
        }
        if (gb->depth < ga->depth) {
            result = 1;
            goto first_person_check;
        }
        if (gb->depth <= ga->depth) {
            if (gb->sort_key < ga->sort_key) {
                result = 1;
                goto first_person_check;
            }
            if (gb->sort_key <= ga->sort_key) {
                result = 0;
                goto first_person_check;
            }
        }
    }
    result = -1;

first_person_check:
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
            Shader *shader = (Shader *)group->shader;
            if (shader == (Shader *)0 ||
                (shader->shader_type != 8 &&
                 ((shader->shader_type != 5 && shader->shader_type != 6 && shader->shader_type != 7) ||
                  ((((uint8_t *)shader)[0x29] >> 4 & 1) == 0)))) {
                break;
            }
        }

        if ((int8_t)group->flags < 0 && applied_frustum_z == 0) {
            rasterizer_set_shader_stage_config(0);
            chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
            applied_frustum_z = 1;
        }

        rasterizer_transparent_geometry_group_draw(group, 0);
        transparent_geometry_group_draw_cursor = transparent_geometry_group_draw_cursor + 1;
        cursor = transparent_geometry_group_draw_cursor;
    } while (cursor < transparent_geometry_group_count);

    if (applied_frustum_z != 0) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
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
    rasterizer_vertex_buffer *vb = (rasterizer_vertex_buffer *)group->vertex_buffer;

    if (vb != (rasterizer_vertex_buffer *)0) {
        return 0xffff0000u | (uint16_t)vb->type;
    }
    if (group->dynamic_vertex_slot != -1) {
        return 0xffff0000u | (uint16_t)rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    return 0xffffffffu;
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
    uint8_t *base = (uint8_t *)transparent_geometry_groups;
    uint8_t *p = (uint8_t *)group;

    if (base <= p && p < base + (uint32_t)transparent_geometry_group_count * 0xa8) {
        return (int32_t)(p - base) / 0xa8;
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
    uint8_t *base = (uint8_t *)transparent_geometry_groups;
    uint8_t *p = (uint8_t *)group;
    int32_t index;

    if (!(base <= p && p < base + (uint32_t)transparent_geometry_group_count * 0xa8)) {
        return;
    }
    index = (int32_t)(p - base) / 0xa8;
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
    uint8_t *base = (uint8_t *)transparent_geometry_groups;
    uint8_t *p = (uint8_t *)group;
    int32_t index = -1;

    if (base <= p && p < base + (uint32_t)transparent_geometry_group_count * 0xa8) {
        index = (int32_t)(p - base) / 0xa8;
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
    uint32_t secondary_pool;

    transparent_geometry_groups = (transparent_geometry_group *)GlobalAlloc(0, 0xfc00);
    transparent_geometry_group_sorted_indices = (int16_t *)GlobalAlloc(0, 0x300);
    secondary_pool = (uint32_t)GlobalAlloc(0, 0x1500);
    transparent_geometry_group_secondary_count = 0;
    transparent_geometry_group_count = 0;
    transparent_geometry_groups_secondary = (transparent_geometry_group *)secondary_pool;

    if (transparent_geometry_groups != (transparent_geometry_group *)0 &&
        transparent_geometry_group_sorted_indices != (int16_t *)0 && secondary_pool != 0) {
        uint32_t result = rasterizer_misc_vertex_buffer_create();
        if ((uint8_t)result != 0) {
            return (int32_t)((result & 0xffffff00) | 1);
        }
        return (int32_t)(result & 0xffffff00);
    }
    return (int32_t)(secondary_pool & 0xffffff00);
}

}  // namespace halo::rasterizer
