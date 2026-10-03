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

animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream);
int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream);
void render_model(TagID model_tag_id, void *node_matrices, float pixels, uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values, render_lighting *lighting, real_point3d *bounding_center, float bounding_radius, render_model_effect *effect, datum_index object_index, uint16_t forced_shader_permutation, uint32_t flags);

}
