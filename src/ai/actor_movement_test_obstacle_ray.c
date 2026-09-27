// actor_movement_test_obstacle_ray  (Ghidra: actor_movement_test_obstacle_ray, already named)
// address 0x418f70, size 710 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: takes one row of the precomputed avoidance sample tables (the seven floats
// actor_avoidance_build_direction_tables writes: scale, radius * unit vector, elevation
// vector), rotates both vectors out of the actor frame held in actor_movement_context
// (forward 0x18, left 0x24, up 0x30), produces the sample's world end point and its scaled
// elevation vector, then reports how far along the segment from the actor's position the
// first blocker sits: the collision BSP first, then every collected obstacle cylinder.
// A null result leaves the caller's per-sample "clear" counter free to saturate upward.
// register convention: the context is in EDI, the elevation output in EAX and the end point
// in EDX, the sample row in ECX; the two out-parameters are genuine stack parameters
// (objdump: [esp+0x448] and [esp+0x44c]).
// blam-cc: EAX -> out_elevation, ECX -> sample, EDX -> out_end_point, EDI -> context,
//          stack -> out_distance, out_clear_counter
//
// Facts recovered from the disassembly that Ghidra hides:
//  - the value compared against *out_distance inside the obstacle loop is not the sample
//    row's seventh float (Ghidra's re-used fVar16) but a float out-parameter of
//    ray_intersects_cylinder at [esp+0xc]; the call is
//    ray_intersects_cylinder(EAX = &hit_fraction_slot, ECX = &obstacle->position,
//    stack: obstacle->height, obstacle->radius, &segment).
//  - the second 0x502060 call is made with EAX = 3 and ECX pointing at a 0x400-byte scratch
//    buffer, and on success *out_distance is taken from [esp+0x2c], the fraction that call
//    leaves behind.
//  - the counter saturates with an unsigned compare against 0xff, so it is a uint8 counter,
//    not the signed -1 test Ghidra prints.
// reconciled: R54 both 0x502060 calls use the physics rewrite collision_bsp_query_segment_init
//   (EAX = flags 3, ECX = &collision_bsp_segment_result, stack: collision bsp (context +0x04,
//   the 0x00746f98 pointer), 0, 0, origin, delta, 1.0f; add esp,0x18 after each). The first call
//   (0x419144) tests context->position along segment; the second (0x419180) tests from the end
//   point (EBX) along the scaled elevation vector (ESI), and on a hit *out_distance is the
//   result's t (the first dword of the shared buffer, [esp+0x2c]). The old FUN_00502060 /
//   FUN_00502060_query aliases and their argument lists are gone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "physics.h"

extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                                ModelCollisionGeometryBSP *bsp,
                                                int16_t breakable_surface_count,
                                                uint32_t *breakable_surfaces, real_point3d *origin,
                                                real_vector3d *delta, float max_fraction);
    // 0x502060, src/physics/collision_bsp_query_segment_init.c; flags in EAX, result in ECX
extern uint8_t ray_intersects_cylinder(real height, real radius, real_vector3d *hit_out, real *t_out,
    real_point3d *center, real_point3d *origin, real_vector3d *direction); // 0x4ce4e0, stack x3, EAX, ECX, EBX, ESI // 0x4ce4e0, EAX/ECX + stack

