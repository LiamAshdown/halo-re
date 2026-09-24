// ai_navigate_around_obstacles  (Ghidra: ai_navigate_around_obstacles, renamed)
// address 0x43be90, size 1196 bytes
// name confidence: 0.35  rewrite confidence: 0.1
// evidence: phase-4 summary "top-level AI pathfinding entry point that builds an
// obstacle-aware point graph around a list of waypoints and searches it to produce a full
// route." Calls ai_search_gather_obstacles (0x43c510, already named, this rewrite),
// ai_search_context_init/ai_search_step/ai_search_run (0x43b790/0x43bcb0/0x43be20, this
// rewrite), ai_search_partition_into_groups (this rewrite) and FUN_0044d860 (outside this rewrite's range).
//
// This is the module's top-level point-search driver and, like ai_search_evaluate_edge_cost
// and ai_search_expand_point_neighbors, one of its least confidently rewritten functions.
// It iterates the caller's waypoint list, and for each segment either reuses a cached
// per-waypoint `ai_search_context`-shaped block hanging off a global table (when `param_1[0x12]`
// is a live cache handle) or builds a fresh one on the stack, runs the point search between
// the segment's two endpoints, and copies the resulting smoothed sub-path into the caller's
// output buffer. The exact shape of the cache record (`+0x10588`/`+0x1058a`/`+0x12dac`/
// `+0x1058c` relative to `param_1[0x12]`) and of the caller's own waypoint-list record
// (`param_1[8]`, `param_1+5`, `param_3` as a 0x10-stride array, `param_5`/`param_4` as the
// output cursor) are not established anywhere else in this module and are preserved as raw
// offsets rather than asserted as named fields.
//
// register convention: stack -> the six Ghidra-recognized formal parameters (this function
//   was not observed calling anything through an unresolved register operand of its own,
//   only forwarding its parameters into callees that themselves have register gaps).
//   // blam-cc: stack -> waypoints, waypoint_count, edges, out_cursor, out_buffer, out_flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern int32_t bsp_generation; // 0x00746f9c
extern uint8_t *ai_navigate_cluster_table; // 0x00746f98, UNSURE: a per-cluster table this function reads bsp-cluster edge data from at +0x40/+0x10

extern void ai_search_gather_obstacles(void *out_list, float *point, float radius, float *direction,
                                       uint32_t self_object_a, uint32_t self_object_b); // 0x43c510
extern void ai_search_partition_into_groups(float step_radius); // 0x43cb60
extern void FUN_0044d860(real_point3d *point); // 0x44d860, outside this rewrite's range
extern void ai_search_context_init(void *context, uint8_t param2, float step_radius, float *point,
                                   float z, float distance, uint32_t flags, uint8_t param8); // 0x43b790, see header UNSURE
extern uint8_t ai_search_step(void *context); // 0x43bcb0
extern uint8_t ai_search_run(uint8_t param2, void *obstacle_list, float step_radius, float *point,
                             float distance, float *direction); // 0x43be20, see header UNSURE

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS

// blam-cc: stack -> waypoints, waypoint_count, edges, out_cursor, out_buffer, out_flag
//
// UNSURE: this rewrite is a close, low-confidence transcription; see the file header.
uint8_t ai_navigate_around_obstacles(float *waypoints, int16_t waypoint_count, uint8_t *edges,
                                     int16_t *out_cursor, uint8_t *out_buffer, uint8_t *out_flag)
{
    float scratch_path[512 * 4]; // local_273c, one {surface_z, x, y, slope} record per collected point
    uint8_t obstacle_list[1284 * 2]; // local_1f3c
    uint8_t search_context[5424]; // local_1534
    uint8_t *cluster_base = ai_navigate_cluster_table;
    float step_radius;
    int32_t segment;
    int16_t last_segment;
    float cached_z;
    float cache_handle;

    step_radius = (*waypoints <= 0.2f) ? 0.2f : *waypoints;
    cache_handle = waypoints[0x12];
    if ((cache_handle != 0.0f) && (*(uint8_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x10588) == 0)) {
        *(int16_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x1058a) = 0;
    }

    if (waypoint_count < 1) {
        return 1;
    }
    last_segment = waypoint_count - 1;

    for (segment = 0; ; segment = segment + 1) {
        uint8_t *ctx = search_context;
        void *obstacles = obstacle_list;
        uint8_t is_last_segment;
        float *from_point;
        float from_z;
        float *edge;
        float dx, dy, dz;
        float len;
        float to_z;

        is_last_segment = 0;
        if (segment == last_segment) {
            is_last_segment = (*out_flag != 0) ? 1 : 0;
        }

        if (segment < 1) {
            cached_z = waypoints[8];
            from_point = waypoints + 5;
        } else {
            from_point = &scratch_path[0]; // UNSURE: original reuses `local_2748` across iterations, see header
        }

        edge = (float *)(edges + segment * 0x10);
        dy = edge[1] - from_point[0];
        dz = edge[2] - from_point[1];
        from_z = *edge;
        dx = edge[3] - from_point[2]; // UNSURE: axis order preserved exactly, see original
        len = (float)sqrt(dy * dy + dx * dx + dz * dz);
        if (0.0001 <= fabs(len)) {
            float inv = 1.0f / len;
            dy = dy * inv;
            dz = dz * inv;
            dx = dx * inv;
        }
        to_z = from_z; // UNSURE: `local_275c = local_2774`, see original

        if (cache_handle == 0.0f) {
            /* fresh, uncached search */
        } else if ((*(uint8_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x10588) == 0) ||
                  (*(int16_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x1058a) <= (int16_t)segment)) {
            /* cache miss: fall back to a fresh search, same as cache_handle == 0 */
        } else {
            ctx = (uint8_t *)(uintptr_t)(uint32_t)cache_handle + segment * 0x1534 + 0x12dac;
            obstacles = (uint8_t *)(uintptr_t)(uint32_t)cache_handle + segment * 0xa08 + 0x1058c;
            goto have_context;
        }

        {
            uint16_t *hdr = (uint16_t *)obstacles;
            hdr[0] = 0; hdr[1] = 0; hdr[2] = 0;
            ai_search_gather_obstacles(obstacles, from_point, 4.0f, &dy,
                                       *(uint32_t *)(waypoints + 2), *(uint32_t *)(waypoints + 3));
            if (*(uint8_t *)(waypoints + 9) != 0) {
                int16_t count = ((int16_t *)obstacles)[1];
                if (count != 0x80) {
                    float radius = waypoints[0xd];
                    float weight = waypoints[0xe];
                    uint16_t *entry = (uint16_t *)obstacles + count * 10 + 4;
                    ((int16_t *)obstacles)[2] = ((int16_t *)obstacles)[2] + 1;
                    ((int16_t *)obstacles)[1] = count + 1;
                    entry[0] = 1;
                    entry[1] = 0xffff;
                    *(float *)(entry + 2) = radius;
                    *(float *)(entry + 4) = waypoints[10];
                    *(float *)(entry + 6) = waypoints[0xb];
                    *(float *)(entry + 8) = weight;
                }
            }
            ai_search_partition_into_groups(step_radius);
            if ((cache_handle != 0.0f) && (*(uint8_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x10588) == 0)) {
                *(int16_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x1058a) =
                    *(int16_t *)((uint8_t *)(uintptr_t)(uint32_t)cache_handle + 0x1058a) + 1;
            }
        }

    have_context:
        ai_search_context_init(ctx, *(uint8_t *)(waypoints + 1), step_radius, from_point, to_z, from_z, 0, 0);
        while (ai_search_step(ctx) != 0) {
        }

        if (*(int16_t *)(ctx + 0x1e) == -1) {
            if (*(int16_t *)(ctx + 0x20) != -1) {
                *(int16_t *)(ctx + 0x1e) = *(int16_t *)(ctx + 0x20);
            }
        } else {
            ctx[0x28] = 1;
        }

        if ((*(int16_t *)(ctx + 0x1e) == -1) &&
            ((((int16_t *)obstacles)[2] < 1) ||
             (ai_search_run(*(uint8_t *)(waypoints + 1), obstacles, step_radius, from_point, to_z, edge + 1) == 0))) {
            return 0;
        }

        {
            uint8_t reached_end = 0;
            float collected[512][4];
            int32_t collected_count = 0;
            int16_t cur;

            if (ctx[0x28] == 0) {
                cached_z = *(float *)(ctx + *(int16_t *)(ctx + 0x1e) * 0x28 + 0x38);
                FUN_0044d860((real_point3d *)&scratch_path[0]);
            } else {
                scratch_path[0] = edge[1];
                scratch_path[1] = edge[2];
                scratch_path[2] = edge[3];
                cached_z = from_z;
            }

            cur = *(int16_t *)(ctx + 0x1e);
            while (cur != 0) {
                float *node = (float *)(ctx + cur * 0x28 + 0x30);
                float surface_z = node[2];
                int32_t table_a = *(int32_t *)(*(int32_t *)(cluster_base + 0x40) + (int32_t)surface_z * 0xc);
                float *cluster_edge = (float *)(table_a * 0x10 + *(int32_t *)(cluster_base + 0x10));
                float slope;

                collected[collected_count][0] = surface_z;
                collected[collected_count][1] = *node;
                collected[collected_count][2] = node[1];
                if (0.0001 <= fabs(cluster_edge[2])) {
                    slope = ((cluster_edge[3] - *node * *cluster_edge) - cluster_edge[1] * node[1]) / cluster_edge[2];
                } else {
                    slope = 0.0f;
                }
                collected[collected_count][3] = slope;

                collected_count = collected_count + 1;
                if (0x80 <= collected_count) {
                    break;
                }
                cur = *(int16_t *)(node + 9);
            }
            if (collected_count < 0x80) {
                reached_end = 1;
            }

            {
                int16_t cursor = *out_cursor;
                int32_t i = collected_count - 1;
                while (-1 < i) {
                    if (3 < cursor) {
                        reached_end = 1;
                        break;
                    }
                    {
                        float *slot = (float *)(out_buffer + cursor * 0x10);
                        slot[0] = collected[i][0];
                        slot[1] = collected[i][1];
                        slot[2] = collected[i][2];
                        slot[3] = collected[i][3];
                    }
                    cursor = cursor + 1;
                    i = i - 1;
                }
                *out_cursor = cursor;
            }

            if (reached_end) {
                *out_flag = 0;
                return 1;
            }
        }

        if (waypoint_count <= (int16_t)(segment + 1)) {
            return 1;
        }
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043be90 @ 0x43be90) ----
/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4
FUN_0043be90(float *param_1,short param_2,int param_3,short *param_4,int param_5,char *param_6)

{
  undefined2 *puVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  char cVar5;
  float *pfVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  float fVar10;
  undefined1 *puVar11;
  undefined2 *puVar12;
  int iVar13;
  float *pfVar14;
  short sVar15;
  float local_2774;
  float local_2770;
  float local_276c;
  float local_2768;
  float local_2764;
  int local_2760;
  float local_275c;
  uint local_2758;
  float local_2754;
  int local_2750;
  int local_274c;
  float local_2748;
  float local_2744;
  float local_2740;
  float local_273c [512];
  undefined2 local_1f3c [1284];
  undefined1 local_1534 [5424];
  undefined4 uStack_4;

  uStack_4 = 0x43be9a;
  local_2750 = DAT_00746f98;
  if (*param_1 <= 0.2) {
    local_2764 = 0.2;
  }
  else {
    local_2764 = *param_1;
  }
  fVar10 = param_1[0x12];
  if ((fVar10 != 0.0) && (*(char *)((int)fVar10 + 0x10588) == '\0')) {
    *(undefined2 *)((int)fVar10 + 0x1058a) = 0;
  }
  local_2760 = 0;
  if (param_2 < 1) {
    return 1;
  }
  local_274c = param_2 + -1;
  do {
    sVar15 = (short)local_2760;
    iVar7 = (int)sVar15;
    puVar12 = local_1f3c;
    puVar11 = local_1534;
    if (iVar7 == local_274c) {
      local_2758 = CONCAT31(local_2758._1_3_,1);
      if (*param_6 == '\0') goto LAB_0043bf41;
    }
    else {
LAB_0043bf41:
      local_2758 = local_2758 & 0xffffff00;
    }
    if (sVar15 < 1) {
      local_2774 = param_1[8];
      pfVar14 = param_1 + 5;
    }
    else {
      pfVar14 = &local_2748;
    }
    pfVar6 = (float *)(iVar7 * 0x10 + param_3);
    local_2770 = pfVar6[1] - *pfVar14;
    fVar10 = *pfVar6;
    local_276c = pfVar6[2] - pfVar14[1];
    local_2768 = pfVar6[3] - pfVar14[2];
    fVar2 = SQRT(local_2770 * local_2770 + local_2768 * local_2768 + local_276c * local_276c);
    if (0.0001 <= ABS(fVar2)) {
      fVar2 = 1.0 / fVar2;
      local_2770 = local_2770 * fVar2;
      local_276c = local_276c * fVar2;
      local_2768 = local_2768 * fVar2;
    }
    fVar2 = param_1[0x12];
    local_275c = local_2774;
    if (fVar2 == 0.0) {
LAB_0043c027:
      *puVar12 = 0;
      puVar12[1] = 0;
      puVar12[2] = 0;
      ai_search_gather_obstacles(puVar12,pfVar14,0x40800000,&local_2770,param_1[2],param_1[3]);
      if (*(char *)(param_1 + 9) != '\0') {
        local_2754 = param_1[0xd];
        fVar2 = param_1[0xe];
        sVar15 = puVar12[1];
        if (sVar15 != 0x80) {
          puVar12[2] = puVar12[2] + 1;
          puVar12[1] = sVar15 + 1;
          puVar1 = puVar12 + sVar15 * 10 + 4;
          *puVar1 = 1;
          puVar1[1] = 0xffff;
          *(float *)(puVar1 + 2) = local_2754;
          fVar3 = param_1[10];
          *(float *)(puVar1 + 6) = param_1[0xb];
          *(float *)(puVar1 + 4) = fVar3;
          *(float *)(puVar1 + 8) = fVar2;
        }
      }
      FUN_0043cb60(local_2764);
      fVar2 = param_1[0x12];
      if ((fVar2 != 0.0) && (*(char *)((int)fVar2 + 0x10588) == '\0')) {
        *(short *)((int)fVar2 + 0x1058a) = *(short *)((int)fVar2 + 0x1058a) + 1;
      }
    }
    else {
      puVar11 = (undefined1 *)(iVar7 * 0x1534 + 0x12dac + (int)fVar2);
      puVar12 = (undefined2 *)(iVar7 * 0xa08 + 0x1058c + (int)fVar2);
      if ((*(char *)((int)fVar2 + 0x10588) == '\0') || (*(short *)((int)fVar2 + 0x1058a) <= sVar15))
      goto LAB_0043c027;
    }
    FUN_0043b790(puVar11,*(undefined1 *)(param_1 + 1),local_2764,pfVar14,local_275c,fVar10,
                 local_2758,0);
    do {
      cVar5 = FUN_0043bcb0();
    } while (cVar5 != '\0');
    if (*(short *)(puVar11 + 0x1e) == -1) {
      if (*(short *)(puVar11 + 0x20) != -1) {
        *(short *)(puVar11 + 0x1e) = *(short *)(puVar11 + 0x20);
      }
    }
    else {
      puVar11[0x28] = 1;
    }
    if ((*(short *)(puVar11 + 0x1e) == -1) &&
       (((short)puVar12[2] < 1 ||
        (cVar5 = FUN_0043be20(*(undefined1 *)(param_1 + 1),puVar12,local_2764,pfVar14,local_275c,
                              pfVar6 + 1), cVar5 == '\0')))) {
      return 0;
    }
    bVar4 = false;
    local_2754 = 0.0;
    if (puVar11[0x28] == '\0') {
      local_2774 = *(float *)(puVar11 + *(short *)(puVar11 + 0x1e) * 0x28 + 0x38);
      FUN_0044d860(&local_2748);
    }
    else {
      local_2748 = pfVar6[1];
      local_2744 = pfVar6[2];
      local_2740 = pfVar6[3];
      local_2774 = fVar10;
    }
    sVar15 = *(short *)(puVar11 + 0x1e);
    fVar10 = local_2754;
    do {
      if (sVar15 == 0) goto LAB_0043c292;
      pfVar14 = (float *)(puVar11 + sVar15 * 0x28 + 0x30);
      fVar2 = pfVar14[2];
      iVar13 = (int)SUB42(fVar10,0);
      local_273c[iVar13 * 4] = fVar2;
      iVar7 = *(int *)(*(int *)(local_2750 + 0x40) + (int)fVar2 * 0xc);
      iVar8 = *(int *)(local_2750 + 0x10);
      local_273c[iVar13 * 4 + 1] = *pfVar14;
      pfVar6 = (float *)(iVar7 * 0x10 + iVar8);
      local_273c[iVar13 * 4 + 2] = pfVar14[1];
      fVar10 = (float)((int)fVar10 + 1);
      if (0.0001 <= ABS(pfVar6[2])) {
        fVar2 = ((pfVar6[3] - *pfVar14 * *pfVar6) - pfVar6[1] * pfVar14[1]) / pfVar6[2];
      }
      else {
        fVar2 = 0.0;
      }
      local_273c[iVar13 * 4 + 3] = fVar2;
      sVar15 = *(short *)(pfVar14 + 9);
    } while (SUB42(fVar10,0) < 0x80);
    bVar4 = true;
LAB_0043c292:
    sVar15 = *param_4;
    iVar7 = (int)fVar10 + -1;
    sVar9 = (short)iVar7;
    while (-1 < sVar9) {
      if (3 < sVar15) {
        bVar4 = true;
        break;
      }
      iVar8 = (int)(short)iVar7;
      pfVar14 = (float *)(sVar15 * 0x10 + param_5);
      *pfVar14 = local_273c[iVar8 * 4];
      pfVar14[1] = local_273c[iVar8 * 4 + 1];
      fVar10 = local_273c[iVar8 * 4 + 3];
      sVar15 = sVar15 + 1;
      iVar7 = iVar7 + -1;
      pfVar14[2] = local_273c[iVar8 * 4 + 2];
      pfVar14[3] = fVar10;
      sVar9 = (short)iVar7;
    }
    *param_4 = sVar15;
    if (bVar4) {
      *param_6 = '\0';
      return 1;
    }
    local_2760 = local_2760 + 1;
    if (param_2 <= (short)local_2760) {
      return 1;
    }
  } while( true );
}
#endif
