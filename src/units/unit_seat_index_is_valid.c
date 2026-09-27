// unit_seat_index_is_valid  (Ghidra: unit_seat_index_is_valid)
// address 0x565150, size 137 bytes
// name confidence: 0.4 (phase2 candidate)   rewrite confidence: 0.9 (VERIFIED against objdump; label test object FIXED)
// evidence: types/tags.h Unit.seats (TagReflexive at 0x2e4/0x2e8, UnitSeat stride 0x11c,
//   UnitSeat.label TagString at +0x4); types/objects.h object.type (0xb4);
//   unit_set_or_test_seat_and_weapon_label (0x5651e0).
// register convention: a second object index in EAX (only its type is read), unit index in
//   ECX, seat index in DX.
//   // blam-cc: in_EAX -> other_object_index, in_ECX -> unit_index, in_DX -> seat_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label,
                                                     char *weapon_label, uint8_t test_only); // 0x5651e0,
// unit_index in EAX; this matches the definition in unit_set_or_test_seat_and_weapon_label.c.
// The phase-4 review pass corrected the arity (Ghidra binds only the stack arguments at these
// call sites) and the return type (the callee returns a byte, tested in AL).

uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index) // blam-cc: see file header
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    if (-1 < seat_index && seat_index < (int32_t)unit_tag->seats.count) {
        object *other_obj = ((object_header *)object_data->data)[other_object_index & 0xffff].data;
        if (other_obj->type == 1) {
            return 1;
        }
        UnitSeat *seat = (UnitSeat *)((uint8_t *)unit_tag->seats.pointer + seat_index * 0x11c);
        // FIXED (0x5651c5): EAX is still the other object (the one entering), not the vehicle
        if (unit_set_or_test_seat_and_weapon_label(other_object_index, seat->label.string, 0, 0) != 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x565150):

undefined4 FUN_00565150(void)

{
  int iVar1;
  char cVar2;
  uint in_EAX;
  uint in_ECX;
  short in_DX;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((-1 < in_DX) && ((int)in_DX < *(int *)(iVar1 + 0x2e4))) {
    if (*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0xb4) ==
        1) {
      return 1;
    }
    cVar2 = unit_set_or_test_seat_and_weapon_label(in_DX * 0x11c + *(int *)(iVar1 + 0x2e8) + 4,0,0);
    if (cVar2 != '\0') {
      return 1;
    }
  }
  return 0;
}
#endif
