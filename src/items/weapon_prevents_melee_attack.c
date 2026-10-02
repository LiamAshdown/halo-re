// weapon_prevents_melee_attack  (Ghidra: FUN_004c2ee0; renamed per items_types_notes.md:
// "(weapon_flags >> 9) & 1 = prevents_melee_attack, OR trigger 0 charging/charged")
// address 0x4c2ee0, size 79 bytes
// VERIFIED against disassembly 0x4c2ee0..0x4c2f2f (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: types/tags.h Weapon.weapon_flags (WeaponFlags bit 9 = prevents_melee_attack);
//   types/items.h weapon_trigger_effect_state (_weapon_trigger_effect_charging = 2,
//   _weapon_trigger_effect_charged = 3).
// register convention: item index in ECX.
// blam-cc: ECX -> item_index
// The original does `shr eax,9; and al,1` (only the low byte is meaningful) and forces AL=1 when the
// trigger-0 effect state (+0x261) is 2 or 3; the C below returns the same 0/1 value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Reports whether an item's weapon currently prevents a melee attack: either the tag flag is
// set, its first trigger is charging/charged, or the item handle itself is invalid.
uint32_t weapon_prevents_melee_attack(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int8_t effect_state;

    if (item_index == (datum_index)0xffffffff) {
        return 1;
    }

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    effect_state = wd->triggers[0].effect_state;
    if (effect_state == _weapon_trigger_effect_charging || effect_state == _weapon_trigger_effect_charged) {
        return 1;
    }
    return (weapon_tag->weapon_flags >> 9) & 1;
}

#if 0
Original Ghidra decompilation (0x4c2ee0):

uint FUN_004c2ee0(void)

{
  char cVar1;
  uint *puVar2;
  uint uVar3;
  uint in_ECX;

  uVar3 = 1;
  if (in_ECX != 0xffffffff) {
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    cVar1 = *(char *)((int)puVar2 + 0x261);
    uVar3 = *(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 9 &
            0xffffff01;
    if ((cVar1 == '\x02') || (cVar1 == '\x03')) {
      uVar3 = 1;
    }
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