// blam-cc: EAX -> out_elevation, ECX -> sample, EDX -> out_end_point, EDI -> context,
//          stack -> out_distance, out_clear_counter
int16_t actor_movement_test_obstacle_ray(real_vector3d *out_elevation, const float *sample,
                                      real_point3d *out_end_point, actor_movement_context *context,
                                      float *out_distance, uint8_t *out_clear_counter)
{
    real_vector3d elevation;
    real_vector3d segment;
    collision_bsp_segment_result bsp_result; // [esp+0x2c] of the caller frame, shared by both calls
    float hit_fraction;
    float scale;
    int16_t result;
    int16_t i;

    *out_distance = 3.4028235e+38f;
    result = 0;

    elevation.i = sample[6] * context->up.i + sample[5] * context->left.i +
                  sample[4] * context->forward.i + global_origin3d_pointer->i;
    elevation.j = sample[6] * context->up.j + sample[5] * context->left.j +
                  sample[4] * context->forward.j + global_origin3d_pointer->j;
    elevation.k = sample[6] * context->up.k + sample[5] * context->left.k +
                  sample[4] * context->forward.k + global_origin3d_pointer->k;

    out_end_point->x = (sample[3] * context->up.i + sample[2] * context->left.i +
                        sample[1] * context->forward.i + global_origin3d_pointer->i) *
                       context->unknown_6040 + context->position.x;
    out_end_point->y = (sample[3] * context->up.j + sample[2] * context->left.j +
                        sample[1] * context->forward.j + global_origin3d_pointer->j) *
                       context->unknown_6040 + context->position.y;
    out_end_point->z = (sample[3] * context->up.k + sample[2] * context->left.k +
                        sample[1] * context->forward.k + global_origin3d_pointer->k) *
                       context->unknown_6040 + context->position.z;

    scale = context->search_radius * sample[0];
    out_elevation->i = elevation.i * scale;
    out_elevation->j = elevation.j * scale;
    out_elevation->k = elevation.k * scale;

    segment.i = out_end_point->x - context->position.x;
    segment.j = out_end_point->y - context->position.y;
    segment.k = out_end_point->z - context->position.z;

    if (collision_bsp_query_segment_init(3, &bsp_result,
                                         (ModelCollisionGeometryBSP *)context->collision_bsp,
                                         0, 0, &context->position, &segment, 1.0f) != 0) {
        result = 2;
        *out_distance = 0.0f;
    } else if (collision_bsp_query_segment_init(3, &bsp_result,
                                                (ModelCollisionGeometryBSP *)context->collision_bsp,
                                                0, 0, out_end_point, out_elevation, 1.0f) != 0) {
        result = 2;
        *out_distance = bsp_result.t;
    }

    for (i = 0; i < context->obstacle_count; i++) {
        actor_movement_obstacle *obstacle = &context->obstacles[i];

        // FIXED (objdump 0x4191b0..0x4191ce): stack = (height, radius, &segment as the hit buffer), EAX = &hit_fraction,
        //   ECX = &obstacle->position (x, y, bottom), EBX = the ray origin (out_end_point), ESI = the direction
        //   (out_elevation). The draft passed five arguments in the wrong order.
        if (ray_intersects_cylinder(obstacle->height, obstacle->radius, &segment, &hit_fraction,
                                    (real_point3d *)&obstacle->position, out_end_point, out_elevation) != 0 &&
            hit_fraction < *out_distance) {
            result = 1;
            *out_distance = hit_fraction;
        }
    }

    if (out_clear_counter != (uint8_t *)0) {
        if (result > 0) {
            *out_clear_counter = 0;
            return result;
        }
        if (*out_clear_counter != 0xff) {
            *out_clear_counter = (uint8_t)(*out_clear_counter + 1);
        }
    }
    return result; // 0x419212..0x419235: AX = 0 clear, 1 obstacle, 2 structure (0x4193d0 tests it)
}

#if 0
Original Ghidra decompilation (0x418f70):

