// item_set_holder  (Ghidra: FUN_004bcfc0; renamed per types/items.h: "item_set_holder
// (0x4bcfc0) is the proof for bits 0x01 and 0x02: it reads the word once, computes
// (flags & ~0x40) | 1, and then either | 3 when the holder's unit_data.controlling_player
// (unit 0x218) is set or (flags & ~0x42) | 1 when it is -1")
// address 0x4bcfc0, size 183 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: types/items.h item_flags (_item_in_inventory_bit 0x01, _item_held_by_player_bit
//   0x02, _item_unknown_40_bit 0x40, _item_at_rest_on_structure_bit 0x08,
//   _item_does_not_accelerate_bit 0x20); types/units.h unit_data.controlling_player (0x218);
//   types/objects.h object (owner_linkage 0x0c0, location_leaf_index 0x098,
//   location_cluster_index 0x09c, unknown_09e 0x09e, network_role 0x004, flags 0x010); callee
//   object_list_membership_set (0x4f7450, already named in src/objects/).
// register convention: item index in ECX (in_ECX), new holder object index (or -1 to clear) in
//   EDX (in_EDX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0

extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, objects module

// Sets (or, with holder_index == -1, clears) the object holding an item, updating the item's
// in-inventory / held-by-player flags, its cached owner linkage, and its tracked/cluster
// membership.
void item_set_holder(uint32_t item_index, datum_index holder_index) // blam-cc: ECX -> item_index, EDX -> holder_index
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if (holder_index == (datum_index)0xffffffff) {
        item->flags &= ~(uint32_t)(_item_in_inventory_bit | _item_held_by_player_bit);
        return;
    }

    {
        uint32_t original_flags = item->flags;
        object *holder = ((object_header *)object_data->data)[holder_index & 0xffff].data;
        unit_data *holder_unit = (unit_data *)((uint8_t *)holder + k_unit_data_offset);

        item->flags = (original_flags & ~(uint32_t)_item_unknown_40_bit) | _item_in_inventory_bit;

        if (holder_unit->controlling_player == (datum_index)0xffffffff) {
            item->flags = (original_flags & ~(uint32_t)(_item_held_by_player_bit | _item_unknown_40_bit))
                | _item_in_inventory_bit;
        } else {
            item->flags = (original_flags & ~(uint32_t)_item_unknown_40_bit)
                | (_item_in_inventory_bit | _item_held_by_player_bit);
        }

        obj->owner_linkage = holder_unit->controlling_player;

        object_list_membership_set(item_index, 0);

        item->flags &= ~(uint32_t)(_item_at_rest_on_structure_bit | _item_does_not_accelerate_bit);
        obj->location_leaf_index = -1;
        obj->location_cluster_index = -1;
        obj->unknown_09e = -1;

        if (obj->network_role == 0) {
            obj->flags |= _object_changed_bit;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4bcfc0):

void FUN_004bcfc0(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint in_ECX;
  uint in_EDX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (in_EDX == 0xffffffff) {
    *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) & 0xfffffffc;
  }
  else {
    uVar2 = *(uint *)(iVar1 + 500);
    uVar4 = uVar2 & 0xffffffbf;
    iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
    *(uint *)(iVar1 + 500) = uVar4 | 1;
    if (*(int *)(iVar3 + 0x218) == -1) {
      uVar4 = uVar2 & 0xffffffbd | 1;
    }
    else {
      uVar4 = uVar4 | 3;
    }
    *(uint *)(iVar1 + 500) = uVar4;
    *(undefined4 *)(iVar1 + 0xc0) = *(undefined4 *)(iVar3 + 0x218);
    FUN_004f7450(0);
    *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) & 0xffffffd7;
    *(undefined4 *)(iVar1 + 0x98) = 0xffffffff;
    *(undefined2 *)(iVar1 + 0x9c) = 0xffff;
    *(undefined2 *)(iVar1 + 0x9e) = 0xffff;
    if (*(int *)(iVar1 + 4) == 0) {
      *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x4000000;
      return;
    }
  }
  return;
}
#endif
