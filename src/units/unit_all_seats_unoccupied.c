// unit_all_seats_unoccupied  (Ghidra: unit_all_seats_unoccupied)
// address 0x566910, size 95 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.95 (VERIFIED against objdump)
// evidence: types/tags.h Unit.seats (TagReflexive count at 0x2e4).
// register convention: unit index in EAX.
//   // blam-cc: in_EAX -> unit_index
// UNSURE: unit_is_seat_occupied is called with zero visible arguments per seat, but its name and the
//   context ("Returns whether none of a unit-type's seats currently report the tracked
//   occupancy status") strongly imply it is a per-seat occupancy test; modelled as taking the
//   unit index and the seat's loop counter explicitly. The return value's upper 24 bits are
//   undefined register garbage in the original (CONCAT31), matching the house simplification
//   used throughout this codebase (see src/memory/bit_stream_write_bit.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index); // 0x56cc10, UNSURE signature

uint8_t unit_all_seats_unoccupied(uint32_t unit_index) // blam-cc: in_EAX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    int32_t count = (int32_t)unit_tag->seats.count;

    for (int16_t i = 0; i < count; i++) {
        if (unit_is_seat_occupied(unit_index, i) == 1) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x566910):

uint FUN_00566910(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  int iVar3;
  short sVar4;

  iVar3 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar1 = *(int *)(iVar3 + 0x2e4);
  sVar4 = 0;
  if (0 < iVar1) {
    do {
      uVar2 = FUN_0056cc10();
      if ((char)uVar2 == '\x01') {
        return uVar2 & 0xffffff00;
      }
      sVar4 = sVar4 + 1;
      iVar3 = (int)sVar4;
    } while (iVar3 < iVar1);
  }
  return CONCAT31((int3)((uint)iVar3 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
