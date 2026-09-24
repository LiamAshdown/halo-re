// player_unit_has_parent  (Ghidra: FUN_00477210; named per this rewrite)
// address 0x477210, size 111 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Tests whether a given player's unit currently has a
//   valid parent object (e.g. is boarding or seated in a vehicle)"); types/units.h unit_data
//   (vehicle_seat_index +0x2f0); types/objects.h object::parent_object (+0x11c),
//   _object_mask_unit.
// objdump -d -M intel --start-address=0x477210 --stop-address=0x477280 bin/halo.exe confirms
// the manual datum-validation idiom against player_data, and that the final result is a tail
// call into unit_seat_flag_bit2(EAX -> parent_object, CX -> vehicle_seat_index).
// register convention: ECX -> player_handle.
//   // blam-cc: ECX -> player_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_seat_flag_bit2(datum_index parent_object, int16_t vehicle_seat_index); // 0x56cd10,
    // units module, not in this batch; blam-cc: EAX -> parent_object, CX -> vehicle_seat_index

// Validates player_handle against player_data, fetches its unit (_object_mask_unit) and, if it
// has a parent object, forwards to unit_seat_flag_bit2 with that parent and the unit's
// vehicle_seat_index. Returns false for an invalid handle, a player with no unit, or a unit with
// no parent.
uint8_t player_unit_has_parent(datum_index player_handle)
    // blam-cc: ECX -> player_handle
{
    int16_t index;
    player *plr;
    object *unit_obj;

    if (player_handle == (datum_index)-1) {
        return 0;
    }
    index = (int16_t)player_handle;
    if (index < 0 || index >= player_data->maximum_count) {
        return 0;
    }
    plr = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)index * player_data->size);
    {
        int16_t salt = (int16_t)(player_handle >> 16);
        if (plr->identifier == 0 || (salt != 0 && plr->identifier != salt)) {
            return 0;
        }
    }

    unit_obj = object_try_and_get(plr->unit, _object_mask_unit);
    if (unit_obj == (object *)0 || unit_obj->parent_object == (datum_index)-1) {
        return 0;
    }
    return unit_seat_flag_bit2(unit_obj->parent_object, *(int16_t *)((uint8_t *)unit_obj + 0x2f0));
}

#if 0
Original Ghidra decompilation (0x477210), from tools/pack.py 0x477210:

undefined4 FUN_00477210(void)

{
  int iVar1;
  undefined4 uVar2;
  short sVar3;
  int in_ECX;
  short sVar4;

  if (((((in_ECX != -1) && (sVar3 = (short)in_ECX, -1 < sVar3)) &&
       (sVar3 < *(short *)(DAT_0087a480 + 0x20))) &&
      ((sVar3 = *(short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3 +
                          *(int *)(DAT_0087a480 + 0x34)), sVar3 != 0 &&
       ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar3 == sVar4)))))) &&
     ((iVar1 = object_try_and_get(3), iVar1 != 0 && (*(int *)(iVar1 + 0x11c) != -1)))) {
    uVar2 = FUN_0056cd10();
    return uVar2;
  }
  return 0;
}
#endif
