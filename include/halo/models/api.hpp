/**
 * @file include/halo/models/api.hpp
 * Functions of the models module that other modules and the data tables call (namespace halo::models). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>

typedef float real;
struct ColorRGB;
struct GBXModel;
struct ModelAnimationsAnimation;
struct TagID;
struct animation_aiming_screen;
struct animation_quaternion48;
struct animation_state;
struct effect;
struct object_marker;
struct rasterizer_node_matrices;
struct real_matrix4x3;
struct real_orientation;
struct real_point3d;
struct real_quaternion;
struct real_vector3d;
struct render_lighting;
struct render_model_effect;
enum animation_random_stream : int;
enum animation_state_advance_result : int;
enum model_level_of_detail : int;
typedef uint32_t datum_index;

namespace halo::models {

void * animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame);
void animation_get_frame_info_distance(ModelAnimationsAnimation *animation, float *dx_to_key_frame, float *dx_total);
void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model, int16_t frame, real_orientation *out_orientations);
void animation_node_get_rotation(ModelAnimationsAnimation *animation, real frame, int16_t rotation_index, int16_t node, real_quaternion *out);
void animation_node_get_translation(ModelAnimationsAnimation *animation, real frame, int16_t translation_index, int16_t node, real_point3d *out);
void animation_node_get_scale(ModelAnimationsAnimation *animation, int16_t scale_index, real frame, real *out);
void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations);
void animation_overlay_frame_orientations_weighted(ModelAnimationsAnimation *animation, int16_t frame, float weight, real_orientation *out_orientations);
void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame, real_orientation *out_orientations);
void animation_overlay_interpolated_frame_orientations_weighted(ModelAnimationsAnimation *animation, float frame, float weight, real_orientation *out_orientations);
void animation_replace_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations);
void animation_aiming_screen_blend(ModelAnimationsAnimation *animation, animation_aiming_screen *screen, real yaw, real pitch, real_orientation *orientation_out);
animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream);
int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream);
int16_t animation_graph_find_animation_by_name(datum_index animation_graph_tag, const char *name);
int16_t animation_keyframe_time_search(uint16_t *times, int16_t count, int16_t frame);
void animation_quaternion16_decode(int16_t *source, real_quaternion *out);
void animation_quaternion48_decode(animation_quaternion48 *source, real_quaternion *out);
void animation_get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model);
void model_animation_get_frame_delta(int16_t frame, ModelAnimationsAnimation *animation, real_vector3d *out, GBXModel *model);
void animation_graph_nodes_build_matrices(datum_index animation_graph_tag, real_point3d *root_position, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *forward, real_vector3d *up);
void model_nodes_blend_transforms(real_orientation *in_out, int16_t node_count, real_orientation *other, int16_t step, int16_t steps);
void model_nodes_build_matrices(real_point3d *root_position, real_vector3d *forward, GBXModel *model, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *up);
void model_ik_solve_two_bone(real_matrix4x3 *target, real_matrix4x3 *middle, real_matrix4x3 *end, real_matrix4x3 *out_end);
void model_nodes_get_default_transforms(GBXModel *model, real_orientation *out);
void model_render_parts(GBXModel *model, uint8_t *region_permutations, rasterizer_node_matrices *node_matrices, model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags);
void render_model(TagID model_tag_id, void *node_matrices, float pixels, uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values, render_lighting *lighting, real_point3d *bounding_center, float bounding_radius, render_model_effect *effect, datum_index object_index, uint16_t forced_shader_permutation, uint32_t flags);
int16_t model_marker_group_index_from_name(datum_index model_tag_id, const char *name);
int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations, int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum);

}
