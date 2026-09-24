// unit_release_selected_equipment  (Ghidra: FUN_0056d300)
// address 0x56d300, size 93 bytes, name confidence 0.3, rewrite confidence 0.6
// functions.md: "Releases the unit's currently selected secondary item by dispatching to a
// type-specific release routine, then clears the selection."
// evidence: types/units.h unit_data.equipment_object_index (0x318); types/objects.h
//   object.network_role (0x04, 0 = delete-unparented path, 3 = delete-recursive path).
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void object_delete_unparented(uint32_t object_index);        // 0x4f5aa0, UNSURE signature
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, UNSURE signature

void unit_release_selected_equipment(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    datum_index equipment_index = unit->equipment_object_index;
    if (equipment_index == k_datum_index_none) {
        return;
    }
    object *equipment_obj = ((object_header *)object_data->data)[equipment_index & 0xffff].data;
    if (equipment_obj->network_role == 0) {
        object_delete_unparented(equipment_index);
    } else if (equipment_obj->network_role != 3) {
        goto clear;
    }
    object_delete_recursive(equipment_index, 0);
clear:
    unit->equipment_object_index = k_datum_index_none;
    return;
}

#if 0
Original Ghidra decompilation (0x56d300):

void FUN_0056d300(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = *(uint *)(iVar1 + 0x318);
  if (uVar2 == 0xffffffff) {
    return;
  }
  iVar3 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 4);
  if (iVar3 == 0) {
    object_delete_unparented();
  }
  else if (iVar3 != 3) goto LAB_0056d350;
  object_delete_recursive(uVar2,0);
LAB_0056d350:
  *(undefined4 *)(iVar1 + 0x318) = 0xffffffff;
  return;
}
#endif
