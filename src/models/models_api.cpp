/**
 * @file src/models/models_api.cpp
 * The models module's free-function API (include/halo/models/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/models/models.hpp"
#include "halo/models/api.hpp"

namespace halo::models {

void * animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame)
{
    return halo::models::animation_view(animation).get_frame_data(frame);
}

void animation_get_frame_info_distance(ModelAnimationsAnimation *animation, float *dx_to_key_frame, float *dx_total)
{
    halo::models::animation_view(animation).get_frame_info_distance(dx_to_key_frame, dx_total);
}

void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model, int16_t frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).get_frame_orientations(model, frame, out_orientations);
}

void animation_node_get_rotation(ModelAnimationsAnimation *animation, real frame, int16_t rotation_index, int16_t node, real_quaternion *out)
{
    halo::models::animation_view(animation).node_get_rotation(frame, rotation_index, node, out);
}

void animation_node_get_translation(ModelAnimationsAnimation *animation, real frame, int16_t translation_index, int16_t node, real_point3d *out)
{
    halo::models::animation_view(animation).node_get_translation(frame, translation_index, node, out);
}

void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index, real frame, real *out)
{
    halo::models::animation_view(animation).node_get_scale(scale_index, frame, out);
}

void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_frame_orientations(frame, out_orientations);
}

void animation_overlay_frame_orientations_weighted(ModelAnimationsAnimation *animation, int16_t frame, float weight, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_frame_orientations_weighted(frame, weight, out_orientations);
}

void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_interpolated_frame_orientations(frame, out_orientations);
}

void animation_overlay_interpolated_frame_orientations_weighted(ModelAnimationsAnimation *animation, float frame, float weight, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_interpolated_frame_orientations_weighted(frame, weight, out_orientations);
}

void animation_replace_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).replace_frame_orientations(frame, out_orientations);
}

void animation_aiming_screen_blend(ModelAnimationsAnimation *animation, animation_aiming_screen *screen, real yaw, real pitch, real_orientation *orientation_out)
{
    halo::models::animation_view(animation).aiming_screen_blend(screen, yaw, pitch, orientation_out);
}

animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream)
{
    return halo::models::animation_graph::state_advance(animation_graph_tag_index, state, sound_tag_id, random_stream);
}

int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream)
{
    return halo::models::animation_graph::choose_random_permutation(animation_graph_tag, first_animation, stream);
}

int16_t animation_graph_find_animation_by_name(datum_index animation_graph_tag, const char *name)
{
    return halo::models::animation_graph::find_animation_by_name(animation_graph_tag, name);
}

int16_t animation_keyframe_time_search(uint16_t *times, int16_t count, int16_t frame)
{
    return halo::models::animation_graph::keyframe_time_search(times, count, frame);
}

void animation_quaternion16_decode(int16_t *source, real_quaternion *out)
{
    halo::models::animation_graph::quaternion16_decode(source, out);
}

void animation_quaternion48_decode(animation_quaternion48 *source, real_quaternion *out)
{
    halo::models::animation_graph::quaternion48_decode(source, out);
}

void animation_get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model)
{
    halo::models::animation_graph::get_root_node_matrix(out, frame, animation, model);
}

void model_animation_get_frame_delta(int16_t frame, ModelAnimationsAnimation *animation, real_vector3d *out, GBXModel *model)
{
    halo::models::animation_graph::get_frame_delta(frame, animation, out, model);
}

void animation_graph_nodes_build_matrices(datum_index animation_graph_tag, real_point3d *root_position, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *forward, real_vector3d *up)
{
    halo::models::animation_graph::nodes_build_matrices(animation_graph_tag, root_position, out_matrices, orientations, forward, up);
}

void model_nodes_blend_transforms(real_orientation *in_out, int16_t node_count, real_orientation *other, int16_t step, int16_t steps)
{
    halo::models::model_skeleton::blend_transforms(in_out, node_count, other, step, steps);
}

void model_nodes_build_matrices(real_point3d *root_position, real_vector3d *forward, GBXModel *model, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *up)
{
    halo::models::model_skeleton::build_matrices(root_position, forward, model, out_matrices, orientations, up);
}

void model_ik_solve_two_bone(real_matrix4x3 *target, real_matrix4x3 *middle, real_matrix4x3 *end, real_matrix4x3 *out_end)
{
    halo::models::model_skeleton::ik_solve_two_bone(target, middle, end, out_end);
}

void model_nodes_get_default_transforms(GBXModel *model, real_orientation *out)
{
    halo::models::model_view(model).get_default_transforms(out);
}

void model_render_parts(GBXModel *model, uint8_t *region_permutations, rasterizer_node_matrices *node_matrices, model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags)
{
    halo::models::model_view(model).render_parts(region_permutations, node_matrices, lod, forced_shader_permutation, flags);
}

int16_t model_marker_group_index_from_name(datum_index model_tag_id, const char *name)
{
    return halo::models::model_markers::group_index_from_name(model_tag_id, name);
}

int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations, int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum)
{
    return halo::models::model_markers::get_by_name(model_tag_id, name, region_permutations, node_remap, node_matrices, mirrored, out, maximum);
}

}
