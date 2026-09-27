// actor_build_order_investigate_encounter_point  (Ghidra: FUN_00408a30; really: build the mode 9 "board vehicle
//   seat" order)
// address 0x408a30, size 287 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// REWRITTEN from objdump 0x408a30..0x408b4e (the draft lost the EBX vehicle and every register argument). EBX:
//   the vehicle; stack (actor, seat, order). The 0x4c-byte order (actor_set_mode's mode 9 data, copied to
//   actor +0x9c) is zeroed, then filled only for an actor not driving anything (+0x158) and not swarming (+0x6),
//   a vehicle that is upright (up.k >= 0.5) and alive (+0x106 bit 2 clear), and a seat the actor's unit may use
//   (0x565150): +0x00 vehicle, +0x04 seat, +0x06 0, then +0x30 the approach point and +0x48 its surface from
//   0x4095c0, and the movement destination is set to it. Returns 1 when all of that succeeded.
// blam-cc: EBX -> vehicle_index, stack -> (actor_index, seat_index, order)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include <string.h>

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index); // 0x565150, EAX, ECX, DX
extern uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score,
    uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front); // 0x4091d0
extern uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry,
    real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index); // 0x4095c0, EAX, ECX, EDX, stack
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, EAX, stack

uint8_t actor_build_order_investigate_encounter_point(uint32_t vehicle_index, uint32_t actor_index, int16_t seat_index,
                                                      uint8_t *order)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *vehicle;
    real_point3d entry;
    real_vector3d direction;
    real_point3d hint;

    memset(order, 0, 0x4c);
    if (*(datum_index *)(act + 0x158) != k_datum_index_none || act[0x6] != 0) {
        return 0;
    }
    vehicle = (uint8_t *)((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    if (*(float *)(vehicle + 0x88) < 0.5f || (vehicle[0x106] & 4) != 0) {
        return 0;
    }
    *(datum_index *)(order + 0x0) = vehicle_index;
    *(int16_t *)(order + 0x4) = seat_index;
    order[0x6] = 0;
    if (!unit_seat_index_is_valid(*(datum_index *)(act + 0x18), vehicle_index, seat_index)) {
        return 0;
    }
    if (!actor_evaluate_search_node(actor_index, vehicle_index, seat_index, &entry, &direction, &hint, 0, 0, 0, 0)) {
        return 0;
    }
    if (!actor_avoid_obstacle_and_project(actor_index, vehicle_index, &entry, &hint, 0, (real_point3d *)(order + 0x30),
                                          (int32_t *)(order + 0x48))) {
        return 0;
    }
    if (!actor_movement_set_destination_point((real_point3d *)(order + 0x30), actor_index, *(int32_t *)(order + 0x48),
                                              vehicle_index)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x408a30):

undefined1
actor_build_order_investigate_encounter_point(uint param_1,undefined2 param_2,uint *param_3)

{
  char cVar1;
  int iVar2;
  uint unaff_EBX;
  int iVar3;
  uint *puVar4;
  undefined1 local_25;
  undefined1 local_24 [36];

  iVar3 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar4 = param_3;
  for (iVar2 = 0x13; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  local_25 = 0;
  if ((*(int *)(iVar3 + 0x158) == -1) && (*(char *)(iVar3 + 6) == '\0')) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
    if ((0.5 <= *(float *)(iVar2 + 0x88)) && ((~(*(byte *)(iVar2 + 0x106) >> 2) & 1) != 0)) {
      *param_3 = unaff_EBX;
      *(undefined2 *)(param_3 + 1) = param_2;
      *(undefined1 *)((int)param_3 + 6) = 0;
      cVar1 = FUN_00565150();
      if (cVar1 != '\0') {
        cVar1 = FUN_004091d0(param_1);
        if (cVar1 != '\0') {
          cVar1 = FUN_004095c0(local_24,0,param_3 + 0xc,param_3 + 0x12);
          if (cVar1 != '\0') {
            cVar1 = actor_movement_set_destination_point(param_1,param_3[0x12]);
            if (cVar1 != '\0') {
              local_25 = 1;
            }
          }
        }
      }
    }
  }
  return local_25;
}
#endif
