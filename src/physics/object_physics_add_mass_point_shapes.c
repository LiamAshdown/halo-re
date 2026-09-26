// object_physics_add_mass_point_shapes  (Ghidra: FUN_00507790; renamed per
//   out/phase4/physics_types_notes.md section 5, which identifies this as an object-physics
//   mass-point routine rather than the phase2 guess "debug draw an antenna object's vertices")
// address 0x507790, size 168 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (step 1: checked against objdump -d 0x507790..0x507837)
// evidence: this module's own collision_gather_nearby_object_shapes (0x5061c0), which calls this
//   function only for a vehicle (object_type == 1) under flags bit 0x400000, exactly the
//   dispatch types/physics.h section 5 documents for the whole object_physics_* family;
//   types/physics.h object_physics_context (definition +0x04, scale +0x08) and PhysicsMassPoint
//   (mass_points.count/.pointer at +0x74/+0x78, stride 0x80, radius +0x68); the shared
//   physics_shape_vertex_to_sphere(x, y, object_index, 0xffffffff, 0, 0xff) sphere-append call this module's
//   "two provenance words" note documents.
// register convention: unaff_EBX -> context (object_physics_context *). param_1..param_3 are
//   Ghidra's own recognized parameters (x_offset forwarded straight into physics_shape_vertex_to_sphere's first
//   argument, y_offset, and an opaque short* checked for all-zero at the end -- the same
//   physics_model* "first three counts" idiom physics_model_build_from_sphere_query's own
//   param_7 uses).
//   // blam-cc: EBX -> context, stack -> x_offset, y_offset, model
// UNSURE: matrix4x3_transform_point's arguments (called with only &context->forward_i visible,
//   per unaff_EBX + 2 in the original); assumed to transform each mass point's local position
//   into world space using context's matrix, writing into a location this rewrite cannot
//   directly observe but that physics_shape_vertex_to_sphere's second argument (mass_point radius * scale +
//   y_offset) implies is not itself the sphere's x/y -- x_offset is passed through unmodified
//   as physics_shape_vertex_to_sphere's first argument on every iteration, which is surprising for a "world
//   position" and is preserved here exactly as decompiled rather than reinterpreted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
    real_matrix4x3 *m); // 0x4cbde0, blam-cc: EAX out, EDX point, stack m
extern void physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex,
    int16_t material_type, float height_offset, float radius, uint32_t object_index,
    int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index);
    // 0x503360, blam-cc: ECX model, ESI vertex, DI material_type, stack the rest

// blam-cc: EBX -> context, stack -> x_offset, y_offset, model_counts
// Adds one sphere (and, for a positive x_offset, the lowered sphere and pill) proxy per Physics
// mass point of the context's object: the mass point position through the context matrix, height
// x_offset, radius mass_point.radius * context->scale + y_offset, no surface or material.
// Returns whether the physics_model now holds any proxy (its three int16 counts).
// FIXED (step 1, objdump -d 0x507790..0x507837): the draft called physics_shape_vertex_to_sphere
// with an invented signature and dropped the transformed position.
uint8_t object_physics_add_mass_point_shapes(float x_offset, float y_offset,
    object_physics_context *context, int16_t *model_counts)
{
    Physics *definition = (Physics *)context->definition;
    int16_t i;

    for (i = 0; (int32_t)i < (int32_t)definition->mass_points.count; i++) {
        PhysicsMassPoint *mass_point = &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        real_point3d world_position;

        matrix4x3_transform_point(&world_position, (real_point3d *)&mass_point->position,
            (real_matrix4x3 *)&context->scale);
        physics_shape_vertex_to_sphere((physics_model *)model_counts, &world_position, -1, x_offset,
            mass_point->radius * context->scale + y_offset, context->object_index, -1, 0, -1);
    }

    return !(model_counts[0] == 0 && model_counts[1] == 0 && model_counts[2] == 0);
}

#if 0
Original Ghidra decompilation (0x507790):

undefined4 FUN_00507790(undefined4 param_1,float param_2,short *param_3)

{
  int iVar1;
  short sVar2;
  undefined4 *unaff_EBX;
  int iVar3;

  iVar1 = unaff_EBX[1];
  iVar3 = 0;
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 0x74)) {
    do {
      iVar1 = *(int *)(iVar1 + 0x78);
      matrix4x3_transform_point(unaff_EBX + 2);
      FUN_00503360(param_1,*(float *)(iVar3 * 0x80 + iVar1 + 0x68) * (float)unaff_EBX[2] + param_2,
                   *unaff_EBX,0xffffffff,0,0xff);
      iVar1 = unaff_EBX[1];
      sVar2 = sVar2 + 1;
      iVar3 = (int)sVar2;
    } while (iVar3 < *(int *)(iVar1 + 0x74));
  }
  if (((*param_3 == 0) && (param_3[1] == 0)) && (param_3[2] == 0)) {
    return 0;
  }
  return 1;
}
#endif
