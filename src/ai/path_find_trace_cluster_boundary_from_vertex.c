// path_find_trace_cluster_boundary_from_vertex  (Ghidra: path_find_trace_cluster_boundary_from_vertex, renamed)
// address 0x43d790, size 383 bytes
// name confidence: 0.3   rewrite confidence: 0.1
// evidence: phase-4 summary "traces along a cluster's boundary edges from a starting vertex,
// returning the nearest edge that would block a proposed move within range." Reuses the same
// bsp-pointer-at-+0xb4 / request-block-at-+0x1e8 / permission-bitmap shape as
// path_find_trace_cluster_boundary.c and path_find_run.c. Calls collision_bsp_surface_clip_line_2d (outside this
// rewrite's range), whose six float outputs (Ghidra's `local_18/14/10/c/8/4`) this function
// reads with no visible arguments beyond the four shown -- an unrecovered hidden-output
// callee, like several others in this cluster.
//
// register convention: EAX -> context; stack -> ignore_permission, cluster_ref,
//   max_distance, param4, distance, out_result.
//   // blam-cc: EAX -> context, stack -> ignore_permission, cluster_ref, max_distance,
//   //   param4, distance, out_result
//
// UNSURE: this is one of the least confident rewrites in this batch; see the header above
// for the unrecovered callee. `out_scratch` groups collision_bsp_surface_clip_line_2d's six float outputs as
// Ghidra's own local numbering; their real field meanings are not established.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern uint32_t ai_path_permission_table;    // 0x006b8d78, see path_find_run.c
extern int16_t local_command_list_generation; // 0x0069e8d8, see path_find_run.c
extern void collision_bsp_surface_clip_line_2d(int32_t bsp, float distance, uint32_t cluster_ref, uint32_t param4); // 0x5017f0, outside this rewrite's range; hidden outputs, see header

// blam-cc: EAX -> context, stack -> ignore_permission, cluster_ref, max_distance, param4,
//   distance, out_result
uint8_t path_find_trace_cluster_boundary_from_vertex(void *context, uint8_t ignore_permission, uint32_t cluster_ref,
                                                      float max_distance, uint32_t param4, float distance,
                                                      float *out_result)
{
    int32_t bsp = *(int32_t *)((uint8_t *)context + 0xb4);
    uint8_t *permission_flags = (uint8_t *)((uint8_t *)context + 0x1e8);
    uint8_t *permission_row = (uint8_t *)&ai_path_permission_table + local_command_list_generation * 0x20 + 1;
    float out_18, out_14, out_10, out_c, out_8, out_4; // collision_bsp_surface_clip_line_2d's hidden outputs, see header

    for (;;) {
        float advanced_distance = distance;
        collision_bsp_surface_clip_line_2d(bsp, advanced_distance, cluster_ref, param4);

        if ((max_distance < out_18) && (permission_flags[(int32_t)out_10] != 0) &&
            ((ignore_permission != 0) ||
             ((-1 < (int8_t)permission_flags[(int32_t)out_10]) ||
              ((*(uint32_t *)(permission_row + (*(uint8_t *)(*(int32_t *)(bsp + 0x40) +
                                                              (int32_t)out_10 * 0xc + 9) >> 5) * 4) &
                (1u << (*(uint8_t *)(*(int32_t *)(bsp + 0x40) + (int32_t)out_10 * 0xc + 9) & 0x1f))) != 0)))) {
            distance = out_10;
            if (out_10 != -1.0f) {
                continue;
            }
        }

        if ((max_distance <= out_c) || (permission_flags[(int32_t)out_4] == 0) ||
            ((ignore_permission == 0) &&
             ((int8_t)permission_flags[(int32_t)out_4] < 0) &&
             ((*(uint32_t *)(permission_row + (*(uint8_t *)(*(int32_t *)(bsp + 0x40) +
                                                             (int32_t)out_4 * 0xc + 9) >> 5) * 4) &
               (1u << (*(uint8_t *)(*(int32_t *)(bsp + 0x40) + (int32_t)out_4 * 0xc + 9) & 0x1f))) == 0)) ||
            ((distance = out_4), out_4 == -1.0f)) {
            if (max_distance < out_18) {
                out_result[1] = advanced_distance;
                out_result[2] = out_14;
                out_result[0] = out_18;
                return 1;
            }
            if (max_distance <= out_c) {
                out_result[1] = advanced_distance;
                out_result[0] = max_distance;
                out_result[2] = -1.0f;
                return 0;
            }
            out_result[1] = advanced_distance;
            out_result[0] = out_c;
            out_result[2] = out_8;
            return 1;
        }
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d790 @ 0x43d790) ----
undefined4
FUN_0043d790(char param_1,undefined4 param_2,float param_3,undefined4 param_4,float param_5,
            float *param_6)

{
  int iVar1;
  char cVar2;
  byte bVar3;
  int iVar4;
  int iVar5;
  int in_EAX;
  float fVar6;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar4 = *(int *)(in_EAX + 0xb4);
  iVar5 = *(int *)(in_EAX + 0x1e8);
  iVar1 = DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78;
LAB_0043d7ce:
  do {
    fVar6 = param_3;
    FUN_005017f0(iVar4,fVar6,param_2,param_4);
    if (((param_5 < local_18) && (cVar2 = *(char *)((int)local_10 + iVar5), cVar2 != '\0')) &&
       ((param_1 != '\0' ||
        ((-1 < cVar2 ||
         (bVar3 = *(byte *)(*(int *)(iVar4 + 0x40) + (int)local_10 * 0xc + 9),
         (*(uint *)(iVar1 + (uint)(bVar3 >> 5) * 4) & 1 << (bVar3 & 0x1f)) != 0)))))) {
      param_3 = local_10;
      if (local_10 != -NAN) goto LAB_0043d7ce;
    }
    if (((param_5 <= local_c) || (cVar2 = *(char *)((int)local_4 + iVar5), cVar2 == '\0')) ||
       (((param_1 == '\0' &&
         ((cVar2 < '\0' &&
          (bVar3 = *(byte *)(*(int *)(iVar4 + 0x40) + (int)local_4 * 0xc + 9),
          (*(uint *)(iVar1 + (uint)(bVar3 >> 5) * 4) & 1 << (bVar3 & 0x1f)) == 0)))) ||
        (param_3 = local_4, local_4 == -NAN)))) {
      if (param_5 < local_18) {
        param_6[1] = fVar6;
        param_6[2] = local_14;
        *param_6 = local_18;
        return 1;
      }
      if (param_5 <= local_c) {
        param_6[1] = fVar6;
        *param_6 = param_5;
        param_6[2] = -NAN;
        return 0;
      }
      param_6[1] = fVar6;
      *param_6 = local_c;
      param_6[2] = local_8;
      return 1;
    }
  } while( true );
}
#endif
