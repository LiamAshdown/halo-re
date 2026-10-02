// game_engine_update_item_scale_and_pickup  (Ghidra: FUN_0045f560; named for what it does)
// address 0x45f560, size 374 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Updates a per-object friction value each tick and
// notifies the game variant about pickup-eligible objects"; low confidence, 0.3) -- the field
// this function actually writes, object+0xb0, is types/objects.h's documented `scale` field
// ("multiplies the radius when non-zero"), not friction, so the name here follows the header
// over the looser one-line summary. Item.scale is at tag offset 0x184 (Object base 0x17c +
// item_flags 4 + pickup_text_index 2 + sort_order 2). types/game.h game_engine_definition
// (object_in_play_update at +0x3c); types/items.h item_data (_item_in_inventory_bit);
// types/objects.h object_header, _object_mask_weapon; src/memory/datum_get.c for the manual
// handle-revalidation Ghidra inlines twice here.
// UNSURE: the tag flag test at data+0x308 bit 3 (same offset used by
// game_engine_cleanup_dropped_objects) is not attributed to a named struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680
extern void game_engine_notify_item_expired(datum_index object_index); // 0x45f510, this batch

// Every tick, sets each item's render/collision scale from its Item tag (or 1.0 while the item
// is held), then, for items whose game engine implements object_in_play_update, notifies both
// the expiry hook and the engine's own per-tick pickup-eligibility callback for weapons that are
// still fully attached (root parent, tag flag bit 3 set).
void game_engine_update_item_scale_and_pickup(void)
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = _object_mask_item;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

        if ((item->flags & _item_in_inventory_bit) == 0) {
            Item *tag = (Item *)tag_instances[obj->definition_tag & 0xffff].data;
            obj->scale = (tag->scale == 0.0f) ? 1.0f : tag->scale;
        } else {
            obj->scale = 1.0f;
        }

        if (current_game_engine != 0 && current_game_engine->object_in_play_update != 0) {
            object_header *hdr = (object_header *)datum_get(iterator.handle, object_data);

            if (hdr != 0 && (1u << hdr->type) == _object_mask_weapon && hdr->data != 0 &&
                ((*(uint32_t *)((uint8_t *)tag_instances[obj->definition_tag & 0xffff].data + 0x308) >> 3) & 1) != 0) {
                game_engine_notify_item_expired(iterator.handle);
                ((void (*)(datum_index, object *))current_game_engine->object_in_play_update)(
                    iterator.handle, hdr->data);
            }
        }

        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x45f560), from tools/pack.py 0x45f560:

void FUN_0045f560(void)

{
  uint uVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  short *psVar7;
  short sVar8;
  short *psVar9;
  int iVar10;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0x1c;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar6 = object_iterator_next(&local_10);
  iVar10 = DAT_008603b0;
  uVar5 = local_8;
  while (iVar6 != 0) {
    iVar6 = (uVar5 & 0xffff) * 0xc;
    puVar3 = *(uint **)(*(int *)(iVar10 + 0x34) + 8 + iVar6);
    if ((puVar3[0x7d] & 1) == 0) {
      iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(float *)(iVar4 + 0x184) == 0.0) {
        uVar1 = 0x3f800000;
      }
      else {
        uVar1 = *(uint *)(iVar4 + 0x184);
      }
      puVar3[0x2c] = uVar1;
    }
    else {
      puVar3[0x2c] = 0x3f800000;
    }
    local_8 = uVar5;
    if (*(int *)(DAT_006f1d20 + 0x3c) != 0) {
      psVar9 = (short *)0x0;
      if (((uVar5 != 0xffffffff) && (local_8._0_2_ = (short)uVar5, -1 < (short)local_8)) &&
         ((short)local_8 < *(short *)(iVar10 + 0x20))) {
        psVar7 = (short *)((int)*(short *)(iVar10 + 0x22) * (int)(short)local_8 +
                          *(int *)(iVar10 + 0x34));
        sVar2 = *psVar7;
        if ((sVar2 != 0) && ((sVar8 = (short)(uVar5 >> 0x10), sVar8 == 0 || (sVar2 == sVar8)))) {
          psVar9 = psVar7;
        }
      }
      if (((psVar9 != (short *)0x0) && ((1 << (*(byte *)((int)psVar9 + 3) & 0x1f) & 4U) != 0)) &&
         ((iVar4 = *(int *)(psVar9 + 4), iVar4 != 0 &&
          ((*(uint *)(*(int *)((**(uint **)(*(int *)(iVar10 + 0x34) + 8 + iVar6) & 0xffff) * 0x20 +
                               0x14 + DAT_0087bc14) + 0x308) >> 3 & 1) != 0)))) {
        FUN_0045f510();
        (**(code **)(DAT_006f1d20 + 0x3c))(uVar5,iVar4);
        iVar10 = DAT_008603b0;
      }
    }
    iVar6 = object_iterator_next(&local_10);
    uVar5 = local_8;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
