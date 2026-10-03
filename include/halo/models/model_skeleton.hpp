/**
 * @file include/halo/models/model_skeleton.hpp
 * Model node transforms: bind pose, blending, world matrices and two-bone IK.
 */
#pragma once

#include "halo/models/models_types.hpp"

namespace halo::models {

/**
 * Stateless operations on the node hierarchy of a model: orientation blending, world matrix construction and inverse
 * kinematics.
 */
struct model_skeleton {
    /**
     * Blends the other orientations into in_out node by node with weight (step + 1) / steps: rotation by slerp,
     * translation and scale linearly.
     *
     * @address 0x4d69e0
     */
    static void blend_transforms(real_orientation *in_out, int16_t node_count, real_orientation *other, int16_t step, int16_t steps);

    /**
     * Breadth-first walk of the model's node tree composing each node's local orientation into a world matrix, with a
     * virtual root parent built from the root position and forward and up axes.
     *
     * @address 0x4d7690
     */
    static void build_matrices(real_point3d *root_position, real_vector3d *forward, GBXModel *model, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *up);

    /**
     * Solves a two-bone inverse kinematics chain toward the target: re-aims the root and middle joint rotations, moves the
     * middle joint along the root's forward axis and writes the effector matrix.
     *
     * @address 0x4d6440
     */
    static void ik_solve_two_bone(real_matrix4x3 *target, real_matrix4x3 *middle, real_matrix4x3 *end, real_matrix4x3 *out_end);

};

}  // namespace halo::models
