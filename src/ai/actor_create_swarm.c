// actor_create_swarm  (Ghidra: actor_create_swarm, already named)
// address 0x427f40, size 355 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: types/ai.h swarm.actor_index(0x04)/component_count(0x02)/unit_index[16](0x18)/
//   component_index[16](0x58), swarm_component.unknown_14(0x14); actor.swarm_index(0x28)/
//   cluster_unit_index(0x24); types/units.h unit_data.swarm_next_unit_index (object+0x1fc).
//   Calls datum_new (0x4d0480) and object_get_position (0x4f6900), both already
//   established, plus the same unit-type/0x4d8 marker pattern as
//   actor_update_swarm_component_position @0x428130 (already rewritten in this module),
//   inlined here rather than called.
// register convention: EAX -> actor_index; no other register operands are read.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *object_data;          // 0x008603b0

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900

// blam-cc: EAX -> actor_index
// Creates a new swarm record for an actor (if it does not already have one) and populates it
// with a swarm-component entry for every actor sharing the same unit cluster, seeding each
// component's cached position and death/ground marker exactly as
// actor_update_swarm_component_position does. Returns the (possibly pre-existing) swarm
// datum index, or k_datum_index_none if allocation failed at any point.
datum_index actor_create_swarm(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm_index == (datum_index)k_datum_index_none) {
        self->swarm_index = datum_new(swarm_data);
        if (self->swarm_index != (datum_index)k_datum_index_none) {
            datum_index unit_index = self->cluster_unit_index;
            swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];

            s->actor_index = actor_index;
            s->component_count = 0;

            while (unit_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
                object *unit_object = header->data;
                datum_index component_index = datum_new(swarm_component_data);

                if (component_index == (datum_index)k_datum_index_none) {
                    return self->swarm_index;
                }

                {
                    swarm *s2 = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
                    swarm_component *component = &((swarm_component *)swarm_component_data->data)[component_index & 0xffff];
                    datum_index marker;

                    component->unknown_14 = 0xffffffff;
                    s2->unit_index[s2->component_count] = unit_index;
                    s2->component_index[s2->component_count] = component_index;
                    s2->component_count = s2->component_count + 1;

                    marker = (unit_object->type == 0) ? *(datum_index *)((uint8_t *)unit_object + 0x4d8)
                                                       : (datum_index)k_datum_index_none;
                    object_get_position(&component->position, unit_index);
                    component->marker_index = marker;
                }

                unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc); // unit_data.swarm_next_unit_index
            }
        }
    }
    return self->swarm_index;
}

#if 0
Original Ghidra decompilation (0x427f40):

undefined4 actor_create_swarm(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  uint uVar9;
  undefined8 uVar10;

  iVar5 = (in_EAX & 0xffff) * 0x724;
  iVar6 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  if (*(int *)(iVar5 + 0x28 + *(int *)(DAT_00880360 + 0x34)) == -1) {
    uVar10 = datum_new();
    uVar2 = (uint)uVar10;
    *(uint *)(iVar6 + 0x28) = uVar2;
    if (uVar2 != 0xffffffff) {
      uVar9 = *(uint *)(iVar6 + 0x24);
      iVar5 = (uVar2 & 0xffff) * 0x98 + *(int *)((int)((ulonglong)uVar10 >> 0x20) + 0x34);
      *(uint *)(iVar5 + 4) = in_EAX;
      *(undefined2 *)(iVar5 + 2) = 0;
      iVar5 = DAT_00880358;
      if (uVar9 == 0xffffffff) {
        return *(undefined4 *)(iVar6 + 0x28);
      }
      do {
        iVar8 = (uVar9 & 0xffff) * 0xc;
        iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
        uVar2 = datum_new();
        if (uVar2 == 0xffffffff) {
          return *(undefined4 *)(iVar6 + 0x28);
        }
        iVar3 = (*(uint *)(iVar6 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
        iVar4 = (uVar2 & 0xffff) * 0x40;
        *(undefined4 *)(iVar4 + 0x14 + *(int *)(iVar5 + 0x34)) = 0xffffffff;
        *(uint *)(iVar3 + 0x18 + *(short *)(iVar3 + 2) * 4) = uVar9;
        *(uint *)(iVar3 + 0x58 + *(short *)(iVar3 + 2) * 4) = uVar2;
        *(short *)(iVar3 + 2) = *(short *)(iVar3 + 2) + 1;
        iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
        iVar3 = *(int *)(iVar5 + 0x34);
        uVar7 = 0xffffffff;
        if (*(short *)(iVar8 + 0xb4) == 0) {
          uVar7 = *(undefined4 *)(iVar8 + 0x4d8);
        }
        object_get_position();
        *(undefined4 *)(iVar3 + iVar4 + 0x10) = uVar7;
        uVar9 = *(uint *)(iVar1 + 0x1fc);
      } while (uVar9 != 0xffffffff);
      return *(undefined4 *)(iVar6 + 0x28);
    }
  }
  return *(undefined4 *)(iVar6 + 0x28);
}
#endif
