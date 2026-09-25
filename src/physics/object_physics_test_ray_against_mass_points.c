// object_physics_test_ray_against_mass_points  (Ghidra: antenna_test_ray_against_vertex_spheres;
//   renamed per out/phase4/physics_types_notes.md section 5, which gives this exact new name)
// address 0x507610, size 374 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: types/physics.h object_physics_ray_result (t, plane_i/j/k/d) and
//   object_physics_context (definition +0x04, the matrix at +0x08, position at +0x30); this
//   module's own object_physics_context_build (0x5074b0) and
//   object_physics_test_point_against_mass_points (0x507590) establish the same
//   mass_points.count/.pointer (Physics +0x74/+0x78, stride 0x80, position +0x38, radius +0x68)
//   pattern; ray_intersects_sphere's own decompile (out/phase2 pack for 0x4ce3a0, fully
//   resolved) supplies its (sphere_center, radius) explicit pair plus (ray_origin, ray_direction,
//   out_normal, out_t) hidden EAX/EDX/ECX/EDI arguments, which is what fixes the meaning of the
//   otherwise-unwritten local_18/14/10 (local ray origin) and local_24/20/1c (local ray
//   direction) locals here.
// register convention: unaff_EBX -> out_result (object_physics_ray_result *); param_1 is
//   Ghidra's own recognized parameter, confirmed as object_physics_context * by its +0x04/+0x08/
//   +0x30 field uses matching that struct exactly. The ray itself (world-space origin and
//   direction) is never visible anywhere in this function's body -- not as a named parameter,
//   not as an unaff_* register -- so this rewrite adds it as two more hidden-register
//   parameters per the EAX/ECX/EDX/EBX/ESI/EDI ordering, before param_1's own stack slot.
//   // blam-cc: EAX -> world_origin, EBX -> out_result, stack -> context, world_direction
// UNSURE (major): which registers actually carry world_origin/world_direction; nothing in this
//   function's own decompile confirms it, only the fact that ray_intersects_sphere plainly needs
//   them and nothing else in this function could supply them.
// UNSURE: matrix4x3_inverse_transform_point's arguments (called with zero visible arguments,
//   immediately before matrix4x3_inverse_transform_vector's explicit matrix argument); assumed
//   to use the same matrix, world_origin and a local-origin out-parameter this rewrite names
//   local_origin.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out,
    real_point3d *point); // 0x4cbf80, math module (src/math/matrix4x3_inverse_transform_point.c)
    // blam-cc: ECX -> m, EDX -> out, ESI -> point
extern void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cc010; EAX out, EDX v, stack m
extern uint8_t ray_intersects_sphere(real_point3d *origin, real_vector3d *normal_out, real_vector3d *direction,
    real *t_out, real_point3d *center, real radius); // 0x4ce3a0; EAX origin, ECX normal, EDX direction, EDI t, stack (center, radius)

// Transforms the world-space ray (world_origin, world_direction) into context's object-local
// space and tests it against every mass point's collision sphere, keeping the closest hit.
// Writes the winning fraction, world-space contact normal and plane d into *out_result (t is
// seeded to FLT_MAX so a caller can tell "no hit" from result->t remaining unchanged) and
// returns whether anything was hit.
// FIXED (objdump 0x507610): EAX = world origin, EBX = out_result, stack = (context, world_direction); the calls to
//   matrix4x3_inverse_transform_vector and ray_intersects_sphere now follow those functions' definitions (the
//   draft passed them in a different order).
// blam-cc: EAX -> world_origin, EBX -> out_result, stack -> context, world_direction
uint8_t object_physics_test_ray_against_mass_points(real_point3d *world_origin,
    real_vector3d *world_direction, object_physics_ray_result *out_result,
    object_physics_context *context)
{
    Physics *definition = (Physics *)context->definition;
    real_point3d local_origin;
    real_vector3d local_direction;
    int32_t count = definition->mass_points.count;
    int16_t i;
    uint8_t hit = 0;

    out_result->t = 3.4028235e+38f; // FLT_MAX

    matrix4x3_inverse_transform_point((real_matrix4x3 *)&context->scale, &local_origin, world_origin);
    matrix4x3_inverse_transform_vector(&local_direction, world_direction, (real_matrix4x3 *)&context->scale); // 0x507625: EAX out, EDX v, push m

    for (i = 0; i < count; i++) {
        PhysicsMassPoint *mass_point =
            &((PhysicsMassPoint *)definition->mass_points.pointer)[i];
        real_vector3d normal;
        float t;

        if (ray_intersects_sphere(&local_origin, &normal, &local_direction, &t,
                (real_point3d *)&mass_point->position, mass_point->radius) && t < out_result->t) {
            out_result->t = t;
            out_result->plane_i = normal.i;
            out_result->plane_j = normal.j;
            out_result->plane_k = normal.k;
            hit = 1;
            out_result->plane_d =
                (local_direction.i * t + local_origin.x) * out_result->plane_i +
                (local_direction.j * t + local_origin.y) * out_result->plane_j +
                (local_direction.k * t + local_origin.z) * out_result->plane_k;
        }
    }

    if (hit) {
        float ni = out_result->plane_i;
        float nj = out_result->plane_j;
        float nk = out_result->plane_k;

        out_result->plane_i = ni * context->forward_i + nj * context->left_i + nk * context->up_i;
        out_result->plane_j = ni * context->forward_j + nj * context->left_j + nk * context->up_j;
        out_result->plane_k = ni * context->forward_k + nj * context->left_k + nk * context->up_k;
        out_result->plane_d = context->position_x * out_result->plane_i +
            out_result->plane_d * context->scale + context->position_y * out_result->plane_j +
            out_result->plane_k * context->position_z;
    }

    return hit;
}

