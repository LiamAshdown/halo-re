// object_physics_test_point_against_mass_points  (Ghidra: FUN_00507590; renamed per
//   out/phase4/physics_types_notes.md section 5: "resolve the object Physics tag ... iterate
//   Physics.mass_points, never the antenna widget", correcting phase2's antenna_* guess)
// address 0x507590, size 124 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: types/physics.h object_physics_context (definition at +0x04) and PhysicsMassPoint
//   (mass_points.count/.pointer at Physics +0x74/+0x78, stride 0x80, position +0x38, radius
//   +0x68) -- object_physics_context and antenna_test_ray_against_vertex_spheres (0x507610,
//   this module, higher half) both index mass_points with exactly these offsets per
//   types/physics.h's own struct comment.
// register convention: unaff_EDI -> context (object_physics_context *). Ghidra could not
//   recover matrix4x3_inverse_transform_point's arguments at all; this rewrite assumes the
//   conventional (matrix, world_point, out_local_point) shape, with matrix = &context->scale
//   (matching object_physics_context_build's own matrix4x3_transform_point usage) and
//   world_point coming from wherever this function's own (also hidden) point parameter lives.
//   Confirmed by objdump 0x50759b (`mov esi,eax` right before the call, matching that callee's
//   own ESI -> point convention): world_point arrives in EAX.
//   // blam-cc: EAX -> world_point, EDI -> context, stack -> out_index
// FIXED (register inputs, objdump): EAX is a genuine live-in (forwarded as world_point) that the
// notes did not map.
// UNSURE (major): the loop's early-return path encodes its result as CONCAT31(garbage, 1) in
//   Ghidra's decompile -- a classic decompiler artifact for a function that returns a bool in AL
//   with uninitialized upper register bytes, not a meaningful packed value. This rewrite returns
//   a plain uint8_t found/not-found flag and adds an out_index parameter for "the hit index" per
//   out/phase4/physics_functions.md's own summary, but neither the out-parameter's existence nor
//   its exact register is confirmed by the decompile.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out,
    real_point3d *point); // 0x4cbf80, math module (src/math/matrix4x3_inverse_transform_point.c)
    // blam-cc: ECX -> m, EDX -> out, ESI -> point

// Transforms world_point into context's object-local space and tests it against every mass
// point's collision sphere (PhysicsMassPoint.position/.radius). Returns 1 and (best-effort)
// *out_index the first mass point whose sphere contains the local point; 0 if none do.
uint8_t object_physics_test_point_against_mass_points(object_physics_context *context,
    real_point3d *world_point, int16_t *out_index)
{
    Physics *definition = (Physics *)context->definition;
    real_point3d local_point;
    int32_t count = definition->mass_points.count;
    int16_t i;

    matrix4x3_inverse_transform_point((real_matrix4x3 *)&context->scale, &local_point, world_point);

    for (i = 0; i < count; i++) {
        PhysicsMassPoint *mass_point =
            &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        float dx = mass_point->position.x - local_point.x;
        float dy = mass_point->position.y - local_point.y;
        float dz = mass_point->position.z - local_point.z;
        float radius = mass_point->radius;

        if (dx * dx + dy * dy + dz * dz <= radius * radius) {
            if (out_index) {
                *out_index = i;
            }
            return 1;
        }
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x507590):

uint FUN_00507590(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  uint uVar7;
  int iVar8;
  short sVar9;
  int unaff_EDI;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  uVar7 = matrix4x3_inverse_transform_point();
  iVar2 = *(int *)(*(int *)(unaff_EDI + 4) + 0x74);
  sVar9 = 0;
  if (0 < iVar2) {
    iVar3 = *(int *)(*(int *)(unaff_EDI + 4) + 0x78);
    uVar7 = 0;
    do {
      fVar1 = *(float *)(uVar7 * 0x80 + 0x68 + iVar3);
      iVar8 = uVar7 * 0x80 + iVar3;
      fVar5 = *(float *)(iVar8 + 0x38) - local_c;
      fVar6 = *(float *)(iVar8 + 0x3c) - local_8;
      fVar4 = *(float *)(iVar8 + 0x40) - local_4;
      fVar4 = fVar6 * fVar6 + fVar5 * fVar5 + fVar4 * fVar4;
      fVar1 = fVar1 * fVar1;
      if (fVar1 < fVar4 == 0) {
        return CONCAT31((int3)(CONCAT22((short)((uint)iVar8 >> 0x10),
                                        (ushort)(fVar1 < fVar4) << 8 |
                                        (ushort)(NAN(fVar1) || NAN(fVar4)) << 10 |
                                        (ushort)(fVar1 == fVar4) << 0xe) >> 8),1);
      }
      sVar9 = sVar9 + 1;
      uVar7 = (uint)sVar9;
    } while ((int)uVar7 < iVar2);
  }
  return uVar7 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
