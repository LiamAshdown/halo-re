// unit_find_best_seat_to_enter  (Ghidra: unit_find_best_seat_to_enter, already named)
// address 0x566560, size 722 bytes
// name confidence: 0.6 (already carries this name)   rewrite confidence: 0.25
// evidence: types/objects.h object.vitality_flags (0x106, _object_health_frozen_bit),
//   object.bounding_center (0xa0); types/units.h unit_data.flags (0x204), .actor_index (0x1f4),
//   .driver_unit_index (0x324); types/tags.h Unit.seats (TagReflexive at 0x2e4/0x2e8, UnitSeat
//   stride 0x11c, UnitSeatFlags bit 0x200 not_valid_without_driver, label TagString at +0x4,
//   third_person_camera bit 0x10 -- see UNSURE); unit_find_weapon_marker_transform (0x5640a0),
//   unit_set_or_test_seat_and_weapon_label (0x5651e0), unit_seat_is_occupied_by_other (0x566840).
// register convention: entering-unit index in EAX, vehicle index in ECX, out-seat pointer on
//   the stack.
//   // blam-cc: param_1 (EAX) -> unit_index, param_2 (ECX) -> vehicle_index, param_3 -> out_seat
// UNSURE: unit_seat_is_occupied_by_other is called here with only 2 of its 4 established
//   parameters visible (seat_index and an out-occupant pointer); self_index/vehicle_index are
//   modelled as this function's own unit_index/vehicle_index. local_2c is declared as a float
//   by Ghidra but is really reused as a datum_index sentinel ("-NAN" standing in for the bit
//   pattern of -1); modelled here as a uint32_t. actor_check_vehicle_target_available and object.flags bit 0x10000 are
//   outside this batch's evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction, see src/math/quaternion_normalize.c
extern uint8_t actor_check_vehicle_target_available(uint32_t flag); // 0x42b810, UNSURE signature
extern uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint); // 0x5640a0, EAX unit, stack
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label,
                                                     char *weapon_label, uint8_t test_only); // 0x5651e0,
// unit_index in EAX; this matches the definition in unit_set_or_test_seat_and_weapon_label.c.
// The phase-4 review pass corrected the arity (Ghidra binds only the stack arguments at these
// call sites) and the return type (the callee returns a byte, tested in AL).
extern uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index,
                                               uint32_t *out_occupant_index); // 0x566840

