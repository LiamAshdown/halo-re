// actor_remove_from_unit_cluster  (Ghidra: actor_remove_from_unit_cluster, already named)
// address 0x427c90, size 360 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/ai.h swarm.unit_index[16]/component_index[16]/component_count,
//   actor.cluster_count (0x1e), actor.swarm_index (0x28); types/objects.h object_header,
//   object.parent_object (0x11c)/location_cluster_index (0x9c); types/units.h
//   unit_data.swarm_actor_index (object+0x1f8)/swarm_next_unit_index (object+0x1fc). Calls
//   datum_delete (0x4d0510, EAX -> array, EDX -> handle) and unit_refresh_targeting_flag_and_weapons (not in this
//   rewrite range). Confirmed the datum_delete handle with objdump (bin/halo.exe
//   0x427c90..0x427df7): EDX is set once, to the *pre-swap* swarm.component_index[slot], and
//   is never touched again before the call.
//   UNSURE: object+0x200 (the doubly-linked cluster list's "previous" pointer, paired with
//   swarm_next_unit_index at +0x1fc) has no established name in types/units.h ("untouched
//   here" per that header's own comment); accessed as a raw offset.
// register convention: ECX -> actor_index, stack -> unit_index.
//   // blam-cc: ECX -> actor_index, stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;           // 0x00880360
extern data_array *object_data;          // 0x008603b0
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358

extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle

// blam-cc: ECX -> actor_index, stack -> unit_index
// Removes an actor from a unit's cluster (the doubly-linked list of controlling actors) and,
// if it had a swarm component for that unit, removes that too (swap-with-last, then deletes
// the freed swarm_component datum). Undoes actor_link_to_unit_cluster.
void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    if (unit->swarm_actor_index != actor_index) {
        return;
    }

    header->flags |= _object_header_in_pvs_pass_bit;
    if (unit_object->parent_object == (datum_index)k_datum_index_none &&
        unit_object->location_cluster_index == -1) {
        if ((header->flags & _object_header_active_bit) != 0) {
            header->flags &= ~_object_header_active_bit;
        }
    }

    unit_refresh_targeting_flag_and_weapons(unit_index, 0); // CL = 0

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        if (s->component_count > 0) {
            int16_t i;
            for (i = 0; i < s->component_count; i++) {
                if (s->unit_index[i] == unit_index) {
                    datum_index freed_component = s->component_index[i];
                    int16_t new_count = s->component_count - 1;
                    s->component_count = new_count;
                    if (i < new_count) {
                        s->unit_index[i] = s->unit_index[new_count];
                        s->component_index[i] = s->component_index[new_count];
                    }
                    datum_delete(swarm_component_data, freed_component);
                    break;
                }
            }
        }
    }

    {
        uint32_t prev = *(uint32_t *)((uint8_t *)unit_object + 0x200); // UNSURE, see file header
        datum_index next = unit->swarm_next_unit_index;

        if (prev == 0xffffffff) {
            self->cluster_unit_index = next;
        } else {
            object *prev_object = ((object_header *)object_data->data)[prev & 0xffff].data;
            *(uint32_t *)((uint8_t *)prev_object + 0x1fc) = next; // unit_data.swarm_next_unit_index
        }
        if (next != (datum_index)k_datum_index_none) {
            object *next_object = ((object_header *)object_data->data)[next & 0xffff].data;
            *(uint32_t *)((uint8_t *)next_object + 0x200) = prev; // UNSURE, see file header
        }
    }

    unit->swarm_actor_index = (datum_index)k_datum_index_none;
    self->cluster_count = self->cluster_count - 1;
}

#if 0
Original Ghidra decompilation (0x427c90):

void actor_remove_from_unit_cluster(uint param_1)

{
  byte *pbVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  uint in_ECX;
  int iVar7;
  int iVar8;
  short sVar9;

  iVar7 = DAT_008603b0;
  iVar8 = (in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar5 = (param_1 & 0xffff) * 0xc;
  iVar3 = *(int *)(iVar5 + 8 + *(int *)(DAT_008603b0 + 0x34));
  if (*(uint *)(iVar3 + 0x1f8) == in_ECX) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
    pbVar1 = (byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar5);
    *pbVar1 = *pbVar1 | 0x40;
    if ((*(int *)(iVar4 + 0x11c) == -1) && (*(short *)(iVar4 + 0x9c) == -1)) {
      iVar5 = *(int *)(iVar7 + 0x34) + iVar5;
      bVar2 = *(byte *)(iVar5 + 2);
      if ((bVar2 & 1) != 0) {
        *(byte *)(iVar5 + 2) = bVar2 & 0xfe;
      }
    }
    FUN_00569bf0(param_1);
    if (*(uint *)(iVar8 + 0x28) != 0xffffffff) {
      iVar5 = (*(uint *)(iVar8 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
      if (0 < *(short *)(iVar5 + 2)) {
        sVar6 = 0;
        do {
          if (*(uint *)(iVar5 + 0x18 + sVar6 * 4) == param_1) {
            sVar9 = *(short *)(iVar5 + 2) + -1;
            *(short *)(iVar5 + 2) = sVar9;
            if (sVar6 < sVar9) {
              *(undefined4 *)(iVar5 + 0x18 + sVar6 * 4) = *(undefined4 *)(iVar5 + 0x18 + sVar9 * 4);
              *(undefined4 *)(iVar5 + 0x58 + sVar6 * 4) =
                   *(undefined4 *)(iVar5 + 0x58 + *(short *)(iVar5 + 2) * 4);
            }
            datum_delete();
            iVar7 = DAT_008603b0;
            break;
          }
          sVar6 = sVar6 + 1;
          iVar7 = DAT_008603b0;
        } while (sVar6 < *(short *)(iVar5 + 2));
      }
    }
    if (*(uint *)(iVar3 + 0x200) == 0xffffffff) {
      *(undefined4 *)(iVar8 + 0x24) = *(undefined4 *)(iVar3 + 0x1fc);
    }
    else {
      *(undefined4 *)
       (*(int *)(*(int *)(iVar7 + 0x34) + 8 + (*(uint *)(iVar3 + 0x200) & 0xffff) * 0xc) + 0x1fc) =
           *(undefined4 *)(iVar3 + 0x1fc);
    }
    if (*(uint *)(iVar3 + 0x1fc) != 0xffffffff) {
      *(undefined4 *)
       (*(int *)(*(int *)(iVar7 + 0x34) + 8 + (*(uint *)(iVar3 + 0x1fc) & 0xffff) * 0xc) + 0x200) =
           *(undefined4 *)(iVar3 + 0x200);
    }
    *(undefined4 *)(iVar3 + 0x1f8) = 0xffffffff;
    *(short *)(iVar8 + 0x1e) = *(short *)(iVar8 + 0x1e) + -1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
