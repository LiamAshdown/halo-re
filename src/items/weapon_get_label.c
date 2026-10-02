// weapon_get_label  (Ghidra: FUN_004c24d0; renamed per items_types_notes.md)
// address 0x4c24d0, size 59 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/tags.h Weapon.label (TagString, 0x30c); global 0x0065512c k_empty_string.
// register convention: item index in ECX (unrecognized register parameter).
// blam-cc: ECX -> item_index

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
extern char k_empty_string[1];      // 0x0065512c

// Returns the weapon's tag-defined label string, or the empty string for an invalid item.
char *weapon_get_label(datum_index item_index)
{
    object *item_obj;
    Weapon *weapon_tag;

    if (item_index == (datum_index)0xffffffff) {
        return k_empty_string;
    }

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    return weapon_tag->label.string;
}

#if 0
Original Ghidra decompilation (0x4c24d0):

undefined1 * FUN_004c24d0(void)

{
  undefined1 *puVar1;
  uint in_ECX;

  puVar1 = &DAT_0065512c;
  if (in_ECX != 0xffffffff) {
    puVar1 = (undefined1 *)
             (*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) &
                       0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x30c);
  }
  return puVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
