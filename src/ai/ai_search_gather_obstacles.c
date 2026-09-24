// ai_search_gather_obstacles  (Ghidra: ai_search_gather_obstacles, already named)
// address 0x43c510, size 883 bytes
// name confidence: 0.5   rewrite confidence: 0.1
// evidence: types/ai.h notes this function as the one that "fills [ai_search_obstacle_list]
// from object_find_in_sphere plus each object's vault/cover surface points". Calls
// point3d_within_radius (math helper, this task's skip list excludes it from rewriting),
// ai_search_append_obstacle (ai_search_append_obstacle, this rewrite), matrix4x3_transform_point /
// object_get_world_matrix / object_find_in_sphere (all established elsewhere with full
// signatures the two former are called here without, per the header note below).
//
// Kept close to the Ghidra decompilation and at very low confidence: almost every offset
// here reaches into the object/unit-type tag data (object flags, ActorType/vault
// classification, a per-object-type "vault point" reflexive block with per-point radius and
// direction fields) that belongs to types/objects.h / types/units.h / types/tags.h and is
// not established at these specific sub-offsets anywhere in this module. Reproduced as raw
// offsets rather than asserted as named fields.
//
// register convention: stack -> the six Ghidra-recognized formal parameters.
//   // blam-cc: stack -> cluster_ref, center, radius, direction, self_object_a, self_object_b
//
// UNSURE: object_get_world_matrix and matrix4x3_transform_point are each called here with
// fewer visible arguments than their established signatures take; called explicitly below
// with the operands available in this scope (the object being examined, and its own vault
// point / world matrix), which is the most plausible reading but not confirmed against a
// disassembly of this function.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include <stdint.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t point3d_within_radius(float value); // 0x43c340, math helper, not rewritten here
extern uint8_t ai_search_append_obstacle(ai_search_obstacle_list *list, uint16_t flags, uint32_t object_index,
                                         real_point2d *position, float radius); // 0x43c4b0
extern int16_t object_find_in_sphere(int32_t kind, int32_t type_mask, const void *from,
                                     const real_point3d *center, float radius,
                                     datum_index *out_objects, int32_t maximum_count); // 0x4f6fe0
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); // 0x4f6a20
extern void matrix4x3_transform_point(real_point3d *out, const real_point3d *point,
                                      const real_matrix4x3 *matrix); // 0x4cbde0

