// ai_search_find_nearest_visible_point  (Ghidra: ai_search_find_nearest_visible_point, renamed)
// address 0x43c8f0, size 163 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (objdump 0x43c8f0..0x43c992: param 3/4 are the ray origin and
//   direction, and the max distance slot receives the ray distance)
// evidence: types/ai.h ai_search_obstacle_list.count(+0x02)/obstacles(+0x08, stride 0x14)
// and ai_search_obstacle.flags(+0x00)/link(+0x02)/radius(+0x10). phase-4 summary "finds the
// closest point in the point array visible along a given direction, optionally excluding
// flagged points, returning its distance and id." Calls ray2d_intersect_circle_distance (a math helper this
// task's skip list excludes from rewriting, "ray-versus-circle intersection test... returning
// the entry distance along the ray when it hits" per phase-4).
// VERIFIED against disassembly 0x43c8f0..0x43c992 (2026-09-30): the ray distance slot IS the max_distance argument slot (passed to
// the callee in ESI, and never reset between candidates); a candidate is accepted when result->distance > that slot.
//
// register convention: EDI -> out_result (distance + point id + link); stack -> list,
//   exclude_index, param_3, param_4, radius, max_distance, require_unflagged.
//   // blam-cc: EDI -> out_result, stack -> list, exclude_index, param_3, param_4, radius,
//   //   max_distance, require_unflagged

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t ray2d_intersect_circle_distance(const real_vector2d *direction, const real_point2d *origin,
    const real_point2d *center, real *out_distance, real radius); // 0x43c380, EAX, ECX, EDX, ESI, stack

// blam-cc: EDI -> out_result, stack -> list, exclude_index, param_3, param_4, radius,
//   max_distance, require_unflagged
uint8_t ai_search_find_nearest_visible_point(ai_search_obstacle_list *list, int16_t exclude_index,
                                             real_point2d *origin, real_vector2d *direction, float radius,
                                             float max_distance, uint8_t require_unflagged,
                                             ai_search_nearest_point_result *out_result)
{
    float distance = max_distance; // 0x43c942: the max_distance slot doubles as the ray's out distance
    int16_t i;

    out_result->distance = max_distance;
    out_result->point_id = -1;
    out_result->link = -1;
    for (i = 0; i < list->count; i++) {
        ai_search_obstacle *obstacle = &list->obstacles[i];

        if (i == exclude_index || (require_unflagged && (obstacle->flags & 1) != 0)) {
            continue;
        }
        if (ray2d_intersect_circle_distance(direction, origin, &obstacle->position, &distance,
                radius + obstacle->radius) != 0 &&
            out_result->distance > distance) {
            out_result->distance = distance;
            out_result->point_id = i;
            out_result->link = obstacle->link;
        }
    }
    return (uint8_t)(out_result->point_id != -1);
}

#if 0
// ---- original Ghidra decompilation (FUN_0043c8f0 @ 0x43c8f0) ----
undefined4
FUN_0043c8f0(int param_1,short param_2,undefined4 param_3,undefined4 param_4,float param_5,
            float param_6,char param_7)

{
  byte *pbVar1;
  char cVar2;
  short sVar3;
  float *unaff_EDI;

  *unaff_EDI = param_6;
  sVar3 = 0;
  *(undefined2 *)(unaff_EDI + 1) = 0xffff;
  *(undefined2 *)((int)unaff_EDI + 6) = 0xffff;
  if (0 < *(short *)(param_1 + 2)) {
    do {
      if (((sVar3 != param_2) &&
          (((pbVar1 = (byte *)(param_1 + 8 + sVar3 * 0x14), param_7 == '\0' || ((*pbVar1 & 1) == 0))
           && (cVar2 = FUN_0043c380(param_5 + *(float *)(pbVar1 + 0x10)), cVar2 != '\0')))) &&
         (param_6 < *unaff_EDI)) {
        *unaff_EDI = param_6;
        *(short *)(unaff_EDI + 1) = sVar3;
        *(undefined2 *)((int)unaff_EDI + 6) = *(undefined2 *)(pbVar1 + 2);
      }
      sVar3 = sVar3 + 1;
    } while (sVar3 < *(short *)(param_1 + 2));
  }
  return CONCAT31(0xffffff,*(short *)(unaff_EDI + 1) != -1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
