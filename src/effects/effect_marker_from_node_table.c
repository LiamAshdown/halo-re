// effect_marker_from_node_table  (not a Ghidra function; builds one object_marker from the effect node table)
// address 0x451930, size 251 bytes
// name confidence: 0.5  rewrite confidence: 0.9
// objdump 0x451930..0x451a2a: the marker's node index is the context's (+0x00); with a node matrix (+0x04) the entry's
//   point (+0x10, 12 bytes each) and normal (+0x14) are taken into node space (matrix4x3_inverse_transform_point and
//   the transposed rotation), otherwise used as they are; the transform is built from the normal as forward and a
//   normalized perpendicular as up (matrix4x3_from_forward_up) with the point as its position.
// blam-cc: AX -> entry_index, EBX -> context, stack -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_math.h"
#include "fn_effects.h"


void effect_marker_from_node_table(int16_t entry_index, uint8_t *context, object_marker *out)
{
    real_matrix4x3 *node = *(real_matrix4x3 **)(context + 4);
    real_point3d *point = (real_point3d *)(*(uint8_t **)(context + 0x10) + entry_index * 0xc);
    real_vector3d *normal = (real_vector3d *)(*(uint8_t **)(context + 0x14) + entry_index * 0xc);
    real_point3d position;
    real_vector3d forward;
    real_vector3d up;

    *(uint16_t *)out = *(uint16_t *)context;
    if (node != 0) {
        matrix4x3_inverse_transform_point(node, &position, point);
        forward.i = normal->k * node->forward.k + normal->j * node->forward.j + normal->i * node->forward.i;
        forward.j = normal->k * node->left.k + normal->j * node->left.j + normal->i * node->left.i;
        forward.k = normal->k * node->up.k + normal->j * node->up.j + normal->i * node->up.i;
    } else {
        position = *point;
        forward = *normal;
    }
    vector3d_build_perpendicular(&up, &forward);
    vector3d_normalize_with_length(&up);
    matrix4x3_from_forward_up(&up, &forward, (real_matrix4x3 *)((uint8_t *)out + 4));
    ((real_matrix4x3 *)((uint8_t *)out + 4))->position = position;
}
