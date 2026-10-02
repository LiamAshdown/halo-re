// actor_avoid_obstacle_and_project  (Ghidra: FUN_004095c0; it picks the point an actor walks to before boarding)
// address 0x4095c0, size 1219 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4095c0..0x409a82 (the draft read the actor's tag instead of the vehicle's, dropped the
//   EAX/ECX/EDX arguments and returned the entry point unprojected). EAX actor, ECX vehicle, EDX the seat's
//   entry point; stack (hint, in/out near-line flag, out_point, out_surface_index). The walk target starts as the
//   entry point. Unless the vehicle tag's flag 0x10 (+0x17c) is set, a target on the far side of the vehicle
//   (bounding sphere +0xa0 / +0xac, the tag's radius +0x280 when positive) is moved round it: the target is the
//   hint (or the entry point once the actor is near the hint - entry line, or the hint lies within 0.5 of the
//   center); when the vehicle center lies ahead of the actor within 1.2 target lengths the point sits beside the
//   vehicle, perpendicular to the path, at 1.1 radii; otherwise directly outward from the target. A point that
//   ends up within 2 world units of the actor is pushed sideways to 2 units. The result is dropped onto the
//   structure BSP (a 4-unit ray down from 1 unit above it).
// blam-cc: EAX -> actor_index, ECX -> vehicle_index, EDX -> entry, stack -> (hint, in_out_near_line, out_point,
//   out_surface_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;       // 0x00880360
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern const real_vector3d *global_up3d_pointer;   // 0x00696720
extern const real_vector3d *global_down3d_pointer; // 0x0069672c

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                                ModelCollisionGeometryBSP *bsp,
                                                int16_t breakable_surface_count,
                                                uint32_t *breakable_surfaces, real_point3d *origin,
                                                real_vector3d *delta, float max_fraction);
    // 0x502060, src/physics/collision_bsp_query_segment_init.c; flags in EAX, result in ECX
extern double sqrt(double x); // FSQRT

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry,
    real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *vehicle_tag = TAG_DATA(*(datum_index *)vehicle);
    real_point3d point = *entry;
    uint8_t near_line = in_out_near_line != 0 ? *in_out_near_line : 0;
    collision_bsp_segment_result result;
    real_point3d start;
    real_vector3d delta;

    if ((vehicle_tag[0x17c] & 0x10) == 0) {
        real_point3d center = *(real_point3d *)&((vehicle_object *)vehicle)->base.bounding_center.x;
        float radius = ((vehicle_object *)vehicle)->base.bounding_radius;
        float ax = ((actor *)act)->body_position.x;
        float ay = ((actor *)act)->body_position.y;
        real_point3d *target_pointer;
        real_point3d target;
        real_vector2d to_center;
        real_vector2d to_target;
        real_vector2d from_target;
        real_vector2d away;

        if (*(float *)(vehicle_tag + 0x280) > 0.0f) {
            radius = *(float *)(vehicle_tag + 0x280);
        }
        target_pointer = entry;
        if (!near_line) {
            float dx = hint->x - center.x;
            float dy = hint->y - center.y;
            float dz = hint->z - center.z;
            float d = (float)sqrt(dz * dz + dy * dy + dx * dx);

            if (d <= 0.5f) {
                near_line = 1;
            } else {
                d = d + 0.3f;
                if (!(radius > d)) {
                    radius = d;
                }
                target_pointer = hint;
            }
        }
        target = *target_pointer;
        to_center.i = center.x - ax;
        to_center.j = center.y - ay;
        to_target.i = target.x - ax;
        to_target.j = target.y - ay;
        from_target.i = center.x - target.x;
        from_target.j = center.y - target.y;
        if (!near_line) {
            // 0x409751: the actor's offset from the hint - entry line (the direction is not normalized)
            float lx = hint->x - entry->x;
            float ly = hint->y - entry->y;
            float t = -(to_target.j * ly + lx * to_target.i);
            float px = to_target.i + lx * t;
            float py = to_target.j + ly * t;

            if (!(py * py + px * px > 0.1225f)) {
                near_line = 1;
            }
        }
        {
            float length_squared = to_target.j * to_target.j + to_target.i * to_target.i;
            float ratio;

            if (!(length_squared > 0.0f)) {
                goto project;
            }
            ratio = (to_target.j * to_center.j + to_target.i * to_center.i) / length_squared;
            if (ratio > 0.0f && ratio <= 1.2f) {
                away.i = -to_target.j;
                away.j = to_target.i;
                if (to_center.j * to_target.i + away.i * to_center.i > 0.0f) {
                    away.i = to_target.j;
                    away.j = -to_target.i;
                }
            } else {
                if (near_line) {
                    goto project;
                }
                away.i = -from_target.i;
                away.j = -from_target.j;
            }
        }
        if (!(vector2d_normalize_with_length(&away) > 0.0f)) {
            goto project;
        }
        point.x = away.i * (radius * 1.1f) + center.x;
        point.y = away.j * (radius * 1.1f) + center.y;
        {
            float dx = point.x - ax;
            float dy = point.y - ay;
            float dz = point.z - ((actor *)act)->body_position.z;
            float distance_squared = dz * dz + dy * dy + dx * dx;
            float distance;
            real_vector3d side;

            if (!(distance_squared > 0.0001f) || !(distance_squared <= 4.0f)) {
                goto project;
            }
            distance = (float)sqrt(distance_squared);
            side.i = -from_target.j;
            side.j = from_target.i;
            if (!(dy * from_target.i + side.i * dx > 0.0f)) {
                side.i = from_target.j;
                side.j = -from_target.i;
            }
            side.k = 0.0f;
            if (!(vector2d_normalize_with_length((real_vector2d *)&side) > 0.0f)) {
                goto project;
            }
            distance = 2.0f - distance;
            point.x = side.i * distance + point.x;
            point.y = side.j * distance + point.y;
            point.z = side.k * distance + point.z;
        }
    }
