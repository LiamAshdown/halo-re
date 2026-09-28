// player_get_vehicle  (Ghidra: FUN_004ab170, named in phase 4)
// address 0x4ab170, size 105 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: rewritten from objdump 0x4ab170..0x4ab1d8 in the phase-4 review. Renamed from
// player_index_from_unit_index: ECX is a player datum (checked against player_data, salt 0
// accepted), the object looked up is the unit of that player (player +0x34, passed in ECX
// to object_try_and_get with type mask 3), and the result is the parent object of that unit
// (+0x11c) when the unit sits in a seat (unit +0x2f0 not -1): the vehicle the player rides.
// Any failed step returns -1. The first rewrite called object_try_and_get without the object.
// register convention: ECX player datum; returns EAX.
//   // blam-cc: ECX -> player_index
// FIXED (register inputs, objdump): note phrasing only -- the reversed "player_index -> ECX"
// form the checker cannot parse, rewritten as "ECX -> player_index".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "interface.h"

extern data_array *player_data; // 0x0087a480, stride 0x200

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index

// blam-cc: ECX -> player_index
datum_index player_get_vehicle(datum_index player_index)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)((uint32_t)player_index >> 16);
    player *p;
    object *unit;

    if (player_index == (datum_index)-1 || index < 0 || index >= player_data->maximum_count) {
        return (datum_index)-1;
    }
    p = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * index);
    if (p->identifier == 0 || (salt != 0 && p->identifier != salt)) {
        return (datum_index)-1;
    }
    unit = object_try_and_get(p->unit, 3);
    if (unit == 0 || ((unit_object *)unit)->base.parent_object == (datum_index)-1 ||
        ((unit_object *)unit)->unit.vehicle_seat_index == -1) {
        return (datum_index)-1;
    }
    return ((unit_object *)unit)->base.parent_object;
}

#if 0
Original Ghidra decompilation (0x4ab170):

int player_index_from_unit_index(void)

{
  int iVar1;
  short sVar2;
  int in_ECX;
  short sVar3;
  int iVar4;

  iVar4 = -1;
  if (((((in_ECX != -1) && (sVar2 = (short)in_ECX, -1 < sVar2)) &&
       (sVar2 < *(short *)(DAT_0087a480 + 0x20))) &&
      (sVar2 = *(short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2 +
                         *(int *)(DAT_0087a480 + 0x34)), sVar2 != 0)) &&
     (((sVar3 = (short)((uint)in_ECX >> 0x10), sVar3 == 0 || (sVar2 == sVar3)) &&
      ((iVar1 = object_try_and_get(3), iVar1 != 0 &&
       ((*(int *)(iVar1 + 0x11c) != -1 && (*(short *)(iVar1 + 0x2f0) != -1)))))))) {
    iVar4 = *(int *)(iVar1 + 0x11c);
  }
  return iVar4;
}
#endif
