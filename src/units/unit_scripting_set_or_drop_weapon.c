// unit_scripting_set_or_drop_weapon  (Ghidra: FUN_0056ddb0)
// address 0x56ddb0, size 270 bytes, name confidence 0.3, rewrite confidence 0.2
// functions.md: "Script/console-callable helper that resolves weapon-name arguments to object
// ids and updates or drops the unit's selected weapon accordingly."
// evidence: types/units.h unit_data.weapons[4] (0x2f8), .current_weapon_index (0x2f2),
//   .desired_weapon_index (0x2f4); unit_get_weapon_object_index.c (0x569970),
//   unit_ready_desired_weapon.c (0x56d6e0), unit_drop_current_weapon (0x56dec0, this same
//   batch).
// blam-cc: in_EAX -> message. `local_c`/`local_8`/`local_4` are ambient script-argument locals
//   this decompilation never shows being written; modelled as explicit parameters.
// UNSURE: message_delta_decode_compound_field/message_delta_decode_compound_field_staged's real roles, and the exact meaning of `local_c`/`local_8`
//   (most plausibly a target-object slot index and a from-weapon-index script argument) are not
//   recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern uint8_t *network_message_table; // 0x00687130, shared with unit_dispatch_seat_exit_message.c

extern uint8_t message_delta_decode_compound_field(void); // 0x4ec590, UNSURE signature
extern void message_delta_decode_compound_field_staged(void);    // 0x4ec670, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970
extern void unit_ready_desired_weapon(uint32_t unit_index); // 0x56d6e0
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0

void unit_scripting_set_or_drop_weapon(int32_t *message, int32_t target_slot, int32_t from_slot, uint8_t force)
    // blam-cc: in_EAX -> message, ambient -> target_slot, from_slot, force
{
    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }
    if ((message_delta_decode_compound_field() == 0) || (target_slot == 0)) {
        return;
    }

    int32_t *player_units = *(int32_t **)(network_message_table + 0x28);
    uint32_t unit_index = (uint32_t)player_units[target_slot];
    if (unit_index == 0xffffffff) {
        return;
    }
    object *unit_obj = object_try_and_get(unit_index, _object_mask_unit);
    if (unit_obj == (object *)0) {
        return;
    }
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    datum_index from_weapon = k_datum_index_none;
    if (from_slot != 0) {
        from_weapon = (datum_index)player_units[from_slot];
    }

    datum_index current_weapon = unit_get_weapon_object_index(unit_index, 0); // UNSURE: slot arg unrecovered
    if (current_weapon != from_weapon) {
        for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
            if (unit->weapons[i] == from_weapon) {
                unit->desired_weapon_index = (int16_t)i;
                unit_ready_desired_weapon(unit_index);
                break;
            }
        }
    }

    datum_index current_after = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_after = unit->weapons[unit->current_weapon_index];
    }
    if (current_after == from_weapon) {
        unit_drop_current_weapon(unit_index, force);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56ddb0):

void FUN_0056ddb0(void)

{
  short sVar1;
  uint uVar2;
  char cVar3;
  undefined4 *in_EAX;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int local_c;
  int local_8;
  undefined4 local_4;

  if (*(int *)*in_EAX == 0) {
    cVar3 = FUN_004ec590();
    if ((cVar3 != '\0') && (local_c != 0)) {
      iVar5 = *(int *)(PTR_DAT_00687130 + 0x28);
      uVar2 = *(uint *)(iVar5 + local_c * 4);
      if ((uVar2 != 0xffffffff) && (iVar4 = object_try_and_get(3), iVar4 != 0)) {
        iVar7 = -1;
        if (local_8 != 0) {
          iVar7 = *(int *)(iVar5 + local_8 * 4);
        }
        iVar5 = unit_get_weapon_object_index();
        if (iVar5 != iVar7) {
          iVar5 = 0;
          piVar6 = (int *)(iVar4 + 0x2f8);
          do {
            if (*piVar6 == iVar7) {
              *(short *)(iVar4 + 0x2f4) = (short)iVar5;
              unit_ready_desired_weapon(uVar2,1);
              break;
            }
            iVar5 = iVar5 + 1;
            piVar6 = piVar6 + 1;
          } while (iVar5 < 4);
        }
        iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        sVar1 = *(short *)(iVar5 + 0x2f2);
        iVar4 = -1;
        if (sVar1 != -1) {
          iVar4 = *(int *)(iVar5 + 0x2f8 + sVar1 * 4);
        }
        if (iVar4 == iVar7) {
          unit_drop_current_weapon(uVar2,local_4);
          return;
        }
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
