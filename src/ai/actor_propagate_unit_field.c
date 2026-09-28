// actor_propagate_unit_field  (Ghidra: actor_propagate_unit_field, renamed)
// address 0x4276e0, size 214 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4276e0..0x4277bc (EAX actor, SI value).)
// evidence: types/ai.h actor.swarm(0x06)/unit_index(0x18)/swarm_index(0x28)/
//   cluster_unit_index(0x24); types/units.h unit_data.swarm_next_unit_index (object+0x1fc).
//   Phase-4 summary: "Propagates a caller-provided value into the object-header field of
//   every unit the actor (or its cluster/swarm) currently controls." The destination field
//   (object+0xb8) is the same unnamed one actor_attach_to_unit and
//   actor_link_to_unit_cluster write from an encounter's team; see those files' UNSURE notes.
// register convention: EAX -> actor_index, SI -> value (unaff_ESI).
//   // blam-cc: EAX -> actor_index, ESI -> value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern data_array *swarm_data; // 0x0088035c
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> actor_index, ESI -> value
// Writes `value` into the unnamed int16 field at object+0xb8 (see UNSURE above) of every unit
// the actor controls: its single unit, every unit in its cluster, or every unit in its
// swarm's component list.
void actor_propagate_unit_field(datum_index actor_index, int16_t value)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm == 0) {
        if (self->unit_index != (datum_index)k_datum_index_none) {
            object *unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
            *(int16_t *)((uint8_t *)unit_object + 0xb8) = value;
        }
    } else if (self->swarm_index == (datum_index)k_datum_index_none) {
        datum_index unit_index = self->cluster_unit_index;
        if (unit_index != (datum_index)k_datum_index_none) {
            do {
                object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
                *(int16_t *)((uint8_t *)unit_object + 0xb8) = value;
                unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc); // unit_data.swarm_next_unit_index
            } while (unit_index != (datum_index)k_datum_index_none);
        }
    } else {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            object *unit_object = ((object_header *)object_data->data)[s->unit_index[i] & 0xffff].data;
            *(int16_t *)((uint8_t *)unit_object + 0xb8) = value;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4276e0):

void FUN_004276e0(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  uint uVar4;
  short sVar5;
  undefined2 unaff_SI;

  iVar1 = DAT_008603b0;
  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar2 + 6) == '\0') {
    if (*(uint *)(iVar2 + 0x18) != 0xffffffff) {
      *(undefined2 *)
       (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar2 + 0x18) & 0xffff) * 0xc) +
       0xb8) = unaff_SI;
    }
  }
  else if (*(uint *)(iVar2 + 0x28) == 0xffffffff) {
    uVar4 = *(uint *)(iVar2 + 0x24);
    if (uVar4 != 0xffffffff) {
      do {
        iVar2 = *(int *)(*(int *)(iVar1 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
        *(undefined2 *)(iVar2 + 0xb8) = unaff_SI;
        uVar4 = *(uint *)(iVar2 + 0x1fc);
      } while (uVar4 != 0xffffffff);
      return;
    }
  }
  else {
    iVar2 = (*(uint *)(iVar2 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    sVar5 = 0;
    if (0 < *(short *)(iVar2 + 2)) {
      do {
        iVar3 = (int)sVar5;
        sVar5 = sVar5 + 1;
        *(undefined2 *)
         (*(int *)(*(int *)(iVar1 + 0x34) + 8 + (*(uint *)(iVar2 + 0x18 + iVar3 * 4) & 0xffff) * 0xc
                  ) + 0xb8) = unaff_SI;
      } while (sVar5 < *(short *)(iVar2 + 2));
      return;
    }
  }
  return;
}
#endif