void actor_movement_test_obstacle_ray(float *param_1,char *param_2)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  char cVar17;
  short sVar18;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  short sVar19;
  int unaff_EDI;
  float local_430;
  float local_42c;
  float local_428;
  float local_424;
  float local_420;
  float local_41c;
  float local_418;

  *param_1 = 3.4028235e+38;
  fVar3 = in_ECX[1];
  fVar11 = *(float *)(PTR_DAT_00696714 + 4);
  fVar12 = *(float *)(PTR_DAT_00696714 + 8);
  fVar4 = *(float *)(unaff_EDI + 0x1c);
  fVar13 = in_ECX[2];
  fVar14 = in_ECX[3];
  sVar18 = 0;
  fVar5 = *(float *)(unaff_EDI + 0x20);
  fVar6 = *(float *)(unaff_EDI + 0x28);
  fVar7 = *(float *)(unaff_EDI + 0x2c);
  fVar15 = in_ECX[5];
  fVar8 = *(float *)(unaff_EDI + 0x34);
  fVar9 = *(float *)(unaff_EDI + 0x38);
  fVar10 = in_ECX[4];
  fVar16 = in_ECX[6];
  pfVar1 = (float *)(unaff_EDI + 0xc);
  local_424 = fVar16 * *(float *)(unaff_EDI + 0x30) +
              fVar15 * *(float *)(unaff_EDI + 0x24) +
              fVar10 * *(float *)(unaff_EDI + 0x18) + *(float *)PTR_DAT_00696714;
  local_420 = fVar16 * *(float *)(unaff_EDI + 0x34) +
              fVar15 * *(float *)(unaff_EDI + 0x28) +
              fVar10 * *(float *)(unaff_EDI + 0x1c) + *(float *)(PTR_DAT_00696714 + 4);
  local_41c = fVar16 * *(float *)(unaff_EDI + 0x38) +
              fVar15 * *(float *)(unaff_EDI + 0x2c) +
              fVar10 * *(float *)(unaff_EDI + 0x20) + *(float *)(PTR_DAT_00696714 + 8);
  fVar10 = *(float *)(unaff_EDI + 0x6040);
  *in_EDX = (fVar14 * *(float *)(unaff_EDI + 0x30) +
            fVar13 * *(float *)(unaff_EDI + 0x24) +
            fVar3 * *(float *)(unaff_EDI + 0x18) + *(float *)PTR_DAT_00696714) * fVar10 + *pfVar1;
  in_EDX[1] = (fVar14 * fVar8 + fVar13 * fVar6 + fVar3 * fVar4 + fVar11) * fVar10 +
              *(float *)(unaff_EDI + 0x10);
  in_EDX[2] = (fVar14 * fVar9 + fVar13 * fVar7 + fVar3 * fVar5 + fVar12) * fVar10 +
              *(float *)(unaff_EDI + 0x14);
  fVar3 = *(float *)(unaff_EDI + 0x6044) * *in_ECX;
  *in_EAX = local_424 * fVar3;
  in_EAX[1] = local_420 * fVar3;
  in_EAX[2] = local_41c * fVar3;
  local_430 = *in_EDX - *pfVar1;
  local_42c = in_EDX[1] - *(float *)(unaff_EDI + 0x10);
  local_428 = in_EDX[2] - *(float *)(unaff_EDI + 0x14);
  cVar17 = FUN_00502060(*(undefined4 *)(unaff_EDI + 4),0,0,pfVar1,&local_430,0x3f800000);
  if (cVar17 == '\0') {
    cVar17 = FUN_00502060(*(undefined4 *)(unaff_EDI + 4),0,0);
    if (cVar17 != '\0') {
      sVar18 = 2;
      *param_1 = local_418;
    }
  }
  else {
    sVar18 = 2;
    *param_1 = 0.0;
  }
  sVar19 = 0;
  if (0 < *(short *)(unaff_EDI + 0x3c)) {
    do {
      iVar2 = unaff_EDI + 0x40 + sVar19 * 0x18;
      cVar17 = ray_intersects_cylinder
                         (*(undefined4 *)(iVar2 + 0x10),*(undefined4 *)(iVar2 + 0x14),&local_430);
      if ((cVar17 != '\0') && (fVar16 < *param_1)) {
        sVar18 = 1;
        *param_1 = fVar16;
      }
      sVar19 = sVar19 + 1;
    } while (sVar19 < *(short *)(unaff_EDI + 0x3c));
  }
  if (param_2 != (char *)0x0) {
    if (sVar18 != 0) {
      *param_2 = '\0';
      return;
    }
    if (*param_2 != -1) {
      *param_2 = *param_2 + '\x01';
    }
  }
  return;
}
#endif
