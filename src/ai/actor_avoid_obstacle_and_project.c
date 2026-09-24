// actor_avoid_obstacle_and_project  (Ghidra: actor_avoid_obstacle_and_project, renamed)
// address 0x4095c0, size 1219 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: types/ai.h actor's cached position (0x12c/0x130/0x134); types/objects.h
//   object.bounding_center (0xa0)/bounding_radius (0xac), matching the offsets
//   actor_movement_obstacle's own header comment cites (object+0xa0/0xa4/0xa8/0xac); phase-4
//   summary "adjusts a destination point to avoid a nearby obstacle and projects the result
//   onto the ground via a ray cast".
//
// This is dense 2D/3D tangent-circle geometry with many interdependent float temporaries.
// Given the size, this rewrite keeps the Ghidra decompilation's own local variable names
// (local_45c, local_458, ...) rather than renaming them, so the arithmetic can be checked
// line-for-line against the #if 0 block; only pointer/offset expressions are cleaned up.
// Confidence is low; treat this as a starting point for a future pass.
// UNSURE, broadly: object+0x17c bit 0x10 (a "no avoidance" flag) and Actor+0x280 (an
// override avoidance-radius float) are read as raw offsets; the two globals
// PTR_DAT_00696720/PTR_DAT_0069672c (a fixed vector and axis triple) and DAT_00746f98 (the
// collision BSP index, already named bsp_index in types/ai.h's globals list) are declared as
// externs matching their use here. FUN_00502060 (a raycast) and vector2d_normalize_with_
// length's in/out aliasing are reproduced as literally as C allows.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t bsp_index; // 0x00746f98
extern const real_vector3d *global_up3d_pointer; // 0x00696720
extern const real_vector3d *global_down3d_pointer; // 0x0069672c, UNSURE: a third axis vector, likely "down" by symmetry with global_up3d_pointer

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern char FUN_00502060(int32_t bsp, int32_t unused_a, int32_t unused_b, real_point3d *from, real_vector3d *delta, int32_t max_fraction); // 0x502060, not yet rewritten

extern double sqrt(double x); // FSQRT

int32_t actor_avoid_obstacle_and_project(uint32_t actor_index, real_point3d *candidate, char *out_avoided_flag, uint32_t object_index, float *out_point, uint32_t *out_extra)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    float local_45c, local_458, local_454, local_450, local_44c, local_448, local_444, local_440;
    float local_43c, local_438, local_434, local_430, local_42c, local_428, local_424, local_420;
    float local_41c, local_418 = 0.0f;
    uint32_t local_410 = 0;
    char cVar5;
    float *pfVar7;

    local_42c = candidate->x;
    local_428 = candidate->y;
    local_424 = candidate->z;
    cVar5 = (out_avoided_flag != 0) ? *out_avoided_flag : 0;

    if ((*(uint8_t *)((uint8_t *)obj + 0x17c) & 0x10) != 0) {
        goto skip_avoidance;
    }

    local_458 = obj->bounding_center.x;
    local_45c = obj->bounding_radius;
    local_454 = obj->bounding_center.y;
    local_450 = obj->bounding_center.z;
    if (actor_def->pathfinding_radius > 0.0f) { // UNSURE: Actor+0x280 guessed as pathfinding_radius reused; the offset does not match that field exactly, see header
        local_45c = *(float *)((uint8_t *)actor_def + 0x280);
    }

    pfVar7 = (float *)candidate;
    if (cVar5 == 0) {
        float dx = out_point[0] - local_458, dy = out_point[1] - local_454, dz = out_point[2] - local_450;
        float dist = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        if (dist < 0.5f) {
            cVar5 = 1;
        } else {
            dist += 0.3f;
            pfVar7 = out_point;
            if (local_45c <= dist) {
                local_45c = dist;
            }
        }
    }

    local_438 = local_458 - a->body_position.x;
    local_444 = pfVar7[0];
    local_440 = pfVar7[1];
    local_43c = pfVar7[2];
    local_434 = local_454 - a->body_position.y;
    local_44c = local_444 - a->body_position.x;
    local_448 = local_440 - a->body_position.y;
    local_420 = local_458 - local_444;
    local_41c = local_454 - local_440;

    if (cVar5 == 0) {
        float t = -((out_point[0] - candidate->x) * local_44c + local_448 * (out_point[1] - candidate->y));
        float px = (out_point[0] - candidate->x) * t + local_44c;
        float py = (out_point[1] - candidate->y) * t + local_448;
        if (px * px + py * py < 0.122499995f) {
            cVar5 = 1;
        }
    }

    {
        float denom = local_44c * local_44c + local_448 * local_448;
        float t;

        if (denom <= 0.0f) {
            goto skip_avoidance;
        }
        t = (local_44c * local_438 + local_448 * local_434) / denom;
        if (t <= 0.0f || t >= 1.2f) {
            float fx = local_41c, fy = local_420;
            if (cVar5 != 0) {
                goto skip_avoidance;
            }
            local_444 = -fy;
            local_440 = -fx;
        } else {
            float fx, fy;
            local_444 = -local_448;
            fx = local_44c;
            local_440 = local_44c;
            fy = local_444;
            if (0.0f < local_444 * local_438 + local_434 * local_44c) {
                local_444 = -fy;
                local_440 = -fx;
            }
        }
    }

    {
        real_vector2d dir;
        dir.i = local_444;
        dir.j = local_440;
        if (vector2d_normalize_with_length(&dir) > 0.0f) {
            local_444 = dir.i;
            local_440 = dir.j;
            local_42c = local_444 * local_45c * 1.1f + local_458;
            local_428 = local_440 * local_45c * 1.1f + local_454;
            local_438 = local_42c - a->body_position.x;
            local_434 = local_428 - a->body_position.y;
            {
                float dz = local_424 - a->body_position.z;
                float dist2 = local_438 * local_438 + local_434 * local_434 + dz * dz;
                if (dist2 > 0.0001f && dist2 < 4.0f) {
                    real_vector2d dir2;
                    local_454 = local_420;
                    local_458 = -local_41c;
                    dir2.i = local_458;
                    dir2.j = local_454;
                    if (dir2.i * local_438 + local_434 * local_420 < 0.0f) {
                        dir2.i = -dir2.i;
                        dir2.j = -local_420;
                    }
                    local_450 = 0.0f;
                    if (vector2d_normalize_with_length(&dir2) > 0.0f) {
                        float remaining = 2.0f - (float)sqrt((double)dist2);
                        local_42c = dir2.i * remaining + local_42c;
                        local_428 = dir2.j * remaining + local_428;
                        local_424 = remaining * local_450 + local_424;
                    }
                }
            }
        }
    }

skip_avoidance:
    if (out_avoided_flag != 0) {
        *out_avoided_flag = cVar5;
    }
    {
        real_point3d from;
        real_vector3d delta;

        from.x = local_42c + global_up3d_pointer->i;
        from.y = local_428 + global_up3d_pointer->j;
        from.z = local_424 + global_up3d_pointer->k;
        delta.i = global_down3d_pointer->i * 4.0f;
        delta.j = global_down3d_pointer->j * 4.0f;
        delta.k = global_down3d_pointer->k * 4.0f;

        if (FUN_00502060(bsp_index, 0, 0, &from, &delta, 0x7f7fffff) == 0) {
            return 0;
        }
        *out_extra = local_410;
        out_point[0] = delta.i * local_418 + from.x;
        out_point[1] = delta.j * local_418 + from.y;
        out_point[2] = delta.k * local_418 + from.z;
        return 1;
    }
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
