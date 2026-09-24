// unit_count_deployed_weapons  (Ghidra: FUN_0056d990)
// address 0x56d990, size 106 bytes, name confidence 0.35, rewrite confidence 0.5
// functions.md: "Counts how many of the unit's carried weapons are not marked with the 0x10
// 'undeployed' flag."
// evidence: types/units.h unit_data.weapons[4] (0x2f8); the weapon tag field at +0x308 is not
// named by this module's header (Weapon tag territory), kept as a raw offset.
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

int16_t unit_count_deployed_weapons(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    int16_t count = 0;
    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        datum_index weapon_index = unit->weapons[i];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            uint8_t *weapon_tag = (uint8_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if ((*(uint8_t *)(weapon_tag + 0x308) & 0x10) == 0) {
                count++;
            }
        }
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x56d990):

undefined4 FUN_0056d990(void)

{
  uint in_EAX;
  int iVar1;
  short sVar2;
  uint *puVar3;

  sVar2 = 0;
  puVar3 = (uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x2f8);
  iVar1 = 4;
  do {
    if ((*puVar3 != 0xffffffff) &&
       ((*(byte *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (*puVar3 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14) + 0x308) & 0x10) == 0)) {
      sVar2 = sVar2 + 1;
    }
    puVar3 = puVar3 + 1;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  return CONCAT22((short)((in_EAX & 0xffff) * 3 >> 0x10),sVar2);
}
#endif
