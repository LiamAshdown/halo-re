// item_new  (Ghidra: missed_4bc580, created by hand this pass -- Ghidra never recovered it as a
// function; only reachable through the item object_type_definition sub-row)
// address 0x4bc580, size 60 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: the item sub-row (0x0069b680, subdefinitions[1] of the weapon/equipment/garbage
//   rows) carries this address at +0x28 (query_create), the same column family as
//   equipment_new/weapon_new/garbage_new in this batch; the item row's own +0x34 is already
//   item_update (0x4bc5c0), per out/phase4/items_types_notes.md ("the item row's +0x34 is
//   0x4bc5c0"). types/items.h item_data.ignore_object_index (0x200, "item_update forces it to
//   -1 the moment the item comes to rest"), item_data.held_game_time (0x204, "item_update stamps
//   the game tick from *(int *)(0x006f1d6c+0xc)"); types/objects.h object_flags
//   (_object_unknown_20000_bit 0x2000... UNSURE, see below). global 0x006f1d6c
//   game_time (+0x0c the game tick).
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family.
// blam-cc: stack -> object_index
// UNSURE: the object.flags bits set here (0x2000 and 0x4000) have no names in types/objects.h's
//   object_flags enum; written here as the raw literal 0x6000, as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c

// The item sub-row's query_create hook (object_type_definition +0x28), run for every freshly
// activated weapon, equipment or garbage object (all three chain through this row). Sets two
// unnamed object flags, stamps held_game_time from the current game tick, and clears
// ignore_object_index. Always reports success.
uint8_t item_new(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    obj->flags |= 0x6000; // UNSURE: unnamed object_flags bits 0x2000 | 0x4000
    id->held_game_time = game_time->game_time;
    id->ignore_object_index = (datum_index)k_datum_index_none;

    return 1;
}

#if 0
Original Ghidra decompilation (0x4bc580):

undefined4 missed_4bc580(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x6000;
  *(undefined4 *)(iVar1 + 0x204) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  *(undefined4 *)(iVar1 + 0x200) = 0xffffffff;
  return CONCAT31((int3)((uint)iVar1 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
