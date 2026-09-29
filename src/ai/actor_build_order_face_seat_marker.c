// actor_build_order_face_seat_marker  (Ghidra: actor_build_order_face_seat_marker, renamed)
// address 0x408110, size 193 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x408110..0x4081d0 (EAX actor, EDX order, stack firing position).)
// evidence: same evidence as actor_build_order_face_seat_marker_committed.c (0x407820),
//   which this is a near-duplicate of; phase-4 summary "like
//   actor_build_order_face_seat_marker_committed but without committing the actor to it".
// register convention: actor index in EAX, order pointer in EDX, a firing-position index in
//   the recognized stack parameter.
//   // blam-cc: EAX -> actor_index, EDX -> order, stack -> firing_position_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "fn_ai.h"

extern data_array *actor_data;    // 0x00880360
extern Scenario *global_scenario; // 0x00746f8c

uint32_t actor_build_order_face_seat_marker(uint32_t actor_index, int16_t firing_position_index, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;
    uint32_t result = 0;

    for (i = 0xd; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0 && a->swarm == 0 && a->encounter_index != (datum_index)k_datum_index_none && firing_position_index != -1) {
        ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
        ScenarioFiringPosition *fp = &((ScenarioFiringPosition *)encounters[a->encounter_index & 0xffff].firing_positions.pointer)[firing_position_index];

        *(int16_t *)((uint8_t *)order + 0xa) = firing_position_index;
        *(int16_t *)(order + 2) = 1;
        order[5] = *(uint32_t *)&fp->position.x;
        order[6] = *(uint32_t *)&fp->position.y;
        order[7] = *(uint32_t *)&fp->position.z;
        order[4] = *(uint32_t *)&fp->surface_index;
        *(uint16_t *)(order + 3) = fp->cluster_index;
        *((uint8_t *)(order + 8)) = 0; // byte offset 0x20 (dword-scaled, per the original's `in_EDX + 8`)
        *((uint8_t *)order + 3) = 1;    // true byte offset 3 (the original casts via (int) first)
        a->unknown_98 = 1;
        return 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x408110):

uint FUN_00408110(short param_1)

{
  undefined2 uVar1;
  uint in_EAX;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined4 *in_EDX;
  undefined4 *puVar5;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar5 = in_EDX;
  for (iVar4 = 0xd; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  uVar3 = 0;
  if ((((*(char *)(iVar2 + 0x160) == '\0') && (uVar3 = 0, *(char *)(iVar2 + 6) == '\0')) &&
      (uVar3 = *(uint *)(iVar2 + 0x34), uVar3 != 0xffffffff)) && (param_1 != -1)) {
    puVar5 = (undefined4 *)
             (*(int *)((uVar3 & 0xffff) * 0xb0 + 0x9c + *(int *)(global_scenario + 0x430)) +
             param_1 * 0x18);
    *(short *)((int)in_EDX + 10) = param_1;
    *(undefined2 *)(in_EDX + 2) = 1;
    in_EDX[5] = *puVar5;
    in_EDX[6] = puVar5[1];
    in_EDX[7] = puVar5[2];
    in_EDX[4] = puVar5[5];
    uVar1 = *(undefined2 *)((int)puVar5 + 0xe);
    *(undefined2 *)(in_EDX + 3) = uVar1;
    *(undefined1 *)(in_EDX + 8) = 0;
    *(undefined1 *)((int)in_EDX + 3) = 1;
    *(undefined1 *)(iVar2 + 0x98) = 1;
    return CONCAT31((int3)(CONCAT22((short)((uint)puVar5 >> 0x10),uVar1) >> 8),1);
  }
  return uVar3 & 0xffffff00;
}
#endif
