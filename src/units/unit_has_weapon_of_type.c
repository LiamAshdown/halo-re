// unit_has_weapon_of_type  (Ghidra: FUN_0056d610)
// address 0x56d610, size 76 bytes, name confidence 0.35, rewrite confidence 0.5
// functions.md: "Returns whether the unit currently carries a weapon of the given type in any
// inventory slot."
// evidence: types/units.h unit_data.weapons[4] (0x2f8); types/objects.h object.definition_tag
//   (0x00, compared here against a raw group-tag value).
// blam-cc: in_EAX -> unit_index, unaff_EBX -> weapon_group_tag.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint8_t unit_has_weapon_of_type(uint32_t unit_index, int32_t weapon_group_tag) // blam-cc: in_EAX, unaff_EBX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            if ((int32_t)weapon_obj->definition_tag == weapon_group_tag) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56d610):

uint FUN_0056d610(void)

{
  uint uVar1;
  uint uVar2;
  uint in_EAX;
  int unaff_EBX;
  short sVar3;

  uVar2 = (in_EAX & 0xffff) * 3;
  sVar3 = 0;
  while ((uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) +
                            0x2f8 + sVar3 * 4), uVar1 == 0xffffffff ||
         (**(int **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) != unaff_EBX))) {
    sVar3 = sVar3 + 1;
    if (3 < sVar3) {
      return uVar2 & 0xffffff00;
    }
  }
  return CONCAT31((int3)(uVar2 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
