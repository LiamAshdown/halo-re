// unit_is_seat_occupied  (Ghidra: FUN_0056cc10)
// address 0x56cc10, size 104 bytes, name confidence 0.4, rewrite confidence 0.5
// functions.md: "Returns whether any object is currently seated at the given parent/seat-index
// pair."
// evidence: types/objects.h object.parent_object (0x11c); types/units.h unit_data.
//   vehicle_seat_index (0x2f0); types/objects.h object_iterator.
// blam-cc: unaff_EDI -> parent_index, unaff_SI -> seat_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object * object_iterator_next(object_iterator *iterator); // 0x4f6f20

uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index) // blam-cc: unaff_EDI, unaff_SI
{
    object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
    object *obj = object_iterator_next(&iter);
    while (obj != (object *)0) {
        if ((obj->parent_object == (uint32_t)parent_index) &&
            (((unit_data *)((uint8_t *)obj + k_unit_data_offset))->vehicle_seat_index == seat_index)) {
            return 1;
        }
        obj = object_iterator_next(&iter);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56cc10):

undefined4 FUN_0056cc10(void)

{
  int iVar1;
  short unaff_SI;
  int unaff_EDI;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 3;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(&local_10);
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if ((*(int *)(iVar1 + 0x11c) == unaff_EDI) && (*(short *)(iVar1 + 0x2f0) == unaff_SI)) break;
    iVar1 = object_iterator_next(&local_10);
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
