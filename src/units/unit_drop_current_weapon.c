// unit_drop_current_weapon  (Ghidra: unit_drop_current_weapon, already named)
// address 0x56dec0, size 260 bytes, name confidence 0.55, rewrite confidence 0.85 (call arguments fixed against objdump 0x56dec0..0x56dfc3)
// functions.md: "Detaches and drops the unit's current weapon object and selects a replacement
// desired-weapon slot."
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .weapons[4] (0x2f8),
//   .desired_weapon_index (0x2f4); types/objects.h object.flags (0x10, bit 0 =
//   _object_no_collision_bit); unit_find_next_zone_permitted_weapon_slot.c (0x56dba0);
//   src/units/unit_drop_object_from_hand.c (from the other half of this session's batch).
// blam-cc: param_1 -> unit_index, param_2 -> force.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern int32_t game_connection_role; // 0x00719720

extern void weapon_action_notify_for_unit(datum_index unit_index, int32_t action_code); // 0x492730, EAX, stack
extern int32_t weapon_put_away(datum_index item_index, int8_t force); // 0x4c28f0, ESI, AL
extern int32_t weapon_is_out_of_ammo(datum_index item_index); // 0x4c2c70, EAX
extern void object_delete(uint32_t object_index);                              // 0x4f5bd0, UNSURE signature
extern int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction); // 0x56dba0
extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00

uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    // 0x56defa: the start slot is pushed zero-extended
    int16_t next_slot = unit_find_next_zone_permitted_weapon_slot(unit_index, (int32_t)(uint16_t)unit->current_weapon_index, 1);

    if ((current_weapon != k_datum_index_none) &&
        ((next_slot != unit->current_weapon_index) || force) &&
        ((((object_header *)object_data->data)[current_weapon & 0xffff].data->flags & 1) == 0)) {
        if ((uint8_t)weapon_put_away(current_weapon, (int8_t)force) != 0) { // 0x56df4c: ESI weapon, AL force
            weapon_action_notify_for_unit(unit_index, 0xd);
            unit_drop_object_from_hand(unit_index, current_weapon);
            unit->weapons[unit->current_weapon_index] = k_datum_index_none;
            unit->current_weapon_index = -1;
            unit->desired_weapon_index = unit_find_next_zone_permitted_weapon_slot(unit_index, -1, 0);
            if (((uint8_t)weapon_is_out_of_ammo(current_weapon) == 0) && (game_connection_role == 0)) {
                object_delete(current_weapon);
            }
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56dec0):

undefined4 unit_drop_current_weapon(uint param_1,char param_2)

{
  int iVar1;
  char cVar2;
  short sVar3;
  undefined2 uVar4;
  uint uVar5;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  uVar5 = 0xffffffff;
  if (*(short *)(iVar1 + 0x2f2) != -1) {
    uVar5 = *(uint *)(iVar1 + 0x2f8 + *(short *)(iVar1 + 0x2f2) * 4);
  }
  sVar3 = FUN_0056dba0(*(undefined2 *)(iVar1 + 0x2f2),1);
  if ((uVar5 != 0xffffffff) &&
     (((sVar3 != *(short *)(iVar1 + 0x2f2) || (param_2 != '\0')) &&
      ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) + 0x10) & 1)
       == 0)))) {
    cVar2 = FUN_004c28f0();
    if (cVar2 != '\0') {
      FUN_00492730(0xd);
      unit_drop_object_from_hand(param_1,uVar5);
      *(undefined4 *)(iVar1 + 0x2f8 + *(short *)(iVar1 + 0x2f2) * 4) = 0xffffffff;
      *(undefined2 *)(iVar1 + 0x2f2) = 0xffff;
      uVar4 = FUN_0056dba0(0xffffffff,0);
      *(undefined2 *)(iVar1 + 0x2f4) = uVar4;
      cVar2 = FUN_004c2c70();
      if ((cVar2 == '\0') && (DAT_00719720 == 0)) {
        object_delete();
      }
      return 1;
    }
  }
  return 0;
}
#endif
