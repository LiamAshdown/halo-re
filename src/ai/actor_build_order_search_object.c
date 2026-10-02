// actor_build_order_search_object  (Ghidra: FUN_00408920; really: build a mode 9 order to board the best seat of
//   a vehicle)
// address 0x408920, size 267 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// REWRITTEN from objdump 0x408920..0x408a2a (the draft lost the EBX vehicle and every register argument). EBX:
//   the vehicle; stack (actor, alert radius a, alert radius b, order). The 0x4c-byte mode 9 order is zeroed and
//   gets the radii at +0x20 / +0x24 (actor +0xbc / +0xc0 once set). For an actor not driving anything, not
//   swarming and not already boarding (mode 9), with the vehicle in its alert range (0x408f30), the best seat
//   (0x409070) is taken: +0x00 vehicle, +0x04 seat, +0x06 1 (scripted), then as 0x408a30 does the seat must be
//   usable (0x565150), the approach point (+0x30, surface +0x48) found (0x4095c0) and made the destination.
// blam-cc: EBX -> vehicle_index, stack -> (actor_index, radius_a, radius_b, order)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;  // 0x00880360

extern uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index); // 0x408f30, stack, EAX, ECX
extern int16_t actor_find_best_search_node(datum_index actor_index, datum_index vehicle_index, real_point3d *out_entry,
                                           real_vector3d *out_direction, real_point3d *out_hint); // 0x409070
extern uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index); // 0x565150, EAX, ECX, DX
extern uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry,
    real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index); // 0x4095c0, EAX, ECX, EDX, stack
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, EAX, stack

uint8_t actor_build_order_search_object(uint32_t vehicle_index, uint32_t actor_index, float radius_a, float radius_b,
                                        uint8_t *order)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    real_point3d entry;
    real_vector3d direction;
    real_point3d hint;
    int16_t seat;

    memset(order, 0, 0x4c);
    *(float *)(order + 0x20) = radius_a;
    *(float *)(order + 0x24) = radius_b;
    if (((actor *)act)->active_unit_index != k_datum_index_none || act[0x6] != 0 || ((actor *)act)->mode == 9) {
        return 0;
    }
    if (!actor_is_within_alert_range(0, radius_a, radius_b, 0, 0, actor_index, vehicle_index)) {
        return 0;
    }
    *(datum_index *)(order + 0x0) = vehicle_index;
    seat = actor_find_best_search_node(actor_index, vehicle_index, &entry, &direction, &hint);
    *(int16_t *)(order + 0x4) = seat;
    if (seat == -1) {
        return 0;
    }
    order[0x6] = 1;
    if (!unit_seat_index_is_valid(((actor *)act)->unit_index, vehicle_index, seat)) {
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
Original Ghidra decompilation (0x408920):

undefined1
actor_build_order_search_object
          (uint param_1,undefined4 param_2,undefined4 param_3,undefined4 *param_4)

{
  undefined1 uVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  undefined4 unaff_EBX;
  int iVar5;
  undefined4 *puVar6;
  undefined1 local_24 [36];

  iVar5 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar6 = param_4;
  for (iVar4 = 0x13; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  param_4[8] = param_2;
  param_4[9] = param_3;
  uVar1 = 0;
  if (((*(int *)(iVar5 + 0x158) == -1) && (*(char *)(iVar5 + 6) == '\0')) &&
     (*(short *)(iVar5 + 0x6c) != 9)) {
    cVar2 = actor_is_within_alert_range(0,param_2,param_3,0,0);
    if (cVar2 != '\0') {
      *param_4 = unaff_EBX;
      sVar3 = FUN_00409070(param_1);
      *(short *)(param_4 + 1) = sVar3;
      if (sVar3 != -1) {
        *(undefined1 *)((int)param_4 + 6) = 1;
        cVar2 = FUN_00565150();
        if (cVar2 != '\0') {
          cVar2 = FUN_004095c0(local_24,0,param_4 + 0xc,param_4 + 0x12);
          if (cVar2 != '\0') {
            cVar2 = actor_movement_set_destination_point(param_1,param_4[0x12]);
            if (cVar2 != '\0') {
              uVar1 = 1;
            }
          }
        }
      }
    }
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
