// unit_clear_selected_equipment  (Ghidra: FUN_0056d2c0)
// address 0x56d2c0, size 57 bytes, name confidence 0.3, rewrite confidence 0.6
// functions.md: "Clears the unit's currently selected secondary item field, releasing it first
// via FUN_0056ed00."
// evidence: types/units.h unit_data.equipment_object_index (0x318);
//   src/units/unit_drop_object_from_hand.c (unit_index, dropped_object_index), already
//   established from the other half of this session's batch.
// blam-cc: in_ECX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00

void unit_clear_selected_equipment(uint32_t unit_index) // blam-cc: in_ECX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (unit->equipment_object_index != k_datum_index_none) {
        unit_drop_object_from_hand(unit_index, unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56d2c0):

void FUN_0056d2c0(void)

{
  int iVar1;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (*(int *)(iVar1 + 0x318) != -1) {
    unit_drop_object_from_hand();
    *(undefined4 *)(iVar1 + 0x318) = 0xffffffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
