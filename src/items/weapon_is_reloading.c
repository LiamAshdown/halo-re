// weapon_is_reloading  (Ghidra: FUN_004c2ad0; renamed per items_types_notes.md:
// "magazines[0].state == 1")
// address 0x4c2ad0, size 69 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: types/items.h weapon_data.magazines[0].state, weapon_magazine_state_enum.
// register convention: item index in EAX.
// blam-cc: EAX -> item_index
// UNSURE: the original packs item_index bits into the unused upper 24 return bits (a codegen
// artifact); every real caller only ever tests the low byte, so this returns a plain bool.

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

// Reports whether a weapon's first magazine is currently mid-reload.
int32_t weapon_is_reloading(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    return weapon_tag->magazines.count > 0 && wd->magazines[0].state == _weapon_magazine_reloading;
}

#if 0
Original Ghidra decompilation (0x4c2ad0):

uint FUN_004c2ad0(void)

{
  uint *puVar1;
  uint uVar2;
  uint in_EAX;
  uint uVar3;

  uVar2 = (in_EAX & 0xffff) * 3;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar3 = uVar2 & 0xffffff00;
  if ((0 < *(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x4f0)) &&
     ((short)puVar1[0xac] == 1)) {
    uVar3 = CONCAT31((int3)(uVar2 >> 8),1);
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
