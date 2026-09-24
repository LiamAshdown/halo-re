// unit_find_empty_weapon_slot  (Ghidra: FUN_0056d660)
// address 0x56d660, size 53 bytes, name confidence 0.45, rewrite confidence 0.6
// functions.md: "Returns the index of the first empty weapon inventory slot, or an invalid
// index if the unit's inventory is full."
// evidence: types/units.h unit_data.weapons[4] (0x2f8), k_maximum_weapons_per_unit (4).
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

int16_t unit_find_empty_weapon_slot(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        if (unit->weapons[slot] == k_datum_index_none) {
            return slot;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x56d660):

int FUN_0056d660(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = 0;
  do {
    if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x2f8 +
                (short)iVar1 * 4) == -1) {
      return iVar1;
    }
    iVar1 = iVar1 + 1;
  } while ((short)iVar1 < 4);
  return CONCAT22((short)((uint)iVar1 >> 0x10),0xffff);
}
#endif
