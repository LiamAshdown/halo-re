// weapon_must_be_readied  (Ghidra: FUN_004c2ea0; renamed per items_types_notes.md:
// "(weapon_flags >> 3) & 1 = must_be_readied")
// address 0x4c2ea0, size 55 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: types/tags.h Weapon.weapon_flags (0x308, WeaponFlags bit 3 = must_be_readied).
// register convention: item index in EAX.
// blam-cc: EAX -> item_index

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

// Tests the tag-defined must_be_readied flag (bit 3) on an item's weapon flags.
uint32_t weapon_must_be_readied(datum_index item_index)
{
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    return (weapon_tag->weapon_flags >> 3) & 1;
}

#if 0
Original Ghidra decompilation (0x4c2ea0):

uint FUN_004c2ea0(void)

{
  uint in_EAX;

  return *(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc
                                        ) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
