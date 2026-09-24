// unit_get_current_weapon_label  (Ghidra: unit_get_current_weapon_label, already named)
// address 0x56dfd0, size 96 bytes, name confidence 0.75, rewrite confidence 0.7
// functions.md: "Returns a label string for the unit's currently held weapon, or 'unarmed' if
// no weapon is equipped."
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .weapons[4] (0x2f8); the
// weapon tag field at +0x30c (label) matches unit_pickup_weapon.c and
// unit_check_weapon_use_permission.c's use of the same offset.
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

char *unit_get_current_weapon_label(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (unit->current_weapon_index != -1) {
        datum_index weapon_index = unit->weapons[unit->current_weapon_index];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            return (char *)(tag_instances[weapon_obj->definition_tag & 0xffff].data) + 0x30c;
        }
    }
    return "unarmed";
}

#if 0
Original Ghidra decompilation (0x56dfd0):

char * unit_get_current_weapon_label(void)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  uint in_EAX;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar2 + 0x2f2);
  if ((sVar1 != -1) && (uVar3 = *(uint *)(iVar2 + 0x2f8 + sVar1 * 4), uVar3 != 0xffffffff)) {
    return (char *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc
                                         ) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x30c);
  }
  return "unarmed";
}
#endif
