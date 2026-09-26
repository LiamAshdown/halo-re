// unit_find_next_zone_permitted_weapon_slot  (Ghidra: FUN_0056dba0)
// address 0x56dba0, size 292 bytes, name confidence 0.4, rewrite confidence 0.3
// functions.md: "Finds the next valid, zone-permitted weapon inventory slot starting from
// param_1, searching forward or backward depending on param_2."
// evidence: types/units.h unit_data.weapons[4] (0x2f8), .weapon_ready_ticks[4] (0x308);
//   unit_check_weapon_use_permission.c (0x56da00, called here with the same implicit-EAX
//   convention). The weapon tag's own field at +0x308, bit 3, is not named by this module.
// blam-cc: in_EAX -> unit_index, param_1 -> start_slot, param_2 -> direction.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index); // 0x56da00, ESI unit, EDI weapon

int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction)
    // blam-cc: in_EAX, param_1, param_2
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    int16_t best_slot = -1;
    int32_t original = start_slot;
    if ((int16_t)start_slot == -1) {
        start_slot = 0;
        original = start_slot;
    }

    int32_t cursor = start_slot;
    do {
        int16_t slot = (int16_t)cursor;
        if ((unit->weapons[slot] != k_datum_index_none) && (unit_check_weapon_use_permission(unit_index, unit->weapons[slot]) != 0) /* 0x56dbe7..0x56dbf7: ESI = unit (EAX at entry), EDI = weapons[slot] */) {
            if ((direction != 0) || (best_slot == -1) ||
                (unit->weapon_ready_ticks[best_slot] < unit->weapon_ready_ticks[slot])) {
                best_slot = slot;
            }
            object *weapon_obj = ((object_header *)object_data->data)[unit->weapons[slot] & 0xffff].data;
            uint8_t *weapon_tag = (uint8_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if ((*(uint32_t *)(weapon_tag + 0x308) >> 3 & 1) != 0) {
                return best_slot;
            }
            if (slot != (int16_t)original) {
                return best_slot;
            }
        }
        if (direction < 0) {
            cursor = (slot == 0) ? 3 : (int32_t)slot - 1;
        } else if (slot == 3) {
            cursor = 0;
        } else {
            cursor = (int32_t)slot + 1;
        }
    } while ((int16_t)cursor != (int16_t)original);
    return best_slot;
}

#if 0
Original Ghidra decompilation (0x56dba0):

short FUN_0056dba0(int param_1,short param_2)

{
  int iVar1;
  char cVar2;
  uint in_EAX;
  short sVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int local_8;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  local_8 = -1;
  iVar5 = param_1;
  if ((short)param_1 == -1) {
    param_1 = 0;
    iVar5 = param_1;
  }
  do {
    sVar4 = (short)iVar5;
    iVar6 = (int)sVar4;
    if ((*(int *)(iVar1 + 0x2f8 + iVar6 * 4) != -1) && (cVar2 = FUN_0056da00(), cVar2 != '\0')) {
      if ((param_2 != 0) ||
         ((sVar3 = (short)local_8, sVar3 == -1 ||
          (*(int *)(iVar1 + 0x308 + sVar3 * 4) < *(int *)(iVar1 + 0x308 + iVar6 * 4))))) {
        sVar3 = sVar4;
        local_8 = iVar5;
      }
      if ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (*(uint *)(iVar1 + 0x2f8 + iVar6 * 4) & 0xffff) * 0xc) &
                              0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1) != 0) {
        return sVar3;
      }
      if (sVar4 != (short)param_1) {
        return (short)local_8;
      }
    }
    if (param_2 < 0) {
      if (sVar4 == 0) {
        iVar5 = 3;
      }
      else {
        iVar5 = iVar6 + -1;
      }
    }
    else if (sVar4 == 3) {
      iVar5 = 0;
    }
    else {
      iVar5 = iVar6 + 1;
    }
  } while ((short)iVar5 != (short)param_1);
  return (short)local_8;
}
#endif
