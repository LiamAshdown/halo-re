/**
 * @file src/models/model_render.cpp
 * Model view operations: bind pose and part rendering, and the top-level model render call.
 * The original author notes and decompiles are in docs/original/models/.
 */

#include "halo/models/models.hpp"

extern "C" {
extern tag_instance *tag_instances;
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m);
extern void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices);
extern void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint8_t *shader, int16_t frame,
                                                          rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                          int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer);
extern void rasterizer_object_shadow_model_draw(const ShaderModel *shader, int16_t frame,
                                                 rasterizer_index_buffer *index_buffer,
                                                 rasterizer_vertex_buffer *vertex_buffer);
extern transparent_geometry_group *rasterizer_transparent_geometry_group_build(
    transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
    int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_vertex_slot, const real_point3d *position);
extern Scenario *global_scenario;
extern uint8_t model_render_first_person;
extern uint8_t model_render_default_region_permutations[8];
extern render_model_effect model_render_default_effect;
extern ColorRGB model_render_default_change_colors[4];
extern float model_render_default_function_values[4];
extern int16_t console_model_lod_override;
extern real_matrix4x3 render_camera_world_to_view;
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);
extern uint8_t rasterizer_caps_flag_689;
extern uint8_t console_debug_toggle_6893f2;
extern rasterizer_window_parameters rasterizer_window;
extern rasterizer_model_draw_context *rasterizer_object_shadow_model_context;
extern uint8_t rasterizer_object_shadow_model_active;
extern void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t mode);
extern void rasterizer_model_draw_restore_states(void);
extern void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes);
extern void debug_fp_render_model_note(uint32_t model_tag, float pixels, int32_t lod, const float *node0,
    const float *center, int32_t early_out);
extern void debug_fp_clip_note(const float *world, int32_t effect_type);
extern void debug_fp_state_arm(int32_t armed);
}

