/**
 * @file src/models/model_skeleton.cpp
 * Model node transforms: bind pose, blending, world matrices and two-bone IK.
 * The original author notes and decompiles are in docs/original/models/.
 */

#include "halo/models/models.hpp"
#include "halo/math/api.hpp"

extern "C" {
extern double sqrt(double x);
}

namespace halo::models {

void model_skeleton::blend_transforms(real_orientation *in_out, int16_t node_count, real_orientation *other, int16_t step, int16_t steps)
{
    real weight, one_minus_weight;
    int16_t node;

    weight = (real)(step + 1) / (real)steps;
    one_minus_weight = 1.0f - weight;

    for (node = 0; node < node_count; node++) {
        in_out[node].scale = weight * in_out[node].scale + one_minus_weight * other[node].scale;

        halo::math::quaternion_lerp(in_out[node].rotation, other[node].rotation, in_out[node].rotation, weight);
        halo::math::quaternion_normalize(in_out[node].rotation);

        in_out[node].translation.x = weight * in_out[node].translation.x +
                                      one_minus_weight * other[node].translation.x;
        in_out[node].translation.y = weight * in_out[node].translation.y +
                                      one_minus_weight * other[node].translation.y;
        in_out[node].translation.z = weight * in_out[node].translation.z +
                                      one_minus_weight * other[node].translation.z;
    }
}

void model_skeleton::build_matrices(real_point3d *root_position, real_vector3d *forward, GBXModel *model, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *up)
{
    real_matrix4x3 root_parent;
    int16_t queue[k_maximum_nodes_per_model];
    int16_t read_index, write_index;

    halo::math::matrix4x3_from_forward_up(*up, *forward, root_parent);
    root_parent.position = *root_position;

    if (model->nodes.count <= 0) {
        return;
    }

    read_index = 0;
    write_index = 1;
    queue[0] = 0;
    do {
        ModelNode *node_def;
        real_matrix4x3 *parent_matrix;
        real_matrix4x3 local_matrix;
        int16_t node;

        node = queue[read_index];
        read_index = read_index + 1;
        node_def = (ModelNode *)((uint8_t *)model->nodes.pointer + node * sizeof(ModelNode));

        parent_matrix = (node == 0) ? &root_parent : &out_matrices[(int16_t)node_def->parent_node_index];

        halo::math::matrix4x3_from_quaternion(orientations[node].rotation, local_matrix);
        local_matrix.scale = orientations[node].scale;
        local_matrix.position = orientations[node].translation;

        halo::math::globals().matrix4x3_multiply_procedure(parent_matrix, &local_matrix, &out_matrices[node]);

        if (node_def->next_sibling_node_index != 0xffff) {
            queue[write_index] = (int16_t)node_def->next_sibling_node_index;
            write_index = write_index + 1;
        }
        if (node_def->first_child_node_index != 0xffff) {
            queue[write_index] = (int16_t)node_def->first_child_node_index;
            write_index = write_index + 1;
        }
    } while (read_index != write_index);
}

void model_skeleton::ik_solve_two_bone(real_matrix4x3 *target, real_matrix4x3 *middle, real_matrix4x3 *end, real_matrix4x3 *out_end)
{
    real bone_end_middle;
    real reach_beyond_end;
    real middle_target_dist;
    real dir_x, dir_y, dir_z;
    real pole_x, pole_y, pole_z;
    real max_reach;
    real projected, remaining, perp;

    {
        real dx = end->position.x - middle->position.x;
        real dy = end->position.y - middle->position.y;
        real dz = end->position.z - middle->position.z;
        bone_end_middle = (real)sqrt((double)(dx * dx + dz * dz + dy * dy));
    }
    {
        real dx = out_end->position.x - end->position.x;
        real dy = out_end->position.y - end->position.y;
        real dz = out_end->position.z - end->position.z;
        reach_beyond_end = (real)sqrt((double)(dx * dx + dy * dy + dz * dz));
    }
    {
        real dx = middle->position.x - target->position.x;
        real dy = middle->position.y - target->position.y;
        real dz = middle->position.z - target->position.z;
        middle_target_dist = (real)sqrt((double)(dx * dx + dy * dy + dz * dz));
    }

    {
        real inv = 1.0f / middle_target_dist;
        real ex = end->position.x - middle->position.x;
        real ey = end->position.y - middle->position.y;
        real ez = end->position.z - middle->position.z;
        real_vector3d pole;

        dir_x = (target->position.x - middle->position.x) * inv;
        dir_y = (target->position.y - middle->position.y) * inv;
        dir_z = (target->position.z - middle->position.z) * inv;

        pole.i = dir_y * ez - dir_z * ey;
        pole.j = dir_z * ex - dir_x * ez;
        pole.k = dir_x * ey - dir_y * ex;
        halo::math::vector3d_normalize_with_length(pole);
        pole_x = pole.i;
        pole_y = pole.j;
        pole_z = pole.k;
    }

    max_reach = (reach_beyond_end + bone_end_middle) * 0.98f;
    if (max_reach < middle_target_dist) {
        target->position.x = dir_x * max_reach + middle->position.x;
        target->position.y = dir_y * max_reach + middle->position.y;
        target->position.z = dir_z * max_reach + middle->position.z;
        middle_target_dist = max_reach;
    }

    projected = ((middle_target_dist * middle_target_dist + bone_end_middle * bone_end_middle) -
                 reach_beyond_end * reach_beyond_end) / (middle_target_dist + middle_target_dist);
    remaining = middle_target_dist - projected;
    perp = (real)sqrt((double)(bone_end_middle * bone_end_middle - projected * projected));

    {
        real side_x = pole_y * dir_z - pole_z * dir_y;
        real side_y = pole_z * dir_x - pole_x * dir_z;
        real side_z = pole_x * dir_y - pole_y * dir_x;

        middle->forward.i = projected * dir_x + side_x * perp;
        middle->forward.j = projected * dir_y + side_y * perp;
        middle->forward.k = projected * dir_z + side_z * perp;
        halo::math::vector3d_normalize_with_length(middle->forward);

        middle->up.i = middle->left.k * middle->forward.j - middle->left.j * middle->forward.k;
        middle->up.j = middle->left.i * middle->forward.k - middle->left.k * middle->forward.i;
        middle->up.k = middle->forward.i * middle->left.j - middle->left.i * middle->forward.j;
        halo::math::vector3d_normalize_with_length(middle->up);

        middle->left.i = middle->up.j * middle->forward.k - middle->up.k * middle->forward.j;
        middle->left.j = middle->up.k * middle->forward.i - middle->up.i * middle->forward.k;
        middle->left.k = middle->up.i * middle->forward.j - middle->up.j * middle->forward.i;

        end->forward.i = remaining * dir_x - side_x * perp;
        end->forward.j = remaining * dir_y - side_y * perp;
        end->forward.k = remaining * dir_z - side_z * perp;
        halo::math::vector3d_normalize_with_length(end->forward);

        end->up.i = end->left.k * end->forward.j - end->left.j * end->forward.k;
        end->up.j = end->left.i * end->forward.k - end->left.k * end->forward.i;
        end->up.k = end->forward.i * end->left.j - end->left.i * end->forward.j;
        halo::math::vector3d_normalize_with_length(end->up);

        end->left.i = end->up.j * end->forward.k - end->up.k * end->forward.j;
        end->left.j = end->up.k * end->forward.i - end->up.i * end->forward.k;
        end->left.k = end->up.i * end->forward.j - end->up.j * end->forward.i;

        end->position.x = bone_end_middle * middle->forward.i + middle->position.x;
        end->position.y = bone_end_middle * middle->forward.j + middle->position.y;
        end->position.z = bone_end_middle * middle->forward.k + middle->position.z;
    }

    *out_end = *target;
}

}  // namespace halo::models
