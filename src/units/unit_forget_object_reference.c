// unit_forget_object_reference  (Ghidra: no function created; the phase-4 types agent carved a
//   placeholder "missed_56f0f0" from the object_type_definition vtable evidence)
// address 0x56f0f0, size 203 bytes
// name confidence 0.4, rewrite confidence 0.75
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are ... 0x56f0f0
//   (+0x3c) ...". Every field this function touches is a datum_index reference to some other
//   object that types/units.h already names (throwing_grenade_projectile +0x294,
//   driver_unit_index +0x324, gunner_unit_index +0x328, weapons[4] +0x2f8, equipment_object_index
//   +0x318, unknown_40c "the object 0x5674a0 recorded as responsible" +0x40c), and every one of
//   them is compared against the function's second parameter and cleared to -1 on a match, with
//   current_weapon_index / desired_weapon_index also cleared and desired_weapon_index refreshed
//   from unit_find_next_zone_permitted_weapon_slot when the current weapon slot was the one
//   cleared. This is the standard "an object is being deleted -- scrub every reference to it"
//   pattern; the object_type_definition column that runs it for every live unit whenever any
//   object is destroyed.
// register convention: object index and the datum_index of the object being forgotten, in two
//   register arguments (matches every other per-object helper in this address range taking a
//   second value); blam-cc: object_index, forgotten_object_index.
// Cleanup-pass review (objdump 0x56f0f0..0x56f1ba): the weapon-slot helper takes the unit in
//   EAX (= object_index, never reloaded) plus (start_slot -1, direction 0); the draft passed
//   (-1, 0) as (unit, start). Fixed to match src/units/unit_find_next_zone_permitted_weapon_slot.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction);
    // 0x56dba0, blam-cc: EAX -> unit_index, stack -> (start_slot, direction)

// object_type_definition "unit" row, +0x3c column. Scrubs every reference this unit holds to
// `forgotten_object_index` (a thrown grenade, a driver/gunner seat occupant, an inventory weapon,
// held equipment, or the last damage attacker), and, if the current weapon slot was cleared,
// picks a new one via unit_find_next_zone_permitted_weapon_slot.
void unit_forget_object_reference(uint32_t object_index, datum_index forgotten_object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot;

    if (unit->throwing_grenade_projectile == forgotten_object_index) {
        unit->throwing_grenade_projectile = (datum_index)-1;
    }
    if (unit->driver_unit_index == forgotten_object_index) {
        unit->driver_unit_index = (datum_index)-1;
    }
    if (unit->gunner_unit_index == forgotten_object_index) {
        unit->gunner_unit_index = (datum_index)-1;
    }

    for (slot = 0; slot < 4; slot++) {
        if (unit->weapons[slot] == forgotten_object_index) {
            unit->weapons[slot] = (datum_index)-1;
            if (slot == unit->desired_weapon_index) {
                unit->desired_weapon_index = -1;
            }
            if (slot == unit->current_weapon_index) {
                unit->current_weapon_index = -1;
            }
        }
    }

    if (unit->current_weapon_index == -1) {
        // EAX still holds object_index at 0x56f18c; the pushes are start_slot -1 and direction 0
        unit->desired_weapon_index = unit_find_next_zone_permitted_weapon_slot(object_index, -1, 0);
    }

    if (unit->equipment_object_index == forgotten_object_index) {
        unit->equipment_object_index = (datum_index)-1;
    }
    if (unit->delayed_damage_responsible_object == forgotten_object_index) {
        unit->delayed_damage_responsible_object = (datum_index)-1;
    }
}

#if 0
Original Ghidra decompilation (0x56f0f0):

void missed_56f0f0(uint param_1,int param_2)

{
  int iVar1;
  undefined2 uVar2;
  short sVar3;
  int *piVar4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (*(int *)(iVar1 + 0x294) == param_2) {
    *(undefined4 *)(iVar1 + 0x294) = 0xffffffff;
  }
  if (*(int *)(iVar1 + 0x324) == param_2) {
    *(undefined4 *)(iVar1 + 0x324) = 0xffffffff;
  }
  if (*(int *)(iVar1 + 0x328) == param_2) {
    *(undefined4 *)(iVar1 + 0x328) = 0xffffffff;
  }
  sVar3 = 0;
  piVar4 = (int *)(iVar1 + 0x2f8);
  do {
    if (*piVar4 == param_2) {
      *piVar4 = -1;
      if (sVar3 == *(short *)(iVar1 + 0x2f4)) {
        *(undefined2 *)(iVar1 + 0x2f4) = 0xffff;
      }
      if (sVar3 == *(short *)(iVar1 + 0x2f2)) {
        *(undefined2 *)(iVar1 + 0x2f2) = 0xffff;
      }
    }
    sVar3 = sVar3 + 1;
    piVar4 = piVar4 + 1;
  } while (sVar3 < 4);
  if (*(short *)(iVar1 + 0x2f2) == -1) {
    uVar2 = unit_find_next_zone_permitted_weapon_slot(0xffffffff,0);
    *(undefined2 *)(iVar1 + 0x2f4) = uVar2;
  }
  if (*(int *)(iVar1 + 0x318) == param_2) {
    *(undefined4 *)(iVar1 + 0x318) = 0xffffffff;
  }
  if (*(int *)(iVar1 + 0x40c) == param_2) {
    *(undefined4 *)(iVar1 + 0x40c) = 0xffffffff;
  }
  return;
}
#endif
