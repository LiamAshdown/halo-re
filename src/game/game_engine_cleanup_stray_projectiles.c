// game_engine_cleanup_stray_projectiles  (Ghidra: FUN_00467f70; named per this rewrite --
// sharpens out/phase4/game_functions.md's guess: "Sweeps all live game objects and deletes/
// garbage-collects any of a specific type (biped or weapon-class, per the +4 type field).")
// address 0x467f70, size 139 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x467f70
//   --stop-address=0x468010), the same object_iterator sweep as the closing half of
//   game_engine_reset_respawns_and_cleanup_bipeds.c (0x467e60, this batch) with
//   object_iterator::type_mask == 0x020 (types/objects.h `_object_mask_projectile`) instead of
//   biped, and without the vitality_flags gate that function has. object::network_role (+0x004,
//   "object_delete dispatches on 0 versus 3") is the "+4 type field" functions.md's summary
//   refers to; it is not an object type, so this rewrite corrects the name accordingly.
//   object_delete_unparented (blam-cc EDI) and object_delete_recursive (blam-cc stack) match
//   their already-committed src/objects/ signatures exactly.
// register convention: no parameters.
// UNSURE: why role 0 calls both object_delete_unparented and object_delete_recursive while
//   role 3 calls only the latter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *object_data; // 0x008603b0

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20
extern void object_delete_unparented(datum_index object_index);    // 0x4f5aa0, blam-cc: EDI
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0

// Sweeps every live projectile object and deletes any whose network_role is 0 (via both
// object_delete_unparented and object_delete_recursive) or 3 (via object_delete_recursive only).
void game_engine_cleanup_stray_projectiles(void)
{
    object_iterator iter;
    object *obj;

    iter.type_mask = 0x020; // _object_mask_projectile
    iter.flags_mask = 0;
    iter.unknown_05 = 0;
    iter.index = 0;
    iter.handle = (datum_index)0xffffffff;

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
}

#if 0
Original Ghidra decompilation (0x467f70), from tools/pack.py 0x467f70:

void FUN_00467f70(void)

{
  uint uVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0x20;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar2 = object_iterator_next(&local_10);
  uVar1 = local_8;
  do {
    if (iVar2 == 0) {
      return;
    }
    iVar2 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 4);
    local_8 = uVar1;
    if (iVar2 == 0) {
      object_delete_unparented();
LAB_00467fe0:
      object_delete_recursive(uVar1,0);
    }
    else if (iVar2 == 3) goto LAB_00467fe0;
    iVar2 = object_iterator_next(&local_10);
    uVar1 = local_8;
  } while( true );
}
#endif
