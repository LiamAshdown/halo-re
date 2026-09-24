// game_engine_reset_vehicles_or_race_cleanup  (Ghidra: FUN_004681a0; named per this rewrite)
// address 0x4681a0, size 192 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md; both branches iterate _object_mask_vehicle (mask 2),
//   not "all objects" as the summary guesses -- Race (game_engine_variant.game_engine_index ==
//   5) runs the same network_role-dispatched delete pair game_engine_cleanup_stray_projectiles.c
//   (0x467f70) and game_engine_cleanup_stray_items.c (0x468010, this batch) already use; every
//   other gametype instead calls unit_set_facing_from_index_table (0x570de0, already named in
//   src/units/) on every live vehicle.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern game_variant game_engine_variant; // 0x006f1c88 (game_engine_index aliased 0x006f1cb8)
extern data_array *object_headers;       // 0x008603b0

extern object *object_iterator_next(object_iterator *iterator);              // 0x4f6f20
extern void object_delete_unparented(datum_index object_index);              // 0x4f5aa0
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0
extern void unit_set_facing_from_index_table(uint32_t object_index);         // 0x570de0

// In Race, deletes any unparented/stray vehicle exactly like the item and projectile sweeps
// (network_role 0 -> both delete calls, network_role 3 -> recursive delete only). In every other
// gametype, instead resets every live vehicle's facing via unit_set_facing_from_index_table.
void game_engine_reset_vehicles_or_race_cleanup(void)
{
    object_iterator iter;
    object *obj;

    iter.type_mask = _object_mask_vehicle;
    iter.flags_mask = 0;
    iter.unknown_05 = 0;
    iter.index = 0;
    iter.handle = (datum_index)0xffffffff;

    if (game_engine_variant.game_engine_index == _game_engine_race) {
        obj = object_iterator_next(&iter);
        while (obj != (object *)0) {
            if (obj->network_role == 0) {
                object_delete_unparented(iter.handle);
                object_delete_recursive(iter.handle, 0);
            } else if (obj->network_role == 3) {
                object_delete_recursive(iter.handle, 0);
            }
            obj = object_iterator_next(&iter);
        }
    } else {
        obj = object_iterator_next(&iter);
        while (obj != (object *)0) {
            unit_set_facing_from_index_table((uint32_t)iter.handle);
            obj = object_iterator_next(&iter);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4681a0), from tools/pack.py 0x4681a0:

void FUN_004681a0(void)

{
  uint uVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 2;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  if (DAT_006f1cb8 == 5) {
    iVar2 = object_iterator_next(&local_10);
    if (iVar2 != 0) {
      do {
        uVar1 = local_8;
        iVar2 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_8 & 0xffff) * 0xc) + 4)
        ;
        if (iVar2 == 0) {
          FUN_004f5aa0();
LAB_00468210:
          FUN_004f59d0(uVar1,0);
        }
        else if (iVar2 == 3) goto LAB_00468210;
        iVar2 = object_iterator_next(&local_10);
        if (iVar2 == 0) {
          return;
        }
      } while( true );
    }
  }
  else {
    iVar2 = object_iterator_next(&local_10);
    while (iVar2 != 0) {
      FUN_00570de0(local_8);
      iVar2 = object_iterator_next(&local_10);
    }
  }
  return;
}
#endif
