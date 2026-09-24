// ai_search_append_obstacle  (Ghidra: ai_search_append_obstacle, renamed)
// address 0x43c4b0, size 83 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/ai.h ai_search_obstacle_list (count +0x02, flagged_count +0x04, obstacles
// +0x08 stride 0x14) and ai_search_obstacle (flags/link/object_index/position/radius),
// matching this function's writes exactly.
// register convention: EAX -> (unused garbage passthrough on the full-list path), EDX ->
//   list, ESI -> position; stack -> object_index, flags, radius.
//   // blam-cc: EDX -> list, ESI -> position, stack -> object_index, flags, radius

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: EDX -> list, ESI -> position, stack -> object_index, flags, radius
uint8_t ai_search_append_obstacle(ai_search_obstacle_list *list, uint16_t flags, uint32_t object_index,
                                  real_point2d *position, float radius)
{
    ai_search_obstacle *entry;

    if (list->count == 0x80) {
        return 0;
    }

    entry = &list->obstacles[list->count];
    list->count = list->count + 1;
    if ((flags & 1) != 0) {
        list->flagged_count = list->flagged_count + 1;
    }

    entry->flags = flags;
    entry->object_index = object_index;
    entry->link = -1;
    entry->position = *position;
    entry->radius = radius;
    return 1;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043c4b0 @ 0x43c4b0) ----
uint FUN_0043c4b0(undefined4 param_1,ushort param_2,undefined4 param_3)

{
  ushort *puVar1;
  short sVar2;
  undefined4 uVar3;
  uint in_EAX;
  int in_EDX;
  undefined4 *unaff_ESI;

  sVar2 = *(short *)(in_EDX + 2);
  if (sVar2 == 0x80) {
    return in_EAX & 0xffffff00;
  }
  puVar1 = (ushort *)(in_EDX + 8 + sVar2 * 0x14);
  *(short *)(in_EDX + 2) = sVar2 + 1;
  if ((param_2 & 1) != 0) {
    *(short *)(in_EDX + 4) = *(short *)(in_EDX + 4) + 1;
  }
  *puVar1 = param_2;
  *(undefined4 *)(puVar1 + 2) = param_1;
  puVar1[1] = 0xffff;
  uVar3 = *unaff_ESI;
  *(undefined4 *)(puVar1 + 6) = unaff_ESI[1];
  *(undefined4 *)(puVar1 + 4) = uVar3;
  *(undefined4 *)(puVar1 + 8) = param_3;
  return CONCAT31((int3)((uint)puVar1 >> 8),1);
}
#endif
