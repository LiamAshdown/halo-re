/**
 * @file include/halo/models/animation_graph.hpp
 * Animation graph level helpers: state advance, lookup, keyframe search, quaternion codecs and node matrices.
 */
#pragma once

#include "halo/models/models_types.hpp"

namespace halo::models {

/**
 * Stateless operations on animation graph tags addressed by tag index, plus the compressed quaternion codecs and
 * keyframe search shared by the sampling code.
 */
struct animation_graph {
    /**
     * Advances an animation state by one frame against the animation graph tag and reports what happened. When
     * sound_tag_id is non-NULL it receives the sound that starts on the frame just advanced from, or -1.
     *
     * @address 0x4d48d0
     */
    static animation_state_advance_result state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream);

    /**
     * Walks the next_animation chain from first_animation and picks the first entry whose cumulative relative weight
     * reaches a fresh random threshold in [0, 1). Returns -1 for an empty chain or when no weight reaches the threshold.
     *
     * @address 0x4d6280
     */
    static int16_t choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream);

    /**
     * Linear case-insensitive search of the graph's animations for a name. Returns its index or -1.
     *
     * @address 0x4d6ab0
     */
    static int16_t find_animation_by_name(datum_index animation_graph_tag, const char *name);

    /**
     * Binary-searches an ascending table of keyframe times and returns the index of the last keyframe whose time is at or before the frame.
     *
     * @address 0x4d6b10
     */
    static int16_t keyframe_time_search(uint16_t *times, int16_t count, int16_t frame);

    /**
     * Decodes a 16-bit compressed quaternion into a real_quaternion.
     *
     * @address 0x4d6330
     */
    static void quaternion16_decode(int16_t *source, real_quaternion *out);

    /**
     * Decodes a 48-bit compressed quaternion into a real_quaternion.
     *
     * @address 0x4d6380
     */
    static void quaternion48_decode(animation_quaternion48 *source, real_quaternion *out);

    /**
     * Builds the root (node 0) world matrix for one animation frame: samples the frame's orientations, converts node 0's
     * rotation into the matrix and replaces its position with node 0's translation.
     *
     * @address 0x4d49b0
     */
    static void get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model);

    /**
     * Computes the root (node 0) translation delta between an animation frame (treated as 1 when 0) and the frame before
     * it.
     *
     * @address 0x4d4a00
     */
    static void get_frame_delta(int16_t frame, ModelAnimationsAnimation *animation, real_vector3d *out, GBXModel *model);

    /**
     * Breadth-first walk of an animation graph's node tree composing each node's local orientation with its parent's world
     * matrix. Node 0's parent is a virtual matrix built from the root position and the forward and up axes.
     *
     * @address 0x4d6880
     */
    static void nodes_build_matrices(datum_index animation_graph_tag, real_point3d *root_position, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *forward, real_vector3d *up);

};

}  // namespace halo::models
