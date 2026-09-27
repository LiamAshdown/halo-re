// biped_ground_adjust_solve_node  (Ghidra: biped_ground_adjust_solve_node, renamed)
// address 0x557b80, size 1143 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// REWRITTEN from objdump 0x557b80..0x557ff6 (the draft crossed with a NULL operand, left the matrix
//   calls unprototyped and read the wrong vectors).
// blam-cc: EAX -> object_index, EBX -> reference_position (biped_ground_adjust_solve's slide result,
//   0x558264), stack -> node_index, nodes, own_position, success_bits
// Tries to move a limp body node (own_position) to the fitted reference position under its parent
//   joint limits (animation graph nodes, 0x40 each: parent +0x24, joint flags +0x28, base vector +0x2c,
//   vector range +0x38; node matrices 0x34 each, forward +0x04, up +0x1c, position +0x28):
//   - with a parent other than the root whose flags lack bit 2 (no movement): P = node - parent and
//     R = reference - parent, both normalized; axis = normalize(cross(R, P)); when P.R is not ~1 the
//     axis is taken into parent space (inverse parent matrix) and the parent forward into grandparent
//     space (inverse grandparent matrix);
//       hinge (flag bit 1): the reference is projected onto the plane through the parent with the
//       parent up as normal; when that direction in parent space is not ~the base vector the node is
//       marked, and moved there when it lies no higher than the node and is not embedded
//       (physics_point_refresh_leaf, tolerance); either way it counts as updated;
//       otherwise: the forward rotated about the axis by the P/R angle must differ from the base vector
//       but stay within the vector range of it, and the reference must be lower than the node; then the
//       node is marked and moved to the reference.
//   - when not updated (or the parent cannot move), a node whose parent is marked and that is above
//     the reference is marked and moved to the reference.
//   Returns whether it updated.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;    // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern void plane3d_from_point_and_normal(real_plane3d *out, const real_vector3d *normal, const real_point3d *point); // 0x44d9e0, stack, ECX, EDX
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, EAX, ECX
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cbe50, EAX, EDX, stack
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX, ECX, stack
extern uint8_t physics_point_refresh_leaf(real_point3d *point, float radius); // 0x505540, EDX point, stack radius
extern double acos(double x); // 0x628140
extern double sin(double x);  // fsin
extern double fabs(double x);

static void biped_ground_adjust_mark(uint32_t *success_bits, int32_t node_index)
{
    success_bits[node_index >> 5] |= 1u << (node_index & 0x1f);
}

static int biped_ground_adjust_is_one(float value)
{
    return fabs((double)(value - 1.0f)) < 9.999999747378752e-05;
}

char biped_ground_adjust_solve_node(uint32_t object_index, real_point3d *reference_position,
                                     int32_t node_index, real_matrix4x3 *nodes,
                                     real_point3d *own_position, uint32_t *success_bits)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)(object_tag + 0x44) & 0xffff].data;
    uint8_t *graph_nodes = *(uint8_t **)(graph + 0x6c);
    uint8_t *self_node = graph_nodes + node_index * 0x40;
    int16_t parent_index = *(int16_t *)(self_node + 0x24);
    uint8_t *parent_node = graph_nodes + parent_index * 0x40;
    float tolerance = *(float *)(graph + 0x60);
    char updated = 0;

    if (fabs((double)tolerance) < 9.999999747378752e-05 || tolerance < 0.0f || tolerance > 0.07f) {
        tolerance = 0.03f;
    }

    if (parent_index != 0 && (parent_node[0x28] & 4) == 0) {
        real_matrix4x3 *parent_matrix = &nodes[parent_index];
        real_point3d *parent_position = &parent_matrix->position;
        real_point3d *self_position = &nodes[node_index].position;
        real_vector3d to_self;
        real_vector3d to_reference;
        real_vector3d axis;
        float cosine;

        to_self.i = self_position->x - parent_position->x;
        to_self.j = self_position->y - parent_position->y;
        to_self.k = self_position->z - parent_position->z;
        to_reference.i = reference_position->x - parent_position->x;
        to_reference.j = reference_position->y - parent_position->y;
        to_reference.k = reference_position->z - parent_position->z;
        vector3d_normalize_with_length(&to_self);
        vector3d_normalize_with_length(&to_reference);
        vector3d_cross_product(&axis, &to_reference, &to_self);
        vector3d_normalize_with_length(&axis);
        cosine = to_reference.k * to_self.k + to_reference.j * to_self.j + to_reference.i * to_self.i;

        if (!biped_ground_adjust_is_one(cosine)) {
            float angle = (float)acos((double)cosine);
            float *base = (float *)(parent_node + 0x2c);
            real_matrix4x3 parent_inverse;
            real_matrix4x3 grandparent_inverse;
            real_vector3d local_axis;
            real_vector3d local_forward;

            matrix4x3_inverse(&parent_inverse, parent_matrix);
            matrix4x3_inverse(&grandparent_inverse, &nodes[*(int16_t *)(parent_node + 0x24)]);
            matrix4x3_transform_vector(&local_axis, &axis, &parent_inverse);
            local_forward = parent_matrix->forward;
            matrix4x3_transform_vector(&local_forward, &local_forward, &grandparent_inverse);

            if (parent_node[0x28] & 2) {
                real_vector3d up = parent_matrix->up;
                real_plane3d plane;
                real_point3d projected;
                real_vector3d direction;
                real_vector3d local_direction;
                float distance;

                plane3d_from_point_and_normal(&plane, &up, parent_position);
                distance = (plane.normal.j * reference_position->y + plane.normal.i * reference_position->x +
                    plane.normal.k * reference_position->z - plane.d) * -1.0f;
                projected.x = up.i * distance + reference_position->x;
                projected.y = up.j * distance + reference_position->y;
                projected.z = up.k * distance + reference_position->z;
                direction.i = projected.x - parent_position->x;
                direction.j = projected.y - parent_position->y;
                direction.k = projected.z - parent_position->z;
                vector3d_normalize_with_length(&direction);
                matrix4x3_transform_vector(&local_direction, &direction, &parent_inverse);
                if (!biped_ground_adjust_is_one(local_direction.j * base[1] + local_direction.k * base[2] +
                        local_direction.i * base[0])) {
                    biped_ground_adjust_mark(success_bits, node_index);
                    if (!(own_position->z < projected.z) && !physics_point_refresh_leaf(&projected, tolerance)) {
                        *own_position = projected;
                    }
                    updated = 1;
                }
            } else {
                float alignment;

                vector3d_rotate_about_axis(&local_forward, &local_axis, (real)sin((double)angle), cosine);
                alignment = local_forward.j * base[1] + local_forward.k * base[2] + local_forward.i * base[0];
                if (!biped_ground_adjust_is_one(alignment) &&
                    *(float *)(parent_node + 0x38) > (float)fabs(acos((double)alignment)) &&
                    own_position->z > reference_position->z) {
                    biped_ground_adjust_mark(success_bits, node_index);
                    *own_position = *reference_position;
                    updated = 1;
                }
            }
        }
    }

    if ((parent_node[0x28] & 4) == 0 && updated) {
        return updated;
    }
    if ((success_bits[parent_index >> 5] & (1u << (parent_index & 0x1f))) == 0) {
        return updated;
    }
    if (!(own_position->z > reference_position->z)) {
        return updated;
    }
    biped_ground_adjust_mark(success_bits, node_index);
    *own_position = *reference_position;
    return 1;
}
