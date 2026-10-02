// unit_recompute_seat_occupants  (Ghidra: FUN_0056ce30)
// address 0x56ce30, size 223 bytes, name confidence 0.4, rewrite confidence 0.9 (verified against objdump 0x56ce30..0x56cf0e)
// functions.md: "Recomputes which child object occupies the unit's primary and secondary
// tracked seats (fields 0xc9/0xca) by scanning its list of attached children."
// evidence: types/objects.h object.first_child_object (0x118), .next_object (0x114), .type
//   (0xb4); types/units.h unit_data.flags (0x204, bit 0 = _unit_flag_unattended),
//   .driver_unit_index (0x324), .gunner_unit_index (0x328), .vehicle_seat_index (0x2f0);
//   types/tags.h Unit.seats (0x2e4/0x2e8), UnitSeat.flags (bit 2 = driver, bit 3 = gunner).
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

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

void unit_recompute_seat_occupants(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;

    datum_index child_index = unit_obj->first_child_object;
    while (child_index != k_datum_index_none) {
        object *child = ((object_header *)object_data->data)[child_index & 0xffff].data;

        if (((_object_mask_unit & (1 << (child->type & 0x1f))) != 0) &&
            (((unit_data *)((uint8_t *)child + k_unit_data_offset))->vehicle_seat_index != -1)) {
            int16_t seat_index = ((unit_data *)((uint8_t *)child + k_unit_data_offset))->vehicle_seat_index;
            uint32_t seat_flags = seats[seat_index].flags;

            if (((seat_flags & 4) == 0) || ((unit->flags & 1) != 0) || (unit->driver_unit_index != k_datum_index_none)) {
                if ((seat_flags & 8) != 0) {
                    if (unit->gunner_unit_index != k_datum_index_none) {
                        if (unit->gunner_unit_index == unit->driver_unit_index) {
                            unit->gunner_unit_index = child_index;
                        }
                    } else {
                        unit->gunner_unit_index = child_index;
                    }
                }
            } else {
                unit->driver_unit_index = child_index;
                if ((seat_flags & 8) != 0) {
                    if (unit->gunner_unit_index == k_datum_index_none) {
                        unit->gunner_unit_index = child_index;
                    }
                }
            }
        }
        child_index = child->next_object;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56ce30):

void FUN_0056ce30(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint in_EAX;
  uint *puVar7;
  bool bVar8;

  iVar6 = DAT_008603b0;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = puVar1[0x46];
  iVar3 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  do {
    if (uVar2 == 0xffffffff) {
      return;
    }
    iVar4 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    if (((1 << (*(byte *)(iVar4 + 0xb4) & 0x1f) & 3U) != 0) && (*(short *)(iVar4 + 0x2f0) != -1)) {
      puVar7 = (uint *)(*(short *)(iVar4 + 0x2f0) * 0x11c + *(int *)(iVar3 + 0x2e8));
      uVar5 = *puVar7;
      if (((uVar5 & 4) == 0) || (((puVar1[0x81] & 1) != 0 || (puVar1[0xc9] != 0xffffffff)))) {
        if ((uVar5 & 8) != 0) {
          if (puVar1[0xca] != 0xffffffff) {
            bVar8 = puVar1[0xca] == puVar1[0xc9];
            goto LAB_0056cef3;
          }
LAB_0056cef5:
          puVar1[0xca] = uVar2;
        }
      }
      else {
        puVar1[0xc9] = uVar2;
        if ((*puVar7 & 8) != 0) {
          bVar8 = puVar1[0xca] == 0xffffffff;
LAB_0056cef3:
          if (bVar8) goto LAB_0056cef5;
        }
      }
    }
    uVar2 = *(uint *)(iVar4 + 0x114);
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
