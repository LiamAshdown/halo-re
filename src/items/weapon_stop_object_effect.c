// weapon_stop_object_effect  (Ghidra: FUN_004c48a0; named from
// out/phase4/items_functions.md, "Stops/clears whatever sound or effect is currently playing
// for a weapon trigger's owning object")
// address 0x4c48a0, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump 0x4c48a0..0x4c48ef)
// evidence: types/objects.h object.flags (_object_no_collision_bit), object.parent_object
//   (0x11c) -- the same holder-redirect idiom as weapon_play_trigger_tag_effect.
// register convention: item index in EDX; tag id in ESI (unaff_ESI).
// blam-cc: EDX -> item_index, ESI -> tag_id
// UNSURE: effect_new_at_texture_coordinate is called with two literal -1 arguments regardless of item_index/tag_id;
// preserved literally rather than "corrected".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"
#include "fn_effects.h"

extern data_array *object_data; // 0x008603b0


uint32_t weapon_stop_object_effect(datum_index item_index, datum_index tag_id)
{
    object *item_obj;

    if (tag_id == (datum_index)0xffffffff) {
        return 0xffffffff;
    }

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    if ((item_obj->flags & _object_no_collision_bit) != 0 && item_obj->parent_object != (datum_index)0xffffffff) {
        item_index = item_obj->parent_object;
    }
    if (item_index != (datum_index)0xffffffff) {
        // FIXED (objdump 0x4c48de..0x4c48e7): EAX = the effect tag, EDX = the (root) item, CX = -1, stack = (-1, -1)
        return effect_new_at_texture_coordinate(tag_id, item_index, -1, -1, -1);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4c48a0):

undefined4 FUN_004c48a0(void)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  uint in_EDX;
  int unaff_ESI;

  uVar3 = 0xffffffff;
  if (unaff_ESI != -1) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
    if (((*(byte *)(iVar1 + 0x10) & 1) != 0) &&
       (uVar2 = *(uint *)(iVar1 + 0x11c), uVar2 != 0xffffffff)) {
      in_EDX = uVar2;
    }
    if (in_EDX != 0xffffffff) {
      uVar3 = FUN_004506d0(0xffffffff,0xffffffff);
    }
  }
  return uVar3;
}
#endif
