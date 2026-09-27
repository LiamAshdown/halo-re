// ai_search_find_covering_point  (Ghidra: ai_search_find_covering_point, renamed)
// address 0x43c890, size 95 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (verified against objdump)
// evidence: types/ai.h ai_search_obstacle_list.count(+0x02)/obstacles(+0x08, stride 0x14)
// and ai_search_obstacle.position(+0x08)/radius(+0x10). phase-4 summary "finds a point in
// the search's fixed point array whose coverage radius reaches a given position, or -1 if
// none does." Called by ai_search_context_init.c.
// register convention: ESI -> list, EDX -> position, BX -> exclude_index; stack -> extra_radius.
//   // blam-cc: ESI -> list, EDX -> position, EBX -> exclude_index, stack -> extra_radius

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: ESI -> list, EDX -> position, EBX -> exclude_index, stack -> extra_radius
int16_t ai_search_find_covering_point(ai_search_obstacle_list *list, real_point2d *position,
                                      int16_t exclude_index, float extra_radius)
{
    int16_t i;

    for (i = 0; i < list->count; i = i + 1) {
        if (i != exclude_index) {
            float radius = extra_radius + list->obstacles[i].radius;
            float dx = list->obstacles[i].position.x - position->x;
            float dy = list->obstacles[i].position.y - position->y;
            if (dy * dy + dx * dx <= radius * radius) {
                return i;
            }
        }
    }
    return -1;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043c890 @ 0x43c890) ----
short FUN_0043c890(float param_1)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  float *in_EDX;
  short unaff_BX;
  int unaff_ESI;

  sVar5 = 0;
  if (0 < *(short *)(unaff_ESI + 2)) {
    do {
      if ((sVar5 != unaff_BX) &&
         (iVar1 = unaff_ESI + 8 + sVar5 * 0x14, fVar2 = param_1 + *(float *)(iVar1 + 0x10),
         fVar3 = *(float *)(iVar1 + 8) - *in_EDX, fVar4 = *(float *)(iVar1 + 0xc) - in_EDX[1],
         fVar4 * fVar4 + fVar3 * fVar3 <= fVar2 * fVar2)) {
        return sVar5;
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < *(short *)(unaff_ESI + 2));
  }
  return -1;
}
#endif
