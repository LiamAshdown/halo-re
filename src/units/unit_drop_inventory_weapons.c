// unit_drop_inventory_weapons  (Ghidra: already named unit_drop_inventory_weapons)
// address 0x56f060, size 141 bytes
// name confidence: 0.75 (cea-pdb hint 'unit_drop_inventory_weapons'; functions.md matches)
// rewrite confidence: 0.85 (REWRITTEN 2026-09-28 against objdump 0x56f060..0x56f0ec: the dropped handle (ESI) is what gets tested and deleted.)
// evidence: types/units.h unit_data.weapons[4] (0x2f8), .current_weapon_index (0x2f2),
//   .desired_weapon_index (0x2f4); callee unit_drop_object_from_hand (0x56ed00, this batch).
// UNSURE: weapon_is_out_of_ammo()'s register argument is not visible; guessed as the dropped weapon's
//   object index (the only value in scope at that point besides the unit).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;      // 0x008603b0
extern int16_t network_game_mode; // 0x00719720 (a WORD; 0x719722 is the screenshot counter)

extern uint8_t weapon_is_out_of_ammo(uint32_t object_index); // 0x4c2c70, EAX; AL result
extern void object_delete(uint32_t object_index);   // 0x4f5bd0, UNSURE exact signature
extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00

// Drops every weapon currently carried in the unit's inventory except the one currently in
// hand, clearing each inventory slot as it is dropped and redirecting the desired-weapon index
// to the current weapon if it pointed at a dropped slot.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> unit_index
void unit_drop_inventory_weapons(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot;

    for (slot = 0; slot < 4; slot++) {
        datum_index *weapon = &unit->weapons[slot];

        datum_index dropped = *weapon; // ESI

        if (dropped != k_datum_index_none && slot != unit->current_weapon_index) {
            unit_drop_object_from_hand(unit_index, dropped);
            if (slot == unit->desired_weapon_index) {
                unit->desired_weapon_index = unit->current_weapon_index;
            }
            *weapon = k_datum_index_none;

            // 0x56f0bb / 0x56f0d7: EAX = the dropped weapon (ESI). FIXED 2026-09-28: the draft passed the slot after
            // clearing it (-1), which crashed in weapon_is_out_of_ammo when the player died holding two weapons.
            if (weapon_is_out_of_ammo(dropped) == 0 && network_game_mode == 0) {
                object_delete(dropped);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x56f060):

void unit_drop_inventory_weapons(uint param_1)

{
  int iVar1;
  char cVar2;
  int *piVar3;
  short sVar4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  sVar4 = 0;
  piVar3 = (int *)(iVar1 + 0x2f8);
  do {
    if ((*piVar3 != -1) && (sVar4 != *(short *)(iVar1 + 0x2f2))) {
      unit_drop_object_from_hand(param_1,*piVar3);
      if (sVar4 == *(short *)(iVar1 + 0x2f4)) {
        *(undefined2 *)(iVar1 + 0x2f4) = *(undefined2 *)(iVar1 + 0x2f2);
      }
      *piVar3 = -1;
      cVar2 = FUN_004c2c70();
      if ((cVar2 == '\0') && (DAT_00719720 == 0)) {
        object_delete();
      }
    }
    sVar4 = sVar4 + 1;
    piVar3 = piVar3 + 1;
  } while (sVar4 < 4);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
