// ai_object_list_detach_actors_from_encounters  (Ghidra: ai_object_list_detach_actors_from_encounters; named for this rewrite)
// address 0x435260, size 433 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: the object-list twin of ai_reference_detach_actors_from_encounters @0x4351c0 --
//   same encounter_remove_actor / unassigned-list relink / encounters_recompute_dirty
//   sequence, but walking an object_list (types/hs.h, the 0x0087a464 header and 0x0087a468
//   reference data_arrays every other ai_object_list_* function in this directory uses)
//   instead of a packed ai reference. Only objects that are bipeds or vehicles
//   ((1 << object.type) & 3) and whose unit has an actor already in an encounter are touched.
// register convention: EAX -> object_list_header_handle handle, ECX -> the reference cursor
//   (Ghidra's in_ECX, overwritten immediately by the header's first_reference).
//   // blam-cc: EAX -> object_list_header_handle
//
// UNSURE: as in the reference twin, actor_movement_action_cancel is argument-less in Ghidra and is written
// here as taking the actor it is about. The "unit_data + 0x1f4" access is rendered by Ghidra
// as `*(uint *)(iVar5 + 500)`, i.e. object + 0x1f4, which is unit_data.actor_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *object_data;                // 0x008603b0
extern data_array *actor_data;                 // 0x00880360
extern ai_globals *ai_globals_ptr;             // 0x00880354

extern void actor_movement_action_cancel(datum_index actor_index); // 0x428650, not yet rewritten
extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters); // 0x436620, blam-cc: EAX -> actor_index
extern void encounters_recompute_dirty(void);                             // 0x435f00

// blam-cc: EAX -> object_list_header_handle
// Pulls every actor controlling a unit in the object list out of its encounter and parks it
// on the global unassigned-actor list.
void ai_object_list_detach_actors_from_encounters(datum_index object_list_header_handle)
{
    object_list_header *header;
    object_list_reference *node;
    datum_index node_index;
    datum_index object_index;
    object_header *entry;
    object *obj;
    unit_data *unit;
    actor *a;
    datum_index actor_index;
    int16_t detached;

    object_index = (datum_index)k_datum_index_none;
    node_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    detached = 0;
    if (object_index == (datum_index)k_datum_index_none) {
        return;
    }

    do {
        entry = 0;
        if (object_index != (datum_index)k_datum_index_none &&
            0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)
                [(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 ||
                 candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            obj = entry->data;
            unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if (unit->actor_index != (datum_index)k_datum_index_none &&
                ((actor *)actor_data->data)[unit->actor_index & 0xffff].encounter_index !=
                    (datum_index)k_datum_index_none) {

                actor_index = unit->actor_index;
                actor_movement_action_cancel(actor_index);
                encounter_remove_actor(actor_index, 0);

                if (ai_globals_ptr->actors_valid != 0) {
                    a = &((actor *)actor_data->data)[actor_index & 0xffff];
                    a->next_in_encounter = ai_globals_ptr->first_encounterless_actor;
                    ai_globals_ptr->first_encounterless_actor = actor_index;
                    a->unknown_09 = 1;
                    *(uint16_t *)&a->unknown_10[0] =
                        (uint16_t)(-(uint16_t)(a->active != 0) & 0x5a);
                    actor_movement_action_cancel(actor_index);
                }
                detached = detached + 1;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    } while (object_index != (datum_index)k_datum_index_none);

    if (0 < detached) {
        encounters_recompute_dirty();
    }
}

#if 0
Original Ghidra decompilation (0x435260):

void FUN_00435260(void)

{
  short sVar1;
  uint in_EAX;
  uint uVar2;
  short *psVar3;
  short sVar4;
  uint in_ECX;
  int iVar5;
  short sVar6;
  int iVar7;
  short *psVar8;

  iVar5 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar5 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  sVar1 = 0;
  iVar7 = DAT_008603b0;
  if (iVar5 != -1) {
    do {
      psVar8 = (short *)0x0;
      if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) &&
         (sVar4 < *(short *)(iVar7 + 0x20))) {
        psVar3 = (short *)((int)*(short *)(iVar7 + 0x22) * (int)sVar4 + *(int *)(iVar7 + 0x34));
        sVar4 = *psVar3;
        if ((sVar4 != 0) && ((sVar6 = (short)((uint)iVar5 >> 0x10), sVar6 == 0 || (sVar4 == sVar6)))
           ) {
          psVar8 = psVar3;
        }
      }
      if (((psVar8 != (short *)0x0) && ((1 << (*(byte *)((int)psVar8 + 3) & 0x1f) & 3U) != 0)) &&
         ((iVar5 = *(int *)(psVar8 + 4), iVar5 != 0 &&
          ((*(uint *)(iVar5 + 500) != 0xffffffff &&
           (*(int *)((*(uint *)(iVar5 + 500) & 0xffff) * 0x724 + 0x34 +
                    *(int *)(DAT_00880360 + 0x34)) != -1)))))) {
        FUN_00428650();
        squad_remove_actor(0);
        iVar7 = DAT_00880354;
        uVar2 = *(uint *)(iVar5 + 500);
        if (*(char *)(DAT_00880354 + 1) != '\0') {
          iVar5 = (uVar2 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
          *(undefined4 *)(iVar5 + 0x2c) = *(undefined4 *)(DAT_00880354 + 8);
          *(uint *)(iVar7 + 8) = uVar2;
          *(undefined1 *)(iVar5 + 9) = 1;
          *(ushort *)(iVar5 + 0x10) = -(ushort)(*(char *)(iVar5 + 8) != '\0') & 0x5a;
          FUN_00428650();
        }
        sVar1 = sVar1 + 1;
        iVar7 = DAT_008603b0;
      }
      if (in_ECX == 0xffffffff) {
        iVar5 = -1;
      }
      else {
        uVar2 = in_ECX & 0xffff;
        in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
        iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
      }
    } while (iVar5 != -1);
    if (0 < sVar1) {
      FUN_00435f00();
      return;
    }
  }
  return;
}
#endif
