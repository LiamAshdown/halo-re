// object_physics_context_build  (Ghidra: FUN_005074b0; renamed)
// address 0x5074b0, size 220 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x5074b0..0x50758b (EBX object, EAX context; cross EAX=left ECX=forward stack=up; -COM through a temporary).)
// evidence: types/physics.h object_physics_context struct comment, written specifically about
//   this function: "0x005074b0 fails when the Object tag has no physics reference, that is when
//   tag offset 0x8c is -1. The matrix is the model space to world transform with the scale
//   forced to 1.0, built from object_get_position and object_get_orientation plus a cross
//   product for the left column, and its translation is shifted by the centre of mass of the
//   definition." Section 7's own "Unresolved" entry for this function is carried into the
//   UNSURE note below verbatim, since this rewrite could not improve on it.
// register convention: unaff_EBX -> object_index, in_EAX -> out_context
//   (object_physics_context *). object_get_position, object_get_orientation's out_forward and
//   matrix4x3_transform_point's point/out arguments are never visible in this function's own
//   body at all; Ghidra shows only object_get_orientation's out_up and matrix4x3_transform_point's
//   matrix pointer.
//   // blam-cc: EBX -> object_index, EAX -> out_context
// UNSURE (from types/physics.h section 7, "object_physics_context translation (0x30..0x3b)"):
//   "0x5074b0 builds it from object_get_position and then runs matrix4x3_transform_point over
//   the negated Physics.center_of_mass, and Ghidra attributes the three negation stores to the
//   matrix translation slot itself. The net effect (a model-space-to-world matrix) is certain
//   because every consumer uses it that way; the exact instruction order is not." This rewrite
//   preserves the literal sequence Ghidra shows: object_get_position, object_get_orientation,
//   the cross product for the left column, matrix4x3_transform_point over the matrix with
//   whatever hidden point/out arguments it already had, and then an explicit store of
//   -center_of_mass into position_x/y/z.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900; EAX out, ECX object
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up); // 0x4f6970
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0; EAX out, ECX a, stack b
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
    real_matrix4x3 *m); // 0x4cbde0, math module (src/math/matrix4x3_transform_point.c)
    // blam-cc: only m is on the stack; out and point arrive in registers Ghidra loses at every
    //          call site in this module

// Builds an object_physics_context for object_index. Fails (returns 0) when the Object tag has
// no physics reference (tag offset 0x8c == -1); otherwise fills object_index, definition (the
// Physics tag data), scale (forced to 1.0), the (forward, left, up) orientation basis (left via
// cross product), and a translation built from the object's position and the Physics
// definition's centre of mass (see UNSURE above), returning 1.
uint8_t object_physics_context_build(uint32_t object_index, object_physics_context *out_context)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    void *object_tag_data = tag_instances[obj->definition_tag & 0xffff].data;
    int32_t physics_tag_id = *(int32_t *)((uint8_t *)object_tag_data + 0x8c);
    void *physics_definition;

    if (physics_tag_id == -1) {
        return 0;
    }

    out_context->object_index = object_index;
    physics_definition = tag_instances[(uint16_t)physics_tag_id].data;
    out_context->definition = physics_definition;
    out_context->scale = 1.0f;

    // REWRITTEN (objdump 0x5074b0..0x507572, 2026-09-26): position, then forward/up, then left = forward x up,
    //   then the translation = M * (-centre of mass), computed into a temporary and copied (the matrix being applied
    //   contains the translation itself). The draft swapped object_get_position's arguments, dropped the forward
    //   output of object_get_orientation and both inputs of the cross product, and transformed in place.
    object_get_position((real_point3d *)&out_context->position_x, object_index);
    object_get_orientation((real_vector3d *)&out_context->forward_i, object_index, (real_vector3d *)&out_context->up_i);
    vector3d_cross_product((real_vector3d *)&out_context->left_i, (const real_vector3d *)&out_context->forward_i,
                           (const real_vector3d *)&out_context->up_i);
    {
        real_point3d point;
        point.x = -*(float *)((uint8_t *)physics_definition + 0x0c);
        point.y = -*(float *)((uint8_t *)physics_definition + 0x10);
        point.z = -*(float *)((uint8_t *)physics_definition + 0x14);
        matrix4x3_transform_point(&point, &point, (real_matrix4x3 *)&out_context->scale);
        out_context->position_x = point.x;
        out_context->position_y = point.y;
        out_context->position_z = point.z;
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x5074b0):

uint FUN_005074b0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  uint uVar4;
  int iVar5;
  uint *in_EAX;
  uint unaff_EBX;

  iVar5 = DAT_0087bc14;
  uVar4 = *(uint *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc) &
                    0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(uVar4 + 0x8c) != -1) {
    *in_EAX = unaff_EBX;
    in_EAX[1] = *(uint *)((*(uint *)(uVar4 + 0x8c) & 0xffff) * 0x20 + 0x14 + iVar5);
    in_EAX[2] = 0x3f800000;
    object_get_position();
    object_get_orientation(in_EAX + 9);
    vector3d_cross_product(in_EAX + 9);
    uVar4 = in_EAX[1];
    fVar1 = *(float *)(uVar4 + 0xc);
    fVar2 = *(float *)(uVar4 + 0x10);
    fVar3 = *(float *)(uVar4 + 0x14);
    matrix4x3_transform_point(in_EAX + 2);
    in_EAX[0xc] = (uint)-fVar1;
    in_EAX[0xd] = (uint)-fVar2;
    in_EAX[0xe] = (uint)-fVar3;
    return CONCAT31((int3)((uint)-fVar2 >> 8),1);
  }
  return uVar4 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
