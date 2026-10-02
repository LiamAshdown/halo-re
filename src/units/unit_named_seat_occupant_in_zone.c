// unit_named_seat_occupant_in_zone  (Ghidra: FUN_0056b380)
// address 0x56b380, size 407 bytes, name confidence 0.35, rewrite confidence 0.3
// functions.md: "Looks up a named seat on the unit and reports whether the object currently
// occupying it also appears in the zone/list referenced by param_3."
// evidence: types/tags.h Unit.seats (0x2e4/0x2e8), UnitSeat (0x11c, label at +4);
//   types/objects.h object_iterator; the same zone-list walk as unit_mark_zone_occupants_flag.c.
// blam-cc: param_1 -> unit_index, param_2 -> seat_label, param_3 -> zone_list_index.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;             // 0x008603b0
extern tag_instance *tag_instances;         // 0x0087bc14
extern data_array *object_list_header_data; // 0x0087a464
extern data_array *object_list_reference_data;  // 0x0087a468

extern object * object_iterator_next(object_iterator *iterator); // 0x4f6f20

uint8_t unit_named_seat_occupant_in_zone(uint32_t unit_index, char *seat_label, uint32_t zone_list_index)
{
    if (unit_index == 0xffffffff) {
        return 0;
    }
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    if ((int32_t)unit_tag->seats.count < 1) {
        return 0;
    }
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;

    for (int16_t seat_index = 0; seat_index < (int32_t)unit_tag->seats.count; seat_index++) {
        if (_stricmp(seat_label, seats[seat_index].label.string) != 0) {
            continue;
        }

        object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
        object *occupant = object_iterator_next(&iter);
        uint32_t occupant_index = 0xffffffff;
        while (occupant != (object *)0) {
            if ((occupant->parent_object == unit_index) &&
                (((unit_data *)((uint8_t *)occupant + k_unit_data_offset))->vehicle_seat_index == seat_index)) {
                occupant_index = iter.handle;
                break;
            }
            occupant = object_iterator_next(&iter);
        }
        if (occupant_index == 0xffffffff) {
            continue; // no occupant found for this seat; try the next matching-label seat
        }

        uint32_t zone_object;  // iVar2
        uint32_t next_link;    // uVar5
        if (zone_list_index == 0xffffffff) {
            zone_object = 0xffffffff;
            next_link = 0xffffffff;
        } else {
            uint32_t first_link = *(uint32_t *)((uint8_t *)object_list_header_data->data + (zone_list_index & 0xffff) * 0xc + 8);
            if (first_link == 0xffffffff) {
                zone_object = 0xffffffff;
                next_link = 0xffffffff;
            } else {
                uint32_t link_slot = first_link & 0xffff;
                uint8_t *node = (uint8_t *)object_list_reference_data->data + link_slot * 0xc;
                next_link = *(uint32_t *)(node + 8);
                zone_object = *(uint32_t *)(node + 4);
            }
        }

        while (zone_object != occupant_index) {
            if (zone_object == 0xffffffff) {
                break;
            }
            if (next_link == 0xffffffff) {
                zone_object = 0xffffffff;
                next_link = 0xffffffff;
            } else {
                uint8_t *node = (uint8_t *)object_list_reference_data->data + (next_link & 0xffff) * 0xc;
                next_link = *(uint32_t *)(node + 8);
                zone_object = *(uint32_t *)(node + 4);
            }
        }
        return zone_object == occupant_index;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x56b380):

uint FUN_0056b380(uint param_1,char *param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  short sVar4;
  uint uVar5;
  undefined1 local_11;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  int local_8;
  undefined4 local_4;

  local_11 = 0;
  if (param_1 == 0xffffffff) {
    return 0xffffff00;
  }
  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar4 = 0;
  if ((int)*(uint *)(iVar1 + 0x2e4) < 1) {
    return *(uint *)(iVar1 + 0x2e4) & 0xffffff00;
  }
  iVar2 = 0;
  uVar5 = param_1;
LAB_0056b3e0:
  iVar2 = __stricmp(param_2,(char *)(iVar2 * 0x11c + *(int *)(iVar1 + 0x2e8) + 4));
  if (iVar2 == 0) {
    local_4 = 0x86868686;
    local_10 = 3;
    local_c = 0;
    local_a = 0;
    local_8 = -1;
    iVar2 = object_iterator_next(&local_10);
    while (iVar2 != 0) {
      if ((*(uint *)(iVar2 + 0x11c) == param_1) && (*(short *)(iVar2 + 0x2f0) == sVar4)) {
        iVar2 = -1;
        if (param_3 != 0xffffffff) {
          uVar3 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (param_3 & 0xffff) * 0xc);
          if (uVar3 == 0xffffffff) {
            iVar2 = -1;
            uVar5 = 0xffffffff;
          }
          else {
            uVar3 = uVar3 & 0xffff;
            uVar5 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar3 * 0xc);
            iVar2 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar3 * 0xc + 4);
          }
        }
        if (iVar2 == -1) goto LAB_0056b4e9;
        goto LAB_0056b4c0;
      }
      iVar2 = object_iterator_next(&local_10);
    }
  }
  goto LAB_0056b4f2;
LAB_0056b4c0:
  do {
    if (local_8 == iVar2) goto LAB_0056b4ed;
    if (uVar5 == 0xffffffff) {
      iVar2 = -1;
    }
    else {
      iVar2 = *(int *)(DAT_0087a468 + 0x34) + (uVar5 & 0xffff) * 0xc;
      uVar5 = *(uint *)(iVar2 + 8);
      iVar2 = *(int *)(iVar2 + 4);
    }
  } while (iVar2 != -1);
LAB_0056b4e9:
  if (local_8 == iVar2) {
LAB_0056b4ed:
    local_11 = 1;
  }
LAB_0056b4f2:
  sVar4 = sVar4 + 1;
  iVar2 = (int)sVar4;
  if (*(int *)(iVar1 + 0x2e4) <= iVar2) {
    return CONCAT31((int3)(char)((ushort)sVar4 >> 8),local_11);
  }
  goto LAB_0056b3e0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