project:
    if (in_out_near_line != 0) {
        *in_out_near_line = near_line;
    }
    start.x = point.x + global_up3d_pointer->i;
    start.y = point.y + global_up3d_pointer->j;
    start.z = point.z + global_up3d_pointer->k;
    delta.i = global_down3d_pointer->i * 4.0f;
    delta.j = global_down3d_pointer->j * 4.0f;
    delta.k = global_down3d_pointer->k * 4.0f;
    if (!collision_bsp_query_segment_init(1, &result, global_structure_collision_bsp, 0, 0, &start, &delta,
                                          3.4028235e38f)) {
        return 0;
    }
    *out_surface_index = result.surface_index;
    out_point->x = delta.i * result.t + start.x;
    out_point->y = delta.j * result.t + start.y;
    out_point->z = delta.k * result.t + start.z;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4095c0):

undefined4 FUN_004095c0(float *param_1,char *param_2,float *param_3,undefined4 *param_4)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  char cVar5;
  uint in_EAX;
  int iVar6;
  float *pfVar7;
  uint in_ECX;
  float *in_EDX;
  float10 fVar8;
  float local_45c;
  float local_458;
  float local_454;
  float local_450;
  float local_44c;
  float local_448;
  float local_444;
  float local_440;
  float local_43c;
  float local_438;
  float local_434;
  float local_430;
  float local_42c;
  float local_428;
  float local_424;
  float local_420;
  float local_41c;
  float local_418;
  undefined4 local_410;

  iVar6 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_42c = *in_EDX;
  local_428 = in_EDX[1];
  local_424 = in_EDX[2];
  if (param_2 == (char *)0x0) {
    cVar5 = '\0';
  }
  else {
    cVar5 = *param_2;
  }
  if ((*(byte *)(iVar2 + 0x17c) & 0x10) != 0) goto LAB_004099a1;
  local_458 = (float)puVar1[0x28];
  local_45c = (float)puVar1[0x2b];
  local_454 = (float)puVar1[0x29];
  local_450 = (float)puVar1[0x2a];
  if (0.0 < *(float *)(iVar2 + 0x280)) {
    local_45c = *(float *)(iVar2 + 0x280);
  }
  pfVar7 = in_EDX;
  if (cVar5 == '\0') {
    fVar3 = SQRT((*param_1 - local_458) * (*param_1 - local_458) +
                 (param_1[1] - local_454) * (param_1[1] - local_454) +
                 (param_1[2] - local_450) * (param_1[2] - local_450));
    if (fVar3 < 0.5) {
      cVar5 = '\x01';
    }
    else {
      fVar3 = fVar3 + 0.3;
      pfVar7 = param_1;
      if (local_45c <= fVar3) {
        local_45c = fVar3;
      }
    }
  }
  local_438 = local_458 - *(float *)(iVar6 + 300);
  local_444 = *pfVar7;
  local_440 = pfVar7[1];
  local_43c = pfVar7[2];
  local_434 = local_454 - *(float *)(iVar6 + 0x130);
  local_44c = local_444 - *(float *)(iVar6 + 300);
  local_448 = local_440 - *(float *)(iVar6 + 0x130);
  local_420 = local_458 - local_444;
  local_41c = local_454 - local_440;
  if ((cVar5 == '\0') &&
     (fVar3 = -((*param_1 - *in_EDX) * local_44c + local_448 * (param_1[1] - in_EDX[1])),
     local_444 = (*param_1 - *in_EDX) * fVar3 + local_44c,
     fVar3 = (param_1[1] - in_EDX[1]) * fVar3 + local_448,
     local_444 * local_444 + fVar3 * fVar3 < 0.122499995)) {
    cVar5 = '\x01';
  }
  fVar3 = local_44c * local_44c + local_448 * local_448;
  if (fVar3 <= 0.0) goto LAB_004099a1;
  fVar3 = (local_44c * local_438 + local_448 * local_434) / fVar3;
  if ((fVar3 <= 0.0) || (1.2 <= fVar3)) {
    fVar3 = local_41c;
    fVar4 = local_420;
    if (cVar5 != '\0') goto LAB_004099a1;
LAB_00409848:
    local_444 = -fVar4;
    local_440 = -fVar3;
  }
  else {
    local_444 = -local_448;
    fVar3 = local_44c;
    local_440 = local_44c;
    fVar4 = local_444;
    if (0.0 < local_444 * local_438 + local_434 * local_44c) goto LAB_00409848;
  }
  fVar8 = (float10)vector2d_normalize_with_length();
  if ((float10)0.0 < fVar8) {
    local_42c = local_444 * local_45c * 1.1 + local_458;
    local_428 = local_440 * local_45c * 1.1 + local_454;
    local_438 = local_42c - *(float *)(iVar6 + 300);
    local_434 = local_428 - *(float *)(iVar6 + 0x130);
    fVar3 = local_424 - *(float *)(iVar6 + 0x134);
    fVar3 = local_438 * local_438 + local_434 * local_434 + fVar3 * fVar3;
    if ((0.0001 < fVar3) && (fVar3 < 4.0)) {
      local_454 = local_420;
      local_458 = -local_41c;
      if (local_458 * local_438 + local_434 * local_420 < 0.0) {
        local_458 = -local_458;
        local_454 = -local_420;
      }
      local_450 = 0.0;
      fVar8 = (float10)vector2d_normalize_with_length();
      if ((float10)0.0 < fVar8) {
        fVar3 = 2.0 - SQRT(fVar3);
        local_42c = local_458 * fVar3 + local_42c;
        local_428 = local_454 * fVar3 + local_428;
        local_424 = fVar3 * local_450 + local_424;
      }
    }
  }
LAB_004099a1:
  if (param_2 != (char *)0x0) {
    *param_2 = cVar5;
  }
  local_438 = local_42c + *(float *)PTR_DAT_00696720;
  local_434 = local_428 + *(float *)(PTR_DAT_00696720 + 4);
  local_430 = local_424 + *(float *)(PTR_DAT_00696720 + 8);
  local_458 = *(float *)PTR_DAT_0069672c * 4.0;
  local_454 = *(float *)(PTR_DAT_0069672c + 4) * 4.0;
  local_450 = *(float *)(PTR_DAT_0069672c + 8) * 4.0;
  cVar5 = FUN_00502060(DAT_00746f98,0,0,&local_438,&local_458,0x7f7fffff);
  if (cVar5 == '\0') {
    return 0;
  }
  *param_4 = local_410;
  *param_3 = local_458 * local_418 + local_438;
  param_3[1] = local_454 * local_418 + local_434;
  param_3[2] = local_450 * local_418 + local_430;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
