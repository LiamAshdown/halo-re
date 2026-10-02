// unit_pick_and_ready_next_weapon  (Ghidra: FUN_0056d6a0)
// address 0x56d6a0, size 63 bytes, name confidence 0.3, rewrite confidence 0.6
// functions.md: "Recomputes which inventory slot should become the unit's next weapon and
// triggers the weapon-switch routine."
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .desired_weapon_index (0x2f4).
// blam-cc: unaff_ESI -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction); // 0x56dba0, EAX, stack
extern void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force); // 0x56d6e0, stack (unit, force)

void unit_pick_and_ready_next_weapon(uint32_t unit_index) // blam-cc: unaff_ESI
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    // 0x56d6c4: EAX = the unit, stack (current weapon index zero-extended, 0); the draft dropped the unit
    unit->desired_weapon_index = unit_find_next_zone_permitted_weapon_slot(unit_index,
        (int32_t)(uint16_t)unit->current_weapon_index, 0);
    unit_ready_desired_weapon(unit_index, 1);
    return;
}

#if 0
Original Ghidra decompilation (0x56d6a0):

void FUN_0056d6a0(void)

{
  int iVar1;
  undefined2 uVar2;
  uint unaff_ESI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  uVar2 = FUN_0056dba0(*(undefined2 *)(iVar1 + 0x2f2),0);
  *(undefined2 *)(iVar1 + 0x2f4) = uVar2;
  unit_ready_desired_weapon();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
