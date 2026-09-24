// unit_drop_inventory_weapons_except_current  (Ghidra: FUN_0056d360)
// address 0x56d360, size 158 bytes, name confidence 0.35, rewrite confidence 0.5
// functions.md: "Releases every weapon in the unit's inventory except the currently equipped
// one, clearing the corresponding slot and any next/desired-weapon references to it."
// evidence: types/units.h unit_data.weapons[4] (0x2f8), .current_weapon_index (0x2f2),
//   .desired_weapon_index (0x2f4); types/objects.h object.network_role (0x04).
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void object_delete_unparented(uint32_t object_index);        // 0x4f5aa0, UNSURE signature
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, UNSURE signature

void unit_drop_inventory_weapons_except_current(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if ((weapon_index != k_datum_index_none) && (slot != unit->current_weapon_index)) {
            object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            if (weapon_obj->network_role == 0) {
                object_delete_unparented(weapon_index);
                object_delete_recursive(weapon_index, 0);
            } else if (weapon_obj->network_role == 3) {
                object_delete_recursive(weapon_index, 0);
            }
            unit->weapons[slot] = k_datum_index_none;
            if (slot == unit->desired_weapon_index) {
                unit->desired_weapon_index = -1;
            }
            if (slot == unit->current_weapon_index) {
                unit->current_weapon_index = -1;
            }
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56d360):

void FUN_0056d360(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint in_EAX;
  short sVar4;
  uint *puVar5;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar4 = 0;
  puVar5 = (uint *)(iVar1 + 0x2f8);
  do {
    uVar2 = *puVar5;
    if ((uVar2 != 0xffffffff) && (sVar4 != *(short *)(iVar1 + 0x2f2))) {
      iVar3 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 4);
      if (iVar3 == 0) {
        object_delete_unparented();
LAB_0056d3be:
        object_delete_recursive(uVar2,0);
      }
      else if (iVar3 == 3) goto LAB_0056d3be;
      *puVar5 = 0xffffffff;
      if (sVar4 == *(short *)(iVar1 + 0x2f4)) {
        *(undefined2 *)(iVar1 + 0x2f4) = 0xffff;
      }
      if (sVar4 == *(short *)(iVar1 + 0x2f2)) {
        *(undefined2 *)(iVar1 + 0x2f2) = 0xffff;
      }
    }
    sVar4 = sVar4 + 1;
    puVar5 = puVar5 + 1;
    if (3 < sVar4) {
      return;
    }
  } while( true );
}
#endif
