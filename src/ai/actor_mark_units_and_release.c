// actor_mark_units_and_release  (Ghidra: actor_mark_units_and_release, renamed)
// address 0x4289c0, size 235 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: types/ai.h actor.swarm(0x06)/unit_index(0x18)/cluster_unit_index(0x24)/
//   encounter_index(0x34); types/objects.h object.vitality_flags (0x106); types/units.h
//   unit_data.swarm_next_unit_index (object+0x1fc). Calls actor_unlink_unit (0x427bc0),
//   actor_remove_from_unit_cluster (0x427c90) and actor_delete (0x427e60), all already
//   rewritten in this module, plus encounter_recompute_morale (outside this rewrite's range, UNSURE
//   signature, same call as actor_release_from_cluster_or_delete @0x428e50).
// register convention: EAX (AL) -> use_alternate_flag, stack -> actor_index, suppress_release.
//   // blam-cc: EAX -> use_alternate_flag, stack -> actor_index, suppress_release

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern void actor_unlink_unit(datum_index actor_index); // 0x427bc0
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60
extern void encounter_recompute_morale(datum_index encounter_index); // 0x437940, UNSURE signature, not in this rewrite range

// blam-cc: EAX -> use_alternate_flag, stack -> actor_index, suppress_release
// Marks the actor's (or each swarm member's) unit object with a vitality-flags status bit
// (0x20, or 0x40 when use_alternate_flag is set) and, unless suppress_release is set,
// deactivates and frees the actor via the shared teardown helpers.
void actor_mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index encounter_index = self->encounter_index;

    if (self->swarm == 0) {
        object *unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
        uint8_t *flags = (uint8_t *)unit_object + 0x106;
        *flags |= use_alternate_flag == 0 ? 0x20 : 0x40;

        if (suppress_release != 0) {
            return;
        }
        actor_unlink_unit(actor_index);
    } else {
        datum_index unit_index = self->cluster_unit_index;
        while (unit_index != (datum_index)k_datum_index_none) {
            object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
            uint8_t *flags = (uint8_t *)unit_object + 0x106;
            *flags |= use_alternate_flag == 0 ? 0x20 : 0x40;

            if (suppress_release == 0) {
                actor_remove_from_unit_cluster(actor_index, unit_index);
            }
            unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc);
        }
        if (suppress_release != 0) {
            return;
        }
    }

    actor_delete(actor_index, 1);
    if (encounter_index != (datum_index)k_datum_index_none) {
        encounter_recompute_morale(encounter_index);
    }
}

#if 0
Original Ghidra decompilation (0x4289c0):

void FUN_004289c0(uint param_1,char param_2)

{
  byte *pbVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  char in_AL;
  int iVar5;
  int iVar6;

  iVar4 = DAT_008603b0;
  iVar5 = (param_1 & 0xffff) * 0x724;
  iVar6 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  iVar2 = *(int *)(iVar6 + 0x34);
  if (*(char *)(iVar5 + 6 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar6 + 0x18) & 0xffff) * 0xc);
    if (in_AL == '\0') {
      pbVar1 = (byte *)(iVar4 + 0x106);
      *pbVar1 = *pbVar1 | 0x20;
    }
    else {
      pbVar1 = (byte *)(iVar4 + 0x106);
      *pbVar1 = *pbVar1 | 0x40;
    }
    if (param_2 != '\0') {
      return;
    }
    actor_unlink_unit();
  }
  else {
    uVar3 = *(uint *)(iVar6 + 0x24);
    while (uVar3 != 0xffffffff) {
      iVar5 = *(int *)(*(int *)(iVar4 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
      if (in_AL == '\0') {
        *(byte *)(iVar5 + 0x106) = *(byte *)(iVar5 + 0x106) | 0x20;
      }
      else {
        *(byte *)(iVar5 + 0x106) = *(byte *)(iVar5 + 0x106) | 0x40;
      }
      if (param_2 == '\0') {
        actor_remove_from_unit_cluster(uVar3);
      }
      uVar3 = *(uint *)(iVar5 + 0x1fc);
    }
    if (param_2 != '\0') {
      return;
    }
  }
  actor_delete(1);
  if (iVar2 != -1) {
    FUN_00437940(iVar2);
  }
  return;
}
#endif
