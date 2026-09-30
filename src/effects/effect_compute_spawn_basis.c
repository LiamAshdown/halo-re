// effect_compute_spawn_basis  (Ghidra: FUN_00451930; the marker builder effect_marker_node_table_resolver calls)
// address 0x451930, size 251 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (REWRITTEN from objdump 0x451930..0x451a2a)
// objdump: EBX = the node-table context (effect_marker_callback_context: +0x00 node index, +0x04 node matrix or 0,
//   +0x10 points, +0x14 normals), AX = the entry index, stack = the output marker. out->node_index = the context
//   node. With a matrix, the entry point is inverse-transformed and the normal inverse-rotated (dotted with the
//   matrix forward/left/up); without one both are used as-is. The marker transform is
//   matrix4x3_from_forward_up(forward = normal, up = normalize(perpendicular(normal))) at the point. The draft
//   had the wrong parameter list for its only caller, which passes (index, context, out).
// DUPLICATE of src/effects/effect_marker_from_node_table.c (same address; the resolver calls that one). Kept
//   equivalent so either resolution of 0x451930 is correct.
// blam-cc: AX -> entry_index, EBX -> context, stack -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "fn_math.h"


void effect_compute_spawn_basis(int16_t entry_index, uint8_t *context, object_marker *out)
{
    real_matrix4x3 *m = *(real_matrix4x3 **)(context + 4);
    real_point3d *points = *(real_point3d **)(context + 0x10);
    real_vector3d *normals = *(real_vector3d **)(context + 0x14);
    real_point3d point;
    real_vector3d forward;
    real_vector3d up;

    out->node_index = *(int16_t *)context;
    if (m != 0) {
        real_vector3d *v = &normals[entry_index];

        matrix4x3_inverse_transform_point(m, &point, &points[entry_index]);
        forward.i = v->k * m->forward.k + v->j * m->forward.j + v->i * m->forward.i;
        forward.j = v->k * m->left.k + v->j * m->left.j + v->i * m->left.i;
        forward.k = v->k * m->up.k + v->j * m->up.j + v->i * m->up.i;
    } else {
        point = points[entry_index];
        forward = normals[entry_index];
    }
    vector3d_build_perpendicular(&up, &forward);
    vector3d_normalize_with_length(&up);
    matrix4x3_from_forward_up(&up, &forward, &out->transform);
    out->transform.position = point;
}

#if 0
Original Ghidra decompilation (0x451930):

void FUN_00451930(undefined2 *param_1)

{
  short in_AX;
  undefined4 *puVar1;
  undefined2 *unaff_EBX;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;

  *param_1 = *unaff_EBX;
  if (*(int *)(unaff_EBX + 2) == 0) {
    puVar1 = (undefined4 *)(*(int *)(unaff_EBX + 8) + in_AX * 0xc);
    local_18 = *puVar1;
    local_14 = puVar1[1];
    local_10 = puVar1[2];
  }
  else {
    matrix4x3_inverse_transform_point();
  }
  vector3d_build_perpendicular();
  vector3d_normalize_with_length();
  matrix4x3_from_forward_up(param_1 + 2);
  *(undefined4 *)(param_1 + 0x16) = local_18;
  *(undefined4 *)(param_1 + 0x18) = local_14;
  *(undefined4 *)(param_1 + 0x1a) = local_10;
  return;
}
#endif
