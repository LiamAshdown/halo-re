// weapon_prevents_grenade_throwing  (Ghidra: FUN_004c2f30; renamed per items_types_notes.md:
// "returns (Weapon.weapon_flags >> 6) & 1 ... OR-ed with weapon_state in 5..10")
// address 0x4c2f30, size 79 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/tags.h Weapon.weapon_flags (WeaponFlags bit 6 = prevents_grenade_throwing);
//   types/items.h weapon_state (5..10 = the reload/charged/ready/put-away states).
// register convention: item index in ECX.
// blam-cc: ECX -> item_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Reports whether an item's weapon currently prevents throwing a grenade: either the tag flag
// is set, its state is one of the "busy" states (5..10), or the item handle is invalid.
uint32_t weapon_prevents_grenade_throwing(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int8_t state;

    if (item_index == (datum_index)0xffffffff) {
        return 1;
    }

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    state = (int8_t)wd->state;
    if (state > 4 && state < 11) {
        return 1;
    }
    return (weapon_tag->weapon_flags >> 6) & 1;
}

#if 0
Original Ghidra decompilation (0x4c2f30):

uint FUN_004c2f30(void)

{
  char cVar1;
  uint *puVar2;
  uint uVar3;
  uint in_ECX;

  uVar3 = 1;
  if (in_ECX != 0xffffffff) {
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    cVar1 = (char)puVar2[0x8e];
    uVar3 = *(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 6 &
            0xffffff01;
    if (('\x04' < cVar1) && (cVar1 < '\v')) {
      uVar3 = 1;
    }
  }
  return uVar3;
}
#endif
