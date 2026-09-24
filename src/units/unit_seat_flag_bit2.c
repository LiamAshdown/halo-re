// unit_seat_flag_bit2  (Ghidra: FUN_0056cd10)
// address 0x56cd10, size 84 bytes, name confidence 0.3, rewrite confidence 0.5
// functions.md: "Returns a specific flag bit (bit 2) from the definition of the unit's seat at
// index in_CX."
// evidence: types/tags.h Unit.seats (0x2e4/0x2e8), UnitSeat (0x11c, flags dword; bit 2 =
//   "driver" per UnitSeatFlags: invisible, locked, driver, gunner, ...).
// blam-cc: in_EAX -> unit_index, in_CX -> seat_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

uint8_t unit_seat_flag_bit2(uint32_t unit_index, int16_t seat_index) // blam-cc: in_EAX, in_CX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    if ((-1 < seat_index) && (seat_index < (int32_t)unit_tag->seats.count)) {
        UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
        return (seats[seat_index].flags >> 2) & 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56cd10):

uint FUN_0056cd10(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  short in_CX;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar2 = DAT_0087bc14 & 0xffffff00;
  if (-1 < in_CX) {
    if ((int)in_CX < *(int *)(iVar1 + 0x2e4)) {
      uVar2 = *(uint *)(in_CX * 0x11c + *(int *)(iVar1 + 0x2e8)) >> 2 & 0xffffff01;
    }
  }
  return uVar2;
}
#endif
