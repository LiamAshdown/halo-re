// ai_object_list_reset_or_wake_awareness  (Ghidra: ai_object_list_reset_or_wake_awareness; named for this rewrite)
// address 0x434590, size 537 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: walks an object_list (types/hs.h) and, for every biped/vehicle object and every
// biped/vehicle child of it (object.next_object/first_child_object, matching
// ai_object_list_remap_units_and_children's own pattern, this batch), applies the same
// awareness reset/wake logic as ai_reference_reset_or_wake_awareness (0x434500, this batch)
// to that unit's controlling actor (unit_data.actor_index, falling back to
// swarm_actor_index).
// register convention: Ghidra fully resolved the flag parameter and left the object_list
// header in EAX.
//   // blam-cc: EAX -> object_list_header_handle, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

extern data_array *object_data;                // 0x008603b0
extern data_array *actor_data;                 // 0x00880360
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void actor_clear_perceived_props(datum_index actor_index); // 0x427e00, outside this rewrite's range, UNSURE signature
extern void actor_dispatch_perception_reset(void); // 0x429000, outside this rewrite's range, UNSURE signature
extern void actor_set_units_active(datum_index actor_index, uint8_t activate); // 0x427860, blam-cc: EAX, BL

static void reset_or_wake(datum_index unit_index, char flag)
{
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
    datum_index actor_index = unit->actor_index;

    if (actor_index == (datum_index)k_datum_index_none) {
        actor_index = unit->swarm_actor_index;
    }
    if (actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
        if (flag == 0) {
            if (a->awareness_level == 0) {
                a->awareness_level = 2;
            }
        } else {
            a->awareness_level = 0;
            a->mode = 0;
            actor_clear_perceived_props(actor_index);
            actor_dispatch_perception_reset();
            actor_set_units_active(actor_index, 0); // BL = 0 at both call sites (0x43469e, 0x43473f)
        }
    }
}

void ai_object_list_reset_or_wake_awareness(datum_index object_list_header_handle, char flag)
{
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object *obj = object_try_and_get(object_index, 3);
        if (obj != 0) {
            datum_index child = obj->first_child_object;
            reset_or_wake(object_index, flag);
            while (child != (datum_index)k_datum_index_none) {
                object_header *child_header = &((object_header *)object_data->data)[child & 0xffff];
                if ((1 << (child_header->type & 0x1f) & 3) != 0) {
                    reset_or_wake(child, flag);
                }
                child = ((object *)child_header->data)->next_object;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x434590):

void FUN_00434590(char param_1)

{
  uint in_EAX;
  uint uVar1;
  short *psVar2;
  int iVar3;
  short sVar4;
  uint in_ECX;
  int iVar5;
  short sVar6;
  int iVar7;
  short *psVar8;

  iVar5 = -1;
  iVar7 = DAT_008603b0;
  if (in_EAX != 0xffffffff) {
    uVar1 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar1 == 0xffffffff) {
      iVar5 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar1 = uVar1 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar1 * 0xc);
      iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar1 * 0xc + 4);
    }
  }
  while (iVar5 != -1) {
    psVar8 = (short *)0x0;
    if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) && (sVar4 < *(short *)(iVar7 + 0x20)))
    {
      psVar2 = (short *)((int)*(short *)(iVar7 + 0x22) * (int)sVar4 + *(int *)(iVar7 + 0x34));
      sVar4 = *psVar2;
      if ((sVar4 != 0) && ((sVar6 = (short)((uint)iVar5 >> 0x10), sVar6 == 0 || (sVar4 == sVar6))))
      {
        psVar8 = psVar2;
      }
    }
    if (((psVar8 != (short *)0x0) && ((1 << (*(byte *)((int)psVar8 + 3) & 0x1f) & 3U) != 0)) &&
       (iVar5 = *(int *)(psVar8 + 4), iVar5 != 0)) {
      uVar1 = *(uint *)(iVar5 + 500);
      if ((uVar1 != 0xffffffff) || (uVar1 = *(uint *)(iVar5 + 0x1f8), uVar1 != 0xffffffff)) {
        iVar3 = (uVar1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
        if (param_1 == '\0') {
          if (*(short *)(iVar3 + 0x6a) == 0) {
            *(undefined2 *)(iVar3 + 0x6a) = 2;
          }
        }
        else {
          *(undefined2 *)(iVar3 + 0x6a) = 0;
          *(undefined2 *)(iVar3 + 0x6c) = 0;
          FUN_00427e00(uVar1);
          FUN_00429000();
          actor_set_units_active();
          iVar7 = DAT_008603b0;
        }
      }
      uVar1 = *(uint *)(iVar5 + 0x118);
      while (uVar1 != 0xffffffff) {
        iVar5 = *(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
        if (((1 << (*(byte *)(iVar5 + 0xb4) & 0x1f) & 3U) != 0) &&
           ((uVar1 = *(uint *)(iVar5 + 500), uVar1 != 0xffffffff ||
            (uVar1 = *(uint *)(iVar5 + 0x1f8), uVar1 != 0xffffffff)))) {
          iVar3 = (uVar1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
          if (param_1 == '\0') {
            if (*(short *)(iVar3 + 0x6a) == 0) {
              *(undefined2 *)(iVar3 + 0x6a) = 2;
            }
          }
          else {
            *(undefined2 *)(iVar3 + 0x6a) = 0;
            *(undefined2 *)(iVar3 + 0x6c) = 0;
            FUN_00427e00(uVar1);
            FUN_00429000();
            actor_set_units_active();
            iVar7 = DAT_008603b0;
          }
        }
        uVar1 = *(uint *)(iVar5 + 0x114);
      }
    }
    if (in_ECX == 0xffffffff) {
      iVar5 = -1;
    }
    else {
      iVar5 = *(int *)(DAT_0087a468 + 0x34) + (in_ECX & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar5 + 8);
      iVar5 = *(int *)(iVar5 + 4);
    }
  }
  return;
}
#endif