namespace halo::models {

void model_view::get_default_transforms(real_orientation *out)
{
    ModelNode *nodes;
    int16_t node;

    nodes = (ModelNode *)self->nodes.pointer;
    for (node = 0; (int32_t)node < (int32_t)self->nodes.count; node++) {
        out[node].rotation = *(real_quaternion *)&nodes[node].default_rotation;
        out[node].translation = *(real_point3d *)&nodes[node].default_translation;
        out[node].scale = 1.0f;
    }
}

void model_view::render_parts(uint8_t *region_permutations, rasterizer_node_matrices *node_matrices, model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags)
{
    int16_t max_pass;
    model_render_pass pass;
    model_part_group_link links[k_maximum_model_part_group_links];

    max_pass = (int16_t)((~flags) & 2);

    for (pass = _model_render_pass_opaque; (int16_t)pass <= max_pass; pass = (model_render_pass)(pass + 1)) {
        int16_t region;
        int16_t link_count = 0;

        for (region = 0; (int32_t)region < self->regions.count; region++) {
            int8_t permutation_index;
            ModelRegion *region_def;
            ModelRegionPermutation *permutations;
            ModelRegionPermutation *perm;
            int16_t geometry_index;
            GBXModelGeometry *geometry;
            int16_t part;

            permutation_index = (int8_t)region_permutations[region];
            if (permutation_index == -1) {
                continue;
            }

            region_def = &((ModelRegion *)self->regions.pointer)[region];
            permutations = (ModelRegionPermutation *)region_def->permutations.pointer;
            perm = &permutations[(int32_t)permutation_index];
            geometry_index = (&perm->super_low)[lod];
            if (geometry_index == -1) {
                continue;
            }

            geometry = &((GBXModelGeometry *)self->geometries.pointer)[geometry_index];

            for (part = 0; (int32_t)part < geometry->parts.count; part++) {
                GBXModelGeometryPart *p = &((GBXModelGeometryPart *)geometry->parts.pointer)[part];
                ModelShaderReference *shader_ref = &((ModelShaderReference *)self->shaders.pointer)[(int16_t)p->base.shader_index];
                Shader *shader = (Shader *)tag_instances[shader_ref->shader.tag_id.index].data;
                int16_t shader_type;
                int16_t permutation;

                if (shader->shader_type <= 2 || shader->shader_type >= 0xc || (p->base.flags & 1) != 0) {
                    continue;
                }

                if ((p->base.flags & 2) != 0) {
                    chimera__rasterizer_set_up_node_parts(p->local_node_count, p->local_node_indices);
                }

                shader_type = (int16_t)shader->shader_type;
                permutation = (forced_shader_permutation == 0) ? (int16_t)shader_ref->permutation
                                                                : (int16_t)forced_shader_permutation;

                if (shader_type == 1 || (4 < shader_type && shader_type < 0xc)) {
                    if (pass == _model_render_pass_transparent) {
                        real_point3d transformed_centroid;
                        real_matrix4x3 *centroid_node_matrix =
                            (real_matrix4x3 *)node_matrices->matrices + (int16_t)p->base.centroid_primary_node;

                        matrix4x3_transform_point(&transformed_centroid, (real_point3d *)&p->base.centroid, centroid_node_matrix);

                        rasterizer_transparent_geometry_group_build(
                            (transparent_geometry_group_link *)&links[link_count], (uint8_t *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type, -1,
                            (int32_t)p->base.triangle_count, (rasterizer_vertex_buffer *)&p->base.vertex_type, -1,
                            &transformed_centroid);

                        if (link_count < k_maximum_model_part_group_links && links[link_count].group_index != -1 &&
                            (flags & _model_render_flag_1_bit) == 0 &&
                            ((int8_t)p->base.next_filthy_part_index > 0 || (int8_t)p->base.prev_filthy_part_index > 0)) {
                            links[link_count].part_index = part;
                            links[link_count].linked_part_index = (int16_t)(int8_t)p->base.next_filthy_part_index;
                            link_count = link_count + 1;
                        }
                    }
                } else if (shader_type == 4 && (((ShaderModel *)shader)->shader_model_flags & 8) != 0) {
                    if (pass == _model_render_pass_model_decal) {
                        rasterizer_shader_environment_draw_dispatch(-1, (uint8_t *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type, -1,
                            p->base.triangle_count, (rasterizer_vertex_buffer *)&p->base.vertex_type);
                    }
                } else if (pass == _model_render_pass_opaque) {
                    if ((flags & _model_render_immediate_bit) == 0) {
                        rasterizer_shader_environment_draw_dispatch(-1, (uint8_t *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type, -1,
                            p->base.triangle_count, (rasterizer_vertex_buffer *)&p->base.vertex_type);
                    } else {
                        rasterizer_object_shadow_model_draw((const ShaderModel *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type,
                            (rasterizer_vertex_buffer *)&p->base.vertex_type);
                    }
                }
            }
        }

        {
            int16_t i;
            for (i = 0; i < link_count; i++) {
                int16_t j;
                for (j = 0; j < link_count; j++) {
                    if (links[i].linked_part_index == links[j].part_index && links[i].linked_part_index > 0) {
                        *(int16_t *)links[i].next_group_index = links[j].group_index;
                        *(int16_t *)links[j].previous_group_index = links[i].group_index;
                        break;
                    }
                }
            }
        }
    }
}

void render_model(TagID model_tag_id, void *node_matrices, float pixels, uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values, render_lighting *lighting, real_point3d *bounding_center, float bounding_radius, render_model_effect *effect, datum_index object_index, uint16_t forced_shader_permutation, uint32_t flags)
{
    GBXModel *model;
    real_matrix4x3 node_matrix_array[k_maximum_nodes_per_model];
    rasterizer_model_draw_context context;
    model_level_of_detail lod;
    int16_t node;

    model = (GBXModel *)tag_instances[model_tag_id.index].data;

    if ((model->node_list_checksum == (int32_t)k_model_first_person_node_list_checksum) &&
        ((global_scenario->flags & 1) != 0)) {
        model_render_first_person = 1;
    } else {
        model_render_first_person = 0;
    }

    if ((&model->super_high_detail_cutoff)[_model_lod_super_low] > pixels && (flags & _model_render_immediate_bit) == 0) {
        if (flags == 8) {
            debug_fp_render_model_note(*(uint32_t *)&model_tag_id, pixels, -1, (const float *)node_matrices,
                (const float *)bounding_center, 1);
        }
        model_render_first_person = 0;
        return;
    }

    if (region_permutations == 0) {
        region_permutations = model_render_default_region_permutations;
    }
    if (effect == 0) {
        effect = &model_render_default_effect;
    }
    if (change_colors == 0) {
        change_colors = model_render_default_change_colors;
    }
    if (function_out_values == 0) {
        function_out_values = model_render_default_function_values;
    }

    if (bounding_center == 0) {
        bounding_center = &((real_matrix4x3 *)node_matrices)->position;
    }

    if (node_matrices == 0) {
        for (node = 0; (int32_t)node < model->nodes.count; node++) {
            node_matrix_array[node] = render_camera_world_to_view;
        }
    } else {
        for (node = 0; (int32_t)node < model->nodes.count; node++) {
            real_matrix4x3 *given = (real_matrix4x3 *)node_matrices + node;
            real_matrix4x3 *inverse_bind = (real_matrix4x3 *)((uint8_t *)model->nodes.pointer +
                                                               node * sizeof(ModelNode) + 0x68);
            matrix4x3_multiply_procedure(given, inverse_bind, &node_matrix_array[node]);
        }
    }

    lod = _model_lod_super_high;
    while (lod > _model_lod_super_low && (&model->super_high_detail_cutoff)[lod] > pixels) {
        lod = (model_level_of_detail)(lod - 1);
    }
    if (console_model_lod_override != -1) {
        if (console_model_lod_override < 0) {
            lod = _model_lod_super_low;
        } else if (console_model_lod_override < k_model_level_of_detail_count) {
            lod = (model_level_of_detail)console_model_lod_override;
        } else {
            lod = _model_lod_super_high;
        }
    }

    context.object_index = (uint32_t)object_index;
    context.lighting = *lighting;
    context.center = *bounding_center;
    context.bounding_radius = bounding_radius;
    context.group_parameters = *(rasterizer_geometry_group_parameters *)effect;

    context.change_colors = (uint32_t)(uintptr_t)change_colors;
    context.function_values = (uint32_t)(uintptr_t)function_out_values;
    context.node_matrices = (uint32_t)(uintptr_t)node_matrix_array;
    context.base_map_u_scale = model->base_map_u_scale;
    context.node_count = (int16_t)model->nodes.count;
    context.base_map_v_scale = model->base_map_v_scale;

    context.flags = 0;
    if ((model->flags & 4) != 0) {
        context.flags |= 0x200;
    }
    if ((flags & _model_render_flag_1_bit) != 0) {
        context.flags |= 0x1f;
    }
    if ((flags & _model_render_outside_fog_plane_bit) != 0) {
        context.flags |= 0x40;
    }
    if ((flags & _model_render_frustum_z_bit) != 0) {
        context.flags |= 0x80;
    }
    if ((model->flags & 2) != 0) {
        context.flags |= 0x100;
    }

    if ((flags & _model_render_immediate_bit) == 0) {
        rasterizer_model_draw_prepare_states(&context, 0);
    } else if (rasterizer_window.type == 1 && rasterizer_caps_flag_689 == 0 && console_debug_toggle_6893f2 != 0) {
        chimera__rasterizer_set_model_skinning((uint8_t)(~(context.flags >> 8) & 1),
                                                (rasterizer_node_matrices *)&context.node_matrices);
        rasterizer_object_shadow_model_context = &context;
        rasterizer_object_shadow_model_active = 1;
    }

    if (flags == 8) {
        debug_fp_render_model_note(*(uint32_t *)&model_tag_id, pixels, lod, (const float *)node_matrices,
            (const float *)bounding_center, 0);
        debug_fp_clip_note(node_matrices ? (const float *)node_matrices + 10 : 0, effect->type);
    }
    if (flags == 8) debug_fp_state_arm(1);
    model_view(model).render_parts(region_permutations, (rasterizer_node_matrices *)&context.node_matrices, lod, forced_shader_permutation, flags);
    debug_fp_state_arm(0);

    if ((flags & _model_render_immediate_bit) != 0) {
        rasterizer_object_shadow_model_context = 0;
        model_render_first_person = 0;
        return;
    }
    rasterizer_model_draw_restore_states();
    model_render_first_person = 0;
}

}  // namespace halo::models
