// biped_ground_adjust_apply_node_rotations  (Ghidra: biped_ground_adjust_apply_node_rotations, renamed)
// address 0x558a20, size 787 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x558a20..0x558d31 (the draft called the rotations with no vector, crossed
//   with a NULL operand and called the CRT acos at its original address).
// blam-cc: EAX -> object_index, stack -> nodes, saved_positions (biped_ground_adjust_step 0x557b4f)
// For every non-root animation graph node (0x40 each, parent +0x24) whose parent joint flags (+0x28)
//   lack bit 2: S = saved node - saved parent and C = current node - current parent (node matrices
//   0x34 each, position +0x28), both normalized; axis = normalize(S x C). When S.C is not ~1 and the
//   angle acos(S.C) is not ~0 and below pi/4, the parent basis is made orthonormal if needed
//   (real_matrix4x3_rotation_is_orthonormal / _rebuild_orthonormal), its forward and up are rotated
//   about the axis (vector3d_rotate_about_axis, sin angle, S.C; EDX still holds S.C for the second
//   call), both normalized, left = cross(forward, up) normalized, and orthonormality restored again.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX, ECX, stack
extern uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up); // 0x5579e0
extern void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up); // 0x558860
extern double acos(double x);
extern double sin(double x);
extern double fabs(double x);

static void biped_ground_adjust_make_orthonormal(real_matrix4x3 *m)
{
    if (!real_matrix4x3_rotation_is_orthonormal(&m->forward, &m->left, &m->up)) {
        real_matrix4x3_rotation_rebuild_orthonormal(&m->forward, &m->left, &m->up);
    }
}

void biped_ground_adjust_apply_node_rotations(uint32_t object_index, real_matrix4x3 *nodes,
                                               real_point3d *saved_positions)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)&((struct Object *)object_tag)->animation_graph.tag_id & 0xffff].data;
    int32_t i;

    for (i = 0; i < *(int32_t *)&((ModelAnimations *)graph)->nodes.count; i++) {
        uint8_t *graph_nodes = *(uint8_t **)&((ModelAnimations *)graph)->nodes.pointer;
        int16_t parent_index;
        real_vector3d saved;
        real_vector3d current;
        real_vector3d axis;
        float cosine;
        float angle;

        if (i == 0) {
            continue;
        }
        parent_index = *(int16_t *)(graph_nodes + i * 0x40 + 0x24);
        if (graph_nodes[parent_index * 0x40 + 0x28] & 4) {
            continue;
        }
        saved.i = saved_positions[i].x - saved_positions[parent_index].x;
        saved.j = saved_positions[i].y - saved_positions[parent_index].y;
        saved.k = saved_positions[i].z - saved_positions[parent_index].z;
        current.i = nodes[i].position.x - nodes[parent_index].position.x;
        current.j = nodes[i].position.y - nodes[parent_index].position.y;
        current.k = nodes[i].position.z - nodes[parent_index].position.z;
        vector3d_normalize_with_length(&saved);
        vector3d_normalize_with_length(&current);
        axis.i = current.k * saved.j - current.j * saved.k;
        axis.j = current.i * saved.k - current.k * saved.i;
        axis.k = current.j * saved.i - saved.j * current.i;
        vector3d_normalize_with_length(&axis);
        cosine = current.k * saved.k + current.j * saved.j + current.i * saved.i;
        if (fabs((double)(cosine - 1.0f)) < 9.999999747378752e-05) {
            continue;
        }
        angle = (float)acos((double)cosine);
        if (fabs((double)angle) < 9.999999747378752e-05 || !(fabs((double)angle) < 0.7853981852531433)) {
            continue;
        }
        {
            real_matrix4x3 *parent = &nodes[parent_index];
            real sine;

            biped_ground_adjust_make_orthonormal(parent);
            sine = (real)sin((double)angle);
            vector3d_rotate_about_axis(&parent->forward, &axis, sine, cosine);
            vector3d_rotate_about_axis(&parent->up, &axis, sine, cosine);
            vector3d_normalize_with_length(&parent->forward);
            vector3d_normalize_with_length(&parent->up);
            vector3d_cross_product(&parent->left, &parent->forward, &parent->up);
            vector3d_normalize_with_length(&parent->left);
            biped_ground_adjust_make_orthonormal(parent);
        }
    }
}
