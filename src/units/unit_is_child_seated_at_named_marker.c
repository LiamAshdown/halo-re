// unit_is_child_seated_at_named_marker  (Ghidra: FUN_0056b520)
// address 0x56b520, size 196 bytes, name confidence 0.35, rewrite confidence 0.4
// functions.md: "Returns whether a given child object is seated at the named marker on this
// unit."
// evidence: types/tags.h Unit.seats (0x2e4/0x2e8), UnitSeat (0x11c, label at +4);
//   types/objects.h object.parent_object (0x11c), types/units.h unit_data.vehicle_seat_index
//   (0x2f0).
// blam-cc: param_1 -> unit_index, param_2 -> seat_label, unaff_EBX -> child_object_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b

uint8_t unit_is_child_seated_at_named_marker(uint32_t unit_index, char *seat_label, uint32_t child_object_index)
{
    if ((unit_index == 0xffffffff) || (child_object_index == 0xffffffff)) {
        return 0;
    }
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    if ((int32_t)unit_tag->seats.count < 1) {
        return 0;
    }
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
    object *child = ((object_header *)object_data->data)[child_object_index & 0xffff].data;

    for (int16_t seat_index = 0; seat_index < (int32_t)unit_tag->seats.count; seat_index++) {
        if ((__stricmp(seat_label, seats[seat_index].label.string) == 0) &&
            (child->parent_object == unit_index) &&
            (((unit_data *)((uint8_t *)child + k_unit_data_offset))->vehicle_seat_index == seat_index)) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56b520):

undefined4 FUN_0056b520(uint param_1,char *param_2)

{
  int iVar1;
  int iVar2;
  uint unaff_EBX;
  short sVar3;

  if ((param_1 != 0xffffffff) && (unaff_EBX != 0xffffffff)) {
    iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) &
                     0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar3 = 0;
    if (*(int *)(iVar1 + 0x2e4) < 1) {
      return 0;
    }
    iVar2 = 0;
    while( true ) {
      iVar2 = __stricmp(param_2,(char *)(iVar2 * 0x11c + *(int *)(iVar1 + 0x2e8) + 4));
      if (((iVar2 == 0) &&
          (iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc),
          *(uint *)(iVar2 + 0x11c) == param_1)) && (*(short *)(iVar2 + 0x2f0) == sVar3)) break;
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
      if (*(int *)(iVar1 + 0x2e4) <= iVar2) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}
#endif
