// unit_get_seat_or_state_name  (Ghidra: FUN_0056c2f0)
// address 0x56c2f0, size 115 bytes, name confidence 0.4, rewrite confidence 0.6
// functions.md: "Returns the name of the seat/marker the unit currently occupies, or a default
// state-name string if it has no parent."
// evidence: types/objects.h object.parent_object (0x11c); types/units.h unit_data.
//   vehicle_seat_index (0x2f0), .base_animation_state (0x2a7); types/tags.h Unit.seats
//   (0x2e4/0x2e8), UnitSeat (0x11c, label at +4); unit_base_animation_state_names[6]
//   (0x0069fde4, established in src/units/unit_set_or_test_seat_and_weapon_label.c).
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;                  // 0x008603b0
extern tag_instance *tag_instances;               // 0x0087bc14
extern char *unit_base_animation_state_names[6]; // 0x0069fde4

char *unit_get_seat_or_state_name(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if ((unit_obj->parent_object != k_datum_index_none) && (unit->vehicle_seat_index != -1)) {
        object *parent = ((object_header *)object_data->data)[unit_obj->parent_object & 0xffff].data;
        Unit *parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
        UnitSeat *seat = (UnitSeat *)parent_tag->seats.pointer + unit->vehicle_seat_index;
        return seat->label.string;
    }
    return unit_base_animation_state_names[unit->base_animation_state];
}

#if 0
Original Ghidra decompilation (0x56c2f0):

undefined * FUN_0056c2f0(void)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((*(uint *)(iVar1 + 0x11c) != 0xffffffff) && (*(short *)(iVar1 + 0x2f0) != -1)) {
    return (undefined *)
           (*(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc) & 0xffff) *
                              0x20 + 0x14 + DAT_0087bc14) + 0x2e8) + 4 +
           *(short *)(iVar1 + 0x2f0) * 0x11c);
  }
  return (&PTR_DAT_0069fde4)[*(char *)(iVar1 + 0x2a7)];
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