uint16_t unit_find_best_seat_to_enter(uint32_t unit_index, uint32_t vehicle_index, int16_t *out_seat) // blam-cc: see file header
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    object *vehicle_obj = ((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    Unit *vehicle_tag = (Unit *)tag_instances[vehicle_obj->definition_tag & 0xffff].data;

    unit_data *vehicle_unit = (unit_data *)((uint8_t *)vehicle_obj + k_unit_data_offset);
    uint16_t best_rank = 0;
    int16_t best_seat = -1;

    if ((vehicle_obj->vitality_flags & _object_health_frozen_bit) != 0) {
        *out_seat = -1;
        return 0;
    }
    if ((*(uint32_t *)((uint8_t *)vehicle_obj + 0x204) & 0x10000) != 0) { // UNSURE: raw unit_data.flags bit
        *out_seat = -1;
        return 0;
    }
    if ((int32_t)vehicle_tag->seats.count < 1) {
        *out_seat = -1;
        return 0;
    }

    float best_distance = 3.4028235e+38f;
    uint8_t best_is_third_person = 0;

    for (int16_t seat_i = 0; seat_i < (int32_t)vehicle_tag->seats.count; seat_i++) {
        UnitSeat *seat = (UnitSeat *)((uint8_t *)vehicle_tag->seats.pointer + seat_i * 0x11c);

        real_vector3d marker_a, marker_b;
        if (unit_find_weapon_marker_transform(unit_index, vehicle_index, seat_i, (real_point3d *)&marker_a,
                (real_point3d *)&marker_b, 0) != 0) { // 0x566622: EAX = the unit
            float dx = unit_obj->bounding_center.x - marker_a.i;
            float dy = unit_obj->bounding_center.y - marker_a.j;
            float dz = unit_obj->bounding_center.z - marker_a.k;
            float dist_a = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

            dx = unit_obj->bounding_center.x - marker_b.i;
            dy = unit_obj->bounding_center.y - marker_b.j;
            dz = unit_obj->bounding_center.z - marker_b.k;
            float dist_b = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

            float dist = (dist_b < dist_a) ? dist_b : dist_a;

            if (dist < 1.0f &&
                ((seat->flags & 0x200) == 0 || vehicle_unit->driver_unit_index != (datum_index)-1) &&
                seat->label.string[0] != '\0' &&
                unit_set_or_test_seat_and_weapon_label(unit_index, seat->label.string, 0, 0) != 0) {

                uint32_t occupant = (uint32_t)-1;
                uint16_t rank;
                if (unit_seat_is_occupied_by_other(unit_index, seat_i, vehicle_index, &occupant) == 0) {
                    if (occupant == (uint32_t)-1) {
                        continue;
                    }
                    object *occupant_obj = ((object_header *)object_data->data)[occupant & 0xffff].data;
                    unit_data *occupant_unit = (unit_data *)((uint8_t *)occupant_obj + k_unit_data_offset);
                    if (occupant_unit->actor_index == (datum_index)-1 || actor_check_vehicle_target_available(0) == 0) {
                        continue;
                    }
                    rank = 1;
                } else {
                    rank = 2;
                }

                float weight = 1.0f;
                if (best_is_third_person && ((seat->flags >> 2 & 1) == 0)) {
                    weight = 1.5f;
                }

                if (best_seat == -1 || best_rank < rank || dist * weight < best_distance) {
                    best_is_third_person = (uint8_t)(seat->flags >> 2) & 1;
                    best_distance = dist;
                    best_rank = rank;
                    best_seat = seat_i;
                }
            }
        }
    }

    *out_seat = best_seat;
    return best_rank;
}

#if 0
Original Ghidra decompilation (0x566560):

ushort unit_find_best_seat_to_enter(uint param_1,uint param_2,undefined2 *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  byte bVar4;
  char cVar5;
  int iVar6;
  uint *puVar7;
  ushort uVar8;
  ushort uVar9;
  int iVar10;
  float local_34;
  int local_30;
  float local_2c;
  float local_28;
  int local_24;
  int local_20;
  uint *local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_20 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_1c = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  local_24 = *(int *)((*local_1c & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar9 = 0;
  local_30 = -1;
  if ((*(byte *)((int)local_1c + 0x106) & 4) != 0) {
    *param_3 = 0xffff;
    return 0;
  }
  if ((local_1c[0x81] & 0x10000) != 0) {
    *param_3 = 0xffff;
    return 0;
  }
  iVar10 = 0;
  local_28 = 3.4028235e+38;
  bVar4 = 0;
  if (*(int *)(local_24 + 0x2e4) < 1) {
    *param_3 = 0xffff;
    return 0;
  }
  iVar6 = 0;
  do {
    puVar7 = (uint *)(iVar6 * 0x11c + *(int *)(local_24 + 0x2e8));
    cVar5 = FUN_005640a0(param_2,iVar10,&local_18,&local_c,0);
    if (cVar5 != '\0') {
      fVar1 = *(float *)(local_20 + 0xa0) - local_18;
      fVar3 = *(float *)(local_20 + 0xa4) - local_14;
      fVar2 = *(float *)(local_20 + 0xa8) - local_10;
      local_34 = SQRT(fVar1 * fVar1 + fVar3 * fVar3 + fVar2 * fVar2);
      fVar1 = *(float *)(local_20 + 0xa0) - local_c;
      fVar3 = *(float *)(local_20 + 0xa4) - local_8;
      fVar2 = *(float *)(local_20 + 0xa8) - local_4;
      local_2c = SQRT(fVar1 * fVar1 + fVar3 * fVar3 + fVar2 * fVar2);
      if (local_2c < local_34) {
        local_34 = local_2c;
      }
      if (((local_34 < 1.0) &&
          ((((*puVar7 & 0x200) == 0 || (local_1c[0xc9] != 0xffffffff)) && ((char)puVar7[1] != '\0'))
          )) && (cVar5 = unit_set_or_test_seat_and_weapon_label(puVar7 + 1,0,0), cVar5 != '\0')) {
        local_2c = -NAN;
        cVar5 = FUN_00566840(iVar10,&local_2c);
        if (cVar5 == '\0') {
          if (((local_2c == -NAN) ||
              (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)local_2c & 0xffff) * 0xc
                                ) + 500) == -1)) || (cVar5 = FUN_0042b810(0), cVar5 == '\0'))
          goto LAB_005667d6;
          uVar8 = 1;
        }
        else {
          uVar8 = 2;
        }
        fVar1 = 1.0;
        if ((bVar4 != 0) && ((*puVar7 >> 2 & 1) == 0)) {
          fVar1 = 1.5;
        }
        if ((((short)local_30 == -1) || (uVar9 < uVar8)) || (local_34 * fVar1 < local_28)) {
          bVar4 = (byte)(*puVar7 >> 2) & 1;
          local_28 = local_34;
          uVar9 = uVar8;
          local_30 = iVar10;
        }
      }
    }
LAB_005667d6:
    iVar10 = iVar10 + 1;
    iVar6 = (int)(short)iVar10;
    if (*(int *)(local_24 + 0x2e4) <= iVar6) {
      *param_3 = (short)local_30;
      return uVar9;
    }
  } while( true );
}
#endif
