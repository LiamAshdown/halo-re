/**
 * @file src/models/models_c_api.cpp
 * The models module's C ABI: one extern "C" function per original symbol, same name and signature,
 * forwarding to the halo::models implementation. The shims do nothing else.
 */

#include "halo/models/models_c_api.h"

extern "C" void * animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame)
{
    return halo::models::animation_view(animation).get_frame_data(frame);
}

extern "C" void animation_get_frame_info_distance(ModelAnimationsAnimation *animation, float *dx_to_key_frame, float *dx_total)
{
    halo::models::animation_view(animation).get_frame_info_distance(dx_to_key_frame, dx_total);
}

extern "C" void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model, int16_t frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).get_frame_orientations(model, frame, out_orientations);
}

extern "C" void animation_node_get_rotation(ModelAnimationsAnimation *animation, real frame, int16_t rotation_index, int16_t node, real_quaternion *out)
{
    halo::models::animation_view(animation).node_get_rotation(frame, rotation_index, node, out);
}

extern "C" void animation_node_get_translation(ModelAnimationsAnimation *animation, real frame, int16_t translation_index, int16_t node, real_point3d *out)
{
    halo::models::animation_view(animation).node_get_translation(frame, translation_index, node, out);
}

extern "C" void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index, real frame, real *out)
{
    halo::models::animation_view(animation).node_get_scale(scale_index, frame, out);
}

extern "C" void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_frame_orientations(frame, out_orientations);
}

extern "C" void animation_overlay_frame_orientations_weighted(ModelAnimationsAnimation *animation, int16_t frame, float weight, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_frame_orientations_weighted(frame, weight, out_orientations);
}

extern "C" void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_interpolated_frame_orientations(frame, out_orientations);
}

extern "C" void animation_overlay_interpolated_frame_orientations_weighted(ModelAnimationsAnimation *animation, float frame, float weight, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).overlay_interpolated_frame_orientations_weighted(frame, weight, out_orientations);
}

extern "C" void animation_replace_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations)
{
    halo::models::animation_view(animation).replace_frame_orientations(frame, out_orientations);
}

extern "C" void animation_aiming_screen_blend(ModelAnimationsAnimation *animation, animation_aiming_screen *screen, real yaw, real pitch, real_orientation *orientation_out)
{
    halo::models::animation_view(animation).aiming_screen_blend(screen, yaw, pitch, orientation_out);
}

extern "C" animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream)
{
    return halo::models::animation_graph::state_advance(animation_graph_tag_index, state, sound_tag_id, random_stream);
}

extern "C" int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream)
{
    return halo::models::animation_graph::choose_random_permutation(animation_graph_tag, first_animation, stream);
}

extern "C" int16_t animation_graph_find_animation_by_name(datum_index animation_graph_tag, const char *name)
{
    return halo::models::animation_graph::find_animation_by_name(animation_graph_tag, name);
}

extern "C" int16_t animation_keyframe_time_search(uint16_t *times, int16_t count, int16_t frame)
{
    return halo::models::animation_graph::keyframe_time_search(times, count, frame);
}

extern "C" void animation_quaternion16_decode(int16_t *source, real_quaternion *out)
{
    halo::models::animation_graph::quaternion16_decode(source, out);
}

extern "C" void animation_quaternion48_decode(animation_quaternion48 *source, real_quaternion *out)
{
    halo::models::animation_graph::quaternion48_decode(source, out);
}

extern "C" void animation_get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model)
{
    halo::models::animation_graph::get_root_node_matrix(out, frame, animation, model);
}

extern "C" void model_animation_get_frame_delta(int16_t frame, ModelAnimationsAnimation *animation, real_vector3d *out, GBXModel *model)
{
    halo::models::animation_graph::get_frame_delta(frame, animation, out, model);
}

extern "C" void animation_graph_nodes_build_matrices(datum_index animation_graph_tag, real_point3d *root_position, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *forward, real_vector3d *up)
{
    halo::models::animation_graph::nodes_build_matrices(animation_graph_tag, root_position, out_matrices, orientations, forward, up);
}

extern "C" void model_nodes_blend_transforms(real_orientation *in_out, int16_t node_count, real_orientation *other, int16_t step, int16_t steps)
{
    halo::models::model_skeleton::blend_transforms(in_out, node_count, other, step, steps);
}

extern "C" void model_nodes_build_matrices(real_point3d *root_position, real_vector3d *forward, GBXModel *model, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *up)
{
    halo::models::model_skeleton::build_matrices(root_position, forward, model, out_matrices, orientations, up);
}

extern "C" void model_ik_solve_two_bone(real_matrix4x3 *target, real_matrix4x3 *middle, real_matrix4x3 *end, real_matrix4x3 *out_end)
{
    halo::models::model_skeleton::ik_solve_two_bone(target, middle, end, out_end);
}

extern "C" void model_nodes_get_default_transforms(GBXModel *model, real_orientation *out)
{
    halo::models::model_view(model).get_default_transforms(out);
}

extern "C" void model_render_parts(GBXModel *model, uint8_t *region_permutations, rasterizer_node_matrices *node_matrices, model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags)
{
    halo::models::model_view(model).render_parts(region_permutations, node_matrices, lod, forced_shader_permutation, flags);
}

extern "C" void render_model(TagID model_tag_id, void *node_matrices, float pixels, uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values, render_lighting *lighting, real_point3d *bounding_center, float bounding_radius, render_model_effect *effect, datum_index object_index, uint16_t forced_shader_permutation, uint32_t flags)
{
    halo::models::render_model(model_tag_id, node_matrices, pixels, region_permutations, change_colors, function_out_values, lighting, bounding_center, bounding_radius, effect, object_index, forced_shader_permutation, flags);
}

extern "C" int16_t model_marker_group_index_from_name(datum_index model_tag_id, const char *name)
{
    return halo::models::model_markers::group_index_from_name(model_tag_id, name);
}

extern "C" int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations, int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum)
{
    return halo::models::model_markers::get_by_name(model_tag_id, name, region_permutations, node_remap, node_matrices, mirrored, out, maximum);
}
