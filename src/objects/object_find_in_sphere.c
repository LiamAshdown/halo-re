// object_find_in_sphere  (Ghidra: object_find_in_sphere, already named)
// address 0x4f6fe0, size 400 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Finds objects of a given type mask whose bounding sphere
//   intersects a query sphere, using cluster data to narrow the search")
// rewrite confidence: 0.5
// evidence: types/objects.h object (bounding_center 0x0a0, bounding_radius 0x0ac, type 0x0b4);
//   global 0x008603b0 object_data; callees object_collect_in_clusters (0x4f7180, this batch),
//   cluster_flood_fill_within_radius (0x554d10, foreign module).
// register convention: all seven parameters are plain STACK arguments (Ghidra's own
//   "object_find_in_sphere(undefined4 param_1,uint param_2,int param_3,float *param_4,
//   float param_5,int param_6,short param_7)"); no register hints appear anywhere in the
//   decompile, and the huge local stack frame (alloca_probe/__chkstk) is consistent with a
//   plain-stack __cdecl entry.
// UNSURE: param_1 is forwarded verbatim to object_collect_in_clusters as its search_mask
//   (collideable/noncollideable selector bits), not otherwise interpreted here. param_3 is a
//   generic {leaf_index int32, cluster_index int16} location pair (the same shape as
//   object.location_leaf_index/location_cluster_index and damage_data's matching fields), read
//   only for its cluster_index half.
// UNSURE: cluster_flood_fill_within_radius (0x554d10) and its DAT_006e3f01/DAT_006e3f04
//   globals belong to a foreign (cluster/BSP) module and are not otherwise examined here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern uint8_t cluster_flood_in_progress; // 0x006e3f01, foreign module, UNSURE
extern int32_t cluster_flood_stamp; // 0x006e3f04, foreign module, UNSURE

extern int16_t object_collect_in_clusters(uint32_t search_mask, int16_t cluster_count,
    int16_t *cluster_indices, int16_t max_output, datum_index *out_objects); // 0x4f7180, this batch
extern int16_t cluster_flood_fill_within_radius(int16_t start_cluster, real_point3d *center,
    float radius, int16_t max_clusters, int16_t *out_clusters); // 0x554d10, foreign module, UNSURE

int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output)
{
    int16_t clusters[512];
    datum_index candidates[0x800];
    int16_t cluster_count;
    int16_t candidate_count;
    int16_t result_count = 0;
    int16_t i;
    int16_t start_cluster;

    if (type_mask == 0) {
        type_mask = 0xffffffff;
    }

    start_cluster = *(int16_t *)((uint8_t *)location + 4);
    cluster_count = 0;
    if (start_cluster != -1) {
        if (radius <= 0.0f) {
            cluster_count = 1;
            clusters[0] = start_cluster;
        } else {
            cluster_flood_stamp++;
            cluster_flood_in_progress = 1;
            cluster_count = cluster_flood_fill_within_radius(start_cluster, center, radius, 0x200, clusters);
            cluster_flood_in_progress = 0;
        }
    }

    candidate_count = object_collect_in_clusters(search_mask, cluster_count, clusters, 0x800, candidates);

    for (i = 0; i < candidate_count; i++) {
        object *obj;
        if (max_output <= result_count) {
            return result_count;
        }
        obj = ((object_header *)object_data->data)[candidates[i] & 0xffff].data;
        if ((type_mask & (1u << (obj->type & 0x1f))) != 0) {
            float sum_radius = radius + obj->bounding_radius;
            float dx = obj->bounding_center.x - center->x;
            float dy = obj->bounding_center.y - center->y;
            float dz = obj->bounding_center.z - center->z;
            if (dy * dy + dz * dz + dx * dx <= sum_radius * sum_radius) {
                out_objects[result_count] = candidates[i];
                result_count++;
            }
        }
    }

    return result_count;
}

#if 0
Original Ghidra decompilation (0x4f6fe0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

short object_find_in_sphere
                (undefined4 param_1,uint param_2,int param_3,float *param_4,float param_5,
                int param_6,short param_7)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  short sVar6;
  int iVar7;
  short sVar8;
  undefined4 uVar9;
  short sVar10;
  short sVar11;
  uint auStackY_22000 [32501];
  short local_2400 [512];
  uint local_2000 [2047];
  undefined4 uStack_4;

  uStack_4 = 0x4f6fea;
  sVar10 = 0;
  sVar6 = 0;
  if (param_2 == 0) {
    param_2 = 0xffffffff;
  }
  sVar8 = *(short *)(param_3 + 4);
  uVar9 = 0;
  if (sVar8 != -1) {
    if (param_5 <= 0.0) {
      uVar9 = 1;
      local_2400[0] = sVar8;
    }
    else {
      DAT_006e3f04 = DAT_006e3f04 + 1;
      DAT_006e3f01 = 1;
      uVar9 = cluster_flood_fill_within_radius(sVar8,param_4,param_5,0x200,local_2400);
      DAT_006e3f01 = 0;
    }
  }
  sVar8 = object_collect_in_clusters(param_1,uVar9,local_2400,0x800,local_2000);
  iVar7 = DAT_008603b0;
  sVar11 = 0;
  if (0 < sVar8) {
    do {
      if (param_7 <= sVar6) {
        return sVar6;
      }
      iVar1 = *(int *)(*(int *)(iVar7 + 0x34) + 8 + (local_2000[sVar11] & 0xffff) * 0xc);
      sVar10 = sVar6;
      if (((param_2 & 1 << (*(byte *)(iVar1 + 0xb4) & 0x1f)) != 0) &&
         (fVar2 = param_5 + *(float *)(iVar1 + 0xac), fVar3 = *(float *)(iVar1 + 0xa0) - *param_4,
         fVar5 = *(float *)(iVar1 + 0xa4) - param_4[1],
         fVar4 = *(float *)(iVar1 + 0xa8) - param_4[2],
         fVar5 * fVar5 + fVar4 * fVar4 + fVar3 * fVar3 <= fVar2 * fVar2)) {
        *(uint *)(param_6 + sVar6 * 4) = local_2000[sVar11];
        sVar10 = sVar6 + 1;
      }
      sVar11 = sVar11 + 1;
      sVar6 = sVar10;
    } while (sVar11 < sVar8);
  }
  return sVar10;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