// blam-cc: stack -> list, center, radius, direction, self_object_a, self_object_b
//
// UNSURE: `list` (Ghidra's `param_1`) is never dereferenced directly in this function's own
// body -- it is purely forwarded into ai_search_append_obstacle's own EDX convention, which
// this rewrite makes an explicit parameter and argument instead of an implicit
// register pass-through.
void ai_search_gather_obstacles(ai_search_obstacle_list *list, real_point3d *center,
                                float radius, real_vector3d *direction, uint32_t self_object_a, uint32_t self_object_b)
{
    datum_index candidates[256];
    int16_t found;

    found = object_find_in_sphere(1, 0xc3,
                                  (uint8_t *)(*(int32_t *)((uint8_t *)((object_header *)object_data->data) + 8 +
                                                            (self_object_a & 0xffff) * 0xc)) + 0x98,
                                  center, radius, candidates, 0x100);

    if (0 < found) {
        int32_t i;
        for (i = 0; i < found; i = i + 1) {
            uint32_t handle = candidates[i];
            uint8_t *object = *(uint8_t **)(*(int32_t *)((uint8_t *)object_data + 0x34) + 8 + (handle & 0xffff) * 0xc);

            if ((handle == self_object_a) || (handle == self_object_b)) {
                continue;
            }
            if ((*(uint32_t *)(object + 0x10) & 1) != 0) { // object_header flags bit0
                continue;
            }

            {
                int32_t definition = *(int32_t *)((*(uint32_t *)object & 0xffff) * 0x20 + 0x14 + (uint32_t)(uintptr_t)tag_instances);
                int16_t kind = *(int16_t *)(object + 0xb4); // UNSURE: unit-type "kind"/classification field
                uint8_t vault_ok = 1;

                if (kind == 0) {
                    vault_ok = (*(uint8_t *)(object + 0x106) & 4) != 0;
                } else if (kind == 7) {
                    uint16_t flags292 = *(uint16_t *)(*(int32_t *)((*(uint32_t *)object & 0xffff) * 0x20 + 0x14 +
                                                                    (uint32_t)(uintptr_t)tag_instances) + 0x292);
                    vault_ok = (flags292 & 1) != 0 && (((flags292 & 2) == 0) || (*(float *)(object + 0x208) == 1.0f));
                }

                if (vault_ok && (point3d_within_radius(radius + *(float *)(object + 0xac)) != 0)) {
                    int32_t obj_definition = definition;
                    int32_t parent_definition = *(int32_t *)((*(uint32_t *)(obj_definition + 0x7c) & 0xffff) * 0x20 +
                                                              0x14 + (uint32_t)(uintptr_t)tag_instances);
                    if (((*(uint8_t *)(obj_definition + 2) & 8) == 0) && (0 < *(int32_t *)(parent_definition + 0x280))) {
                        real_matrix4x3 world_matrix;
                        int32_t point_count = *(int32_t *)(parent_definition + 0x280);
                        int32_t point_index;

                        object_get_world_matrix(handle, &world_matrix);

                        for (point_index = 0; point_index < point_count; point_index = point_index + 1) {
                            int16_t *point_def = (int16_t *)(point_index * 0x20 + *(int32_t *)(parent_definition + 0x284));
                            real_point3d world_point;
                            float radius_at_point;

                            if (*point_def == -1) {
                                matrix4x3_transform_point(&world_point, center, &world_matrix); // UNSURE: point argument
                                radius_at_point = world_point.x * *(float *)(point_def + 0xe);
                            } else {
                                uint8_t *unit_data = *(uint8_t **)(*(int32_t *)((uint8_t *)object_data + 0x34) + 8 +
                                                                    (handle & 0xffff) * 0xc);
                                real_point3d *marker = (real_point3d *)((int32_t)*(int16_t *)(unit_data + 0x1f2) +
                                                                        *point_def * 0x34 + (int32_t)(uintptr_t)unit_data);
                                matrix4x3_transform_point(&world_point, marker, &world_matrix);
                                radius_at_point = *(float *)(point_def + 0xe) * world_point.x;
                            }

                            if (((center->z <= world_point.z + radius_at_point + 0.5f) || (direction->k <= -0.2f)) &&
                                (((world_point.z - radius_at_point) - 0.5f <= center->z) || (0.2f <= direction->k))) {
                                float dx = world_point.x - center->x;
                                float dy = world_point.y - center->y;
                                float dz = world_point.z - center->z;
                                if (dx * dx + dy * dy + dz * dz * 4.0f <= (radius_at_point + radius) * (radius_at_point + radius)) {
                                    uint16_t flags = 0;
                                    if ((kind == 0) &&
                                        (0.0f < dz * direction->k + dx * direction->i + dy * direction->j) &&
                                        (0.06666667f < *(float *)(object + 0x68) * direction->i +
                                                       *(float *)(object + 0x6c) * direction->j +
                                                       *(float *)(object + 0x70) * direction->k)) {
                                        flags = 1;
                                    }
                                    {
                                        real_point2d p; p.x = world_point.x; p.y = world_point.y;
                                        ai_search_append_obstacle(list, flags, handle, &p, radius_at_point);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#if 0
// ---- original Ghidra decompilation (ai_search_gather_obstacles @ 0x43c510) ----
void ai_search_gather_obstacles
               (undefined4 param_1,float *param_2,float param_3,float *param_4,uint param_5,
               uint param_6)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  char cVar8;
  ushort uVar9;
  short sVar10;
  int iVar11;
  short *psVar12;
  undefined4 uVar13;
  int iVar14;
  float *pfVar15;
  int iVar16;
  uint *local_46c;
  uint local_454;
  float local_44c;
  float local_448;
  float local_444;
  float local_440 [14];
  uint local_408 [257];

  uVar9 = object_find_in_sphere
                    (1,0xc3,*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_5 & 0xffff) * 0xc) +
                            0x98,param_2,param_3,local_408,0x100);
  if (0 < (short)uVar9) {
    local_454 = (uint)uVar9;
    local_46c = local_408;
    iVar16 = DAT_0087bc14;
    do {
      uVar1 = *local_46c;
      iVar14 = (uVar1 & 0xffff) * 0xc;
      puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar14);
      if ((((((uVar1 != param_5) && (uVar1 != param_6)) && ((puVar2[4] & 1) == 0)) &&
           ((((short)puVar2[0x2d] != 0 || ((*(byte *)((int)puVar2 + 0x106) & 4) == 0)) &&
            (((short)puVar2[0x2d] != 7 ||
             ((uVar9 = *(ushort *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + iVar16) + 0x292),
              (uVar9 & 1) != 0 && (((uVar9 & 2) == 0 || (puVar2[0x82] != 0x3f800000)))))))))) &&
          (cVar8 = FUN_0043c340(param_3 + (float)puVar2[0x2b]), cVar8 != '\0')) &&
         ((iVar11 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + iVar16),
          iVar3 = *(int *)((*(uint *)(iVar11 + 0x7c) & 0xffff) * 0x20 + 0x14 + iVar16),
          (*(byte *)(iVar11 + 2) & 8) == 0 && (0 < *(int *)(iVar3 + 0x280))))) {
        object_get_world_matrix();
        iVar11 = 0;
        sVar10 = 0;
        iVar16 = DAT_0087bc14;
        if (0 < *(int *)(iVar3 + 0x280)) {
          do {
            psVar12 = (short *)(iVar11 * 0x20 + *(int *)(iVar3 + 0x284));
            if (*psVar12 == -1) {
              matrix4x3_transform_point(local_440);
              fVar4 = local_440[0] * *(float *)(psVar12 + 0xe);
            }
            else {
              iVar16 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar14);
              pfVar15 = (float *)((int)*(short *)(iVar16 + 0x1f2) + *psVar12 * 0x34 + iVar16);
              matrix4x3_transform_point(pfVar15);
              fVar4 = *(float *)(psVar12 + 0xe) * *pfVar15;
            }
            if (((param_2[2] <= local_444 + fVar4 + 0.5) || (param_4[2] <= -0.2)) &&
               (((local_444 - fVar4) - 0.5 <= param_2[2] || (0.2 <= param_4[2])))) {
              fVar5 = local_44c - *param_2;
              fVar6 = local_448 - param_2[1];
              fVar7 = local_444 - param_2[2];
              if (fVar5 * fVar5 + fVar6 * fVar6 + fVar7 * fVar7 * 4.0 <=
                  (fVar4 + param_3) * (fVar4 + param_3)) {
                uVar13 = 0;
                if ((((short)puVar2[0x2d] == 0) &&
                    (0.0 < fVar7 * param_4[2] + fVar5 * *param_4 + fVar6 * param_4[1])) &&
                   (0.06666667 <
                    (float)puVar2[0x1a] * *param_4 +
                    (float)puVar2[0x1b] * param_4[1] + (float)puVar2[0x1c] * param_4[2])) {
                  uVar13 = 1;
                }
                FUN_0043c4b0(uVar1,uVar13,fVar4);
              }
            }
            sVar10 = sVar10 + 1;
            iVar11 = (int)sVar10;
            iVar16 = DAT_0087bc14;
          } while (iVar11 < *(int *)(iVar3 + 0x280));
        }
      }
      local_46c = local_46c + 1;
      local_454 = local_454 - 1;
    } while (local_454 != 0);
  }
  return;
}
#endif
