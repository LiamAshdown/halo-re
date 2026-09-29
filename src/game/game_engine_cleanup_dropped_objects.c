// game_engine_cleanup_dropped_objects  (Ghidra: game_engine_cleanup_dropped_objects, already
// named)
// address 0x45f320, size 472 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Periodically deletes old corpses and expired dropped
// items that have exceeded their lifetime (about 30 seconds)"); types/items.h item_data (flags
// +0x1f4, _item_in_inventory_bit; held_game_time +0x204); types/objects.h object (network_role
// +0x004), object_header, object_iterator, _object_mask_item (0x1c), _object_mask_biped (0x1);
// types/cache.h tag_instance; src/devices/device_group_set_value.c's established
// object_iterator_next idiom and datum_get for the manual handle-revalidation Ghidra inlines.
// UNSURE: the tag-data flag test on the item's collision-model-ish record (offset +0x308, bit 3)
// is not attributable to any struct in types/tags.h from this evidence alone; kept as a raw
// offset with the exact bit test preserved.
// Note: the second loop's Ghidra output calls `object_iterator_next(&iVar5)` for its recursive
// step, passing the address of the just-fetched object pointer instead of the iterator struct --
// this is a decompiler rendering artifact (every other iterator loop in the image, including
// this function's own first loop, re-uses the same iterator variable throughout), not a real
// bug in the binary; the loop below re-uses `iterator` as every other one in this batch does.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"

extern game_time_globals *game_time; // 0x006f1d6c
extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20
extern void *datum_get(datum_index handle, data_array *array);  // 0x4d0680
extern void object_delete(datum_index object_index);             // 0x4f5bd0, UNSURE exact signature
extern void object_delete_unparented(datum_index object_index);  // 0x4f5aa0, UNSURE exact signature
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0, UNSURE

// Deletes dropped items whose item_data.held_game_time is more than 900 ticks (30 s) old and
// that aren't resting on a "wake on destroy" surface flagged for special handling, then, in a
// second pass, deletes biped corpses that have been dead for more than 900 ticks and are marked
// for cleanup (object_header flags bit 4).
void game_engine_cleanup_dropped_objects(void)
{
    int32_t now = game_time->game_time;
    object_iterator iterator;
    object *obj;

    iterator.type_mask = _object_mask_item;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

        if ((int32_t)item->held_game_time < now - 900 &&
            (item->flags & _item_in_inventory_bit) == 0) {
            object_header *hdr = (object_header *)datum_get(iterator.handle, object_data);
            uint8_t wake_flag = 0;

            if (hdr != 0) {
                tag_instance *ti = &tag_instances[obj->definition_tag & 0xffff];
                wake_flag = (uint8_t)((*(uint32_t *)((uint8_t *)ti->data + 0x308) >> 3) & 1);
            }

            if ((hdr == 0 || (1u << hdr->type) != _object_mask_weapon ||
                 hdr->data == 0 || wake_flag == 0) &&
                (obj->network_role != 1 && (item->flags & 0x40) == 0)) {
                object_delete(iterator.handle);
            }
        }

        obj = object_iterator_next(&iterator);
    }

    iterator.type_mask = _object_mask_biped;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        if (900 < *(int16_t *)&((struct object *)obj)->dead_at_rest_ticks &&
            (*(uint8_t *)&((object *)obj)->vitality_flags & 4) != 0) {
            if (obj->network_role == 0) {
                object_delete_unparented(iterator.handle);
            } else if (obj->network_role == 3) {
                object_delete_recursive(iterator.handle, 0);
            }
        }
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x45f320), from tools/pack.py 0x45f320:

void game_engine_cleanup_dropped_objects(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  short *psVar4;
  int iVar5;
  short *psVar6;
  short sVar7;
  short sVar8;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  iVar5 = *(int *)(DAT_006f1d6c + 0xc);
  local_4 = 0x86868686;
  local_10 = 0x1c;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar3 = object_iterator_next(&local_10);
  while (iVar3 != 0) {
    puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_8 & 0xffff) * 0xc);
    if (((int)puVar1[0x81] < iVar5 + -900) && ((puVar1[0x7d] & 1) == 0)) {
      psVar4 = (short *)0x0;
      if ((local_8 != 0xffffffff) &&
         ((sVar7 = (short)local_8, -1 < sVar7 && (sVar7 < *(short *)(DAT_008603b0 + 0x20))))) {
        psVar6 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar7 +
                          *(int *)(DAT_008603b0 + 0x34));
        sVar7 = *psVar6;
        if ((sVar7 != 0) && ((sVar8 = (short)(local_8 >> 0x10), sVar8 == 0 || (sVar7 == sVar8)))) {
          psVar4 = psVar6;
        }
      }
      if (((((psVar4 == (short *)0x0) || ((1 << (*(byte *)((int)psVar4 + 3) & 0x1f) & 4U) == 0)) ||
           (*(int *)(psVar4 + 4) == 0)) ||
          ((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1)
           == 0)) && ((puVar1[1] != 1 && ((puVar1[0x7d] & 0x40) == 0)))) {
        object_delete();
      }
    }
    iVar3 = object_iterator_next(&local_10);
  }
  local_4 = 0x86868686;
  local_10 = 1;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar5 = object_iterator_next(&local_10);
  uVar2 = local_8;
  do {
    if (iVar5 == 0) {
      return;
    }
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    local_8 = uVar2;
    if ((900 < *(short *)(iVar5 + 0xbc)) && ((*(byte *)(iVar5 + 0x106) & 4) != 0)) {
      if (*(int *)(iVar5 + 4) == 0) {
        object_delete_unparented();
      }
      else if (*(int *)(iVar5 + 4) != 3) goto LAB_0045f4ef;
      object_delete_recursive(uVar2,0);
    }
LAB_0045f4ef:
    iVar5 = object_iterator_next(&iVar5);
    uVar2 = local_8;
  } while( true );
}
#endif