#if 0
Original Ghidra decompilation (0x507610):

uint antenna_test_ray_against_vertex_spheres(int param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  float *unaff_EBX;
  short sVar8;
  char local_29;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_29 = '\0';
  *unaff_EBX = 3.4028235e+38;
  matrix4x3_inverse_transform_point();
  matrix4x3_inverse_transform_vector((float *)(param_1 + 8));
  iVar7 = *(int *)(param_1 + 4);
  sVar8 = 0;
  if ((int)*(uint *)(iVar7 + 0x74) < 1) {
    uVar6 = *(uint *)(iVar7 + 0x74) & 0xffffff00;
  }
  else {
    iVar5 = 0;
    do {
      cVar4 = ray_intersects_sphere
                        (iVar5 * 0x80 + *(int *)(iVar7 + 0x78) + 0x38,
                         *(undefined4 *)(iVar5 * 0x80 + 0x68 + *(int *)(iVar7 + 0x78)));
      if ((cVar4 != '\0') && (local_28 < *unaff_EBX)) {
        *unaff_EBX = local_28;
        unaff_EBX[1] = local_c;
        unaff_EBX[2] = local_8;
        unaff_EBX[3] = local_4;
        local_29 = '\x01';
        unaff_EBX[4] = (local_24 * local_28 + local_18) * unaff_EBX[1] +
                       (local_20 * local_28 + local_14) * unaff_EBX[2] +
                       (local_1c * local_28 + local_10) * unaff_EBX[3];
      }
      iVar7 = *(int *)(param_1 + 4);
      sVar8 = sVar8 + 1;
      iVar5 = (int)sVar8;
    } while (iVar5 < *(int *)(iVar7 + 0x74));
    uVar6 = CONCAT31((int3)(char)((ushort)sVar8 >> 8),local_29);
    if (local_29 != '\0') {
      fVar1 = unaff_EBX[1];
      fVar2 = unaff_EBX[2];
      fVar3 = unaff_EBX[3];
      unaff_EBX[1] = fVar1 * *(float *)(param_1 + 0xc) +
                     fVar2 * *(float *)(param_1 + 0x18) + fVar3 * *(float *)(param_1 + 0x24);
      unaff_EBX[2] = fVar1 * *(float *)(param_1 + 0x10) +
                     fVar2 * *(float *)(param_1 + 0x1c) + fVar3 * *(float *)(param_1 + 0x28);
      fVar1 = fVar1 * *(float *)(param_1 + 0x14) +
              fVar2 * *(float *)(param_1 + 0x20) + fVar3 * *(float *)(param_1 + 0x2c);
      unaff_EBX[3] = fVar1;
      unaff_EBX[4] = *(float *)(param_1 + 0x30) * unaff_EBX[1] +
                     unaff_EBX[4] * *(float *)(param_1 + 8) +
                     *(float *)(param_1 + 0x34) * unaff_EBX[2] + fVar1 * *(float *)(param_1 + 0x38);
      return uVar6;
    }
  }
  return uVar6;
}
#endif
