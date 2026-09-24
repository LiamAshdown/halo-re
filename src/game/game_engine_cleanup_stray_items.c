// game_engine_cleanup_stray_items  (Ghidra: FUN_00468010; named per this rewrite)
// address 0x468010, size 308 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md's summary ("...except when engine type 1 (CTF) applies
//   special handling") is CORRECTED here: the guarded global is network_game_mode (0x00719720,
//   "0 local, 1 client, 2 host, 3 replay" per types/game.h), not a game_engine_index -- the test
//   is "not a client, or this object's own network_role is 3", exactly the same network_role
//   dispatch game_engine_cleanup_stray_projectiles.c (0x467f70, already committed) uses. The
//   iterator mask is 0x1c == _object_mask_item (weapon | equipment | garbage), matching this
//   function's callers alongside the 0x2/vehicle sweep in game_engine_reset_objects (0x468260).
//   types/objects.h object_iterator, object_header, object (flags, definition_tag); types/items.h
//   item_data.flags (_item_in_inventory_bit 0x01, _item_unknown_40_bit 0x40); types/cache.h
//   tag_instance. object_delete_unparented / object_delete_recursive already carry this exact
//   pair-call shape from the sibling function.
// register convention: no parameters.
// UNSURE: the inner block re-resolves the SAME iterated handle through the object_header table a
//   second time and tests the header's own type bit against _object_mask_weapon plus a bit at
//   tag-data offset 0x308 (bit 3) before allowing a "protected, do not delete" skip outside
//   oddball (game_engine_variant.game_engine_index != 3). Transcribed literally; the exact
//   meaning of the tag-data bit is not recovered here (see types/tags.h TODO for Weapon flags).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"

extern data_array *object_headers;   // 0x008603b0
extern int16_t network_game_mode;    // 0x00719720
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_variant game_engine_variant; // 0x006f1c88 (game_engine_index aliased 0x006f1cb8)

extern object *object_iterator_next(object_iterator *iterator);              // 0x4f6f20
extern void object_delete_unparented(datum_index object_index);              // 0x4f5aa0
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0

// Sweeps every live item (weapon/equipment/garbage) object and deletes any that has come
// unparented and is not currently held (item_data flags 0x01/0x40 both clear), skipping objects
// that a redundant header re-lookup shows to be a still-protected weapon (see UNSURE above)
// except in Oddball games. On a client, only objects whose network_role is 3 are considered.
void game_engine_cleanup_stray_items(void)
{
    object_iterator iter;
    object *obj;

    iter.type_mask = _object_mask_item;
    iter.flags_mask = 0;
    iter.unknown_05 = 0;
    iter.index = 0;
    iter.handle = (datum_index)0xffffffff;

    obj = object_iterator_next(&iter);
    while (obj != (object *)0) {
        if (network_game_mode != 1 || obj->network_role == 3) {
            int16_t index16 = (int16_t)(uint32_t)iter.handle;

            if (iter.handle != (datum_index)0xffffffff && index16 >= 0 &&
                index16 < object_headers->maximum_count) {
                object_header *hdr = (object_header *)
                    ((uint8_t *)object_headers->data + (int32_t)object_headers->size * index16);
                int16_t salt = (int16_t)((uint32_t)iter.handle >> 16);

                if (hdr->identifier != 0 &&
                    (salt == 0 || hdr->identifier == salt) &&
                    ((1 << (hdr->type & 0x1f)) & _object_mask_weapon) != 0) {
                    if (hdr->data != (object *)0) {
                        uint32_t *tag_data = (uint32_t *)tag_instances[obj->definition_tag & 0xffff].data;
                        // UNSURE: tag-data offset 0x308, bit 3 -- exact Weapon-tag flag not recovered
                        if ((*(uint32_t *)((uint8_t *)tag_data + 0x308) >> 3 & 1) != 0 &&
                            game_engine_variant.game_engine_index != _game_engine_oddball) {
                            goto next;
                        }
                    }
                }
            }

            {
                item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
                if ((item->flags & (_item_in_inventory_bit | _item_unknown_40_bit)) == 0) {
                    if (obj->network_role == 0) {
                        object_delete_unparented(iter.handle);
                    } else if (obj->network_role != 3) {
                        goto next;
                    }
                    object_delete_recursive(iter.handle, 0);
                }
            }
        }
next:
        obj = object_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x468010), from tools/pack.py 0x468010:

void FUN_00468010(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  short *psVar4;
  short sVar5;
  int iVar6;
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
  iVar3 = object_iterator_next(&local_10);
  iVar6 = DAT_008603b0;
  uVar2 = local_8;
  do {
    if (iVar3 == 0) {
      return;
    }
    puVar1 = *(uint **)(*(int *)(iVar6 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    local_8 = uVar2;
    if ((DAT_00719720 != 1) || (puVar1[1] == 3)) {
      if ((uVar2 != 0xffffffff) &&
         ((local_8._0_2_ = (short)uVar2, -1 < (short)local_8 &&
          ((short)local_8 < *(short *)(iVar6 + 0x20))))) {
        psVar4 = (short *)((int)*(short *)(iVar6 + 0x22) * (int)(short)local_8 +
                          *(int *)(iVar6 + 0x34));
        if (((*psVar4 != 0) &&
            (((sVar5 = (short)(uVar2 >> 0x10), sVar5 == 0 || (*psVar4 == sVar5)) &&
             ((1 << (*(byte *)((int)psVar4 + 3) & 0x1f) & 4U) != 0)))) &&
           (((*(int *)(psVar4 + 4) != 0 &&
             ((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1
              ) != 0)) && (DAT_006f1cb8 != 3)))) goto LAB_00468127;
      }
      if ((puVar1[0x7d] & 0x41) == 0) {
        if (puVar1[1] == 0) {
          FUN_004f5aa0();
        }
        else if (puVar1[1] != 3) goto LAB_00468127;
        FUN_004f59d0(uVar2,0);
        iVar6 = DAT_008603b0;
      }
    }
LAB_00468127:
    iVar3 = object_iterator_next(&local_10);
    uVar2 = local_8;
  } while( true );
}
#endif
