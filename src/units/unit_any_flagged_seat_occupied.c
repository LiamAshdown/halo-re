// unit_any_flagged_seat_occupied  (Ghidra: FUN_0056cc80)
// address 0x56cc80, size 120 bytes, name confidence 0.4, rewrite confidence 0.5
// functions.md: "Returns whether any of the unit's flagged seats currently has an occupant."
// evidence: types/tags.h Unit.seats (0x2e4/0x2e8), UnitSeat (0x11c, flags dword bit 0x4).
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index); // 0x56cc10

uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    int32_t count = (int32_t)unit_tag->seats.count;
    if (count < 1) {
        return 0;
    }
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
    for (int16_t i = 0; i < count; i++) {
        if ((seats[i].flags & 4) != 0) {
            if (unit_is_seat_occupied((int32_t)unit_index, i) == 1) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56cc80):

undefined4 FUN_0056cc80(void)

{
  int iVar1;
  int iVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;
  short sVar5;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)(iVar1 + 0x2e4);
  sVar5 = 0;
  if (iVar2 < 1) {
    return 0;
  }
  iVar1 = *(int *)(iVar1 + 0x2e8);
  iVar4 = 0;
  do {
    if ((*(byte *)(iVar4 * 0x11c + iVar1) & 4) != 0) {
      cVar3 = FUN_0056cc10();
      if (cVar3 == '\x01') {
        return 1;
      }
    }
    sVar5 = sVar5 + 1;
    iVar4 = (int)sVar5;
  } while (iVar4 < iVar2);
  return 0;
}
#endif
