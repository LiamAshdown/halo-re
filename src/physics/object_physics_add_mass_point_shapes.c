// object_physics_add_mass_point_shapes  (Ghidra: FUN_00507790; renamed per
//   out/phase4/physics_types_notes.md section 5, which identifies this as an object-physics
//   mass-point routine rather than the phase2 guess "debug draw an antenna object's vertices")
// address 0x507790, size 168 bytes
// name confidence: 0.4   rewrite confidence: 0.35
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
    real_matrix4x3 *m); // 0x4cbde0, math module (src/math/matrix4x3_transform_point.c)
    // blam-cc: only m is on the stack; out and point arrive in registers Ghidra loses at every
    //          call site in this module
extern void physics_shape_vertex_to_sphere(float center_x, float center_y, uint32_t object_index,
    uint32_t surface_index, int32_t margin, uint32_t flags); // 0x503360, this module (higher half)

// Appends one sphere proxy per Physics mass point of context's object into the physics_model,
// each at (x_offset, mass_point.radius * context->scale + y_offset), using context->object_index
// as the sphere's provenance (matching physics_shape_vertex_to_sphere's world/object convention). Returns whether
// *model ended up with anything in it (its first three int16 counts, checked directly since
// this function receives model only as an opaque short* the way physics_model_build_from_sphere_query
// receives its own equivalent parameter).
uint8_t object_physics_add_mass_point_shapes(float x_offset, float y_offset,
    object_physics_context *context, int16_t *model_counts)
{
    Physics *definition = (Physics *)context->definition;
    int32_t count = definition->mass_points.count;
    int32_t i;

    for (i = 0; i < count; i++) {
        PhysicsMassPoint *mass_point =
            &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        real_point3d world_position;

        // Ghidra's single visible argument is unaff_EBX + 2, i.e. &context->scale -- the base of
        // the embedded matrix4x3, NOT &context->forward_i (an earlier rewrite was one field off).
        // out and point are register arguments; reconstructed as the mass point definition
        // position transformed into world space, matching
        // object_physics_compute_mass_point_forces own visible three-argument call.
        matrix4x3_transform_point(&world_position, (real_point3d *)&mass_point->position,
            (real_matrix4x3 *)&context->scale); // UNSURE: out and point are register arguments
        physics_shape_vertex_to_sphere(x_offset, mass_point->radius * context->scale + y_offset,
            context->object_index, 0xffffffff, 0, 0xff);
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
