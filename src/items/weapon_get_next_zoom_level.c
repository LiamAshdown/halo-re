// weapon_get_next_zoom_level  (Ghidra: FUN_004c2cf0; renamed per items_types_notes.md:
// "same two tag fields, wraps against zoom_levels - 1")
// address 0x4c2cf0, size 126 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/items.h weapon_data.magazines[0].state; types/tags.h Weapon.zoom_levels
//   (0x3da).
// register convention: current zoom level in EAX; item index in ECX.
// blam-cc: EAX -> current_level, ECX -> item_index

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

// Advances a weapon's zoom level by one, or -1/0 once the top level is passed. Leaves the
// level unchanged while the first magazine is mid-reload.
int32_t weapon_get_next_zoom_level(int32_t current_level, datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int16_t level;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (weapon_tag->magazines.count < 1 || wd->magazines[0].state != _weapon_magazine_reloading) {
        level = (int16_t)current_level;
        if (level >= 0 && level < weapon_tag->zoom_levels - 1) {
            return current_level + 1;
        }
        return (level != weapon_tag->zoom_levels - 1) ? 0 : -1;
    }
    return current_level;
}

#if 0
Original Ghidra decompilation (0x4c2cf0):

int FUN_004c2cf0(void)

{
  uint *puVar1;
  int iVar2;
  short sVar3;
  int in_EAX;
  uint in_ECX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x4f0) < 1) ||
     ((short)puVar1[0xac] != 1)) {
    sVar3 = (short)in_EAX;
    if ((-1 < sVar3) && ((int)sVar3 < *(short *)(iVar2 + 0x3da) + -1)) {
      return in_EAX + 1;
    }
    in_EAX = ((int)sVar3 != *(short *)(iVar2 + 0x3da) + -1) - 1;
  }
  return in_EAX;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
