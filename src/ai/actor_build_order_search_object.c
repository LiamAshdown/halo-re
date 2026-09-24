// actor_build_order_search_object  (Ghidra: actor_build_order_search_object, already named)
// address 0x408920, size 267 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: types/ai.h actor.active_unit_index (0x158)/swarm (0x06)/mode (0x6c, compared
//   against _actor_mode_vocalize); phase-4 summary "builds an order sending the actor to
//   investigate a specific object, chaining range, waypoint, visibility, and reachability
//   checks".
// register convention: actor index in EAX, an unresolved value in EBX, a target point (x/y)
//   in the two recognized stack parameters, and the order/output record in the third stack
//   parameter.
//   // blam-cc: EAX -> actor_index, EBX -> unknown_target, stack -> target_x/target_y/order
// TYPES-GAP: `order`'s tail (offsets 0x20/0x24/0x30..0x48) is not defined anywhere in
//   types/ai.h; used here as raw offsets, matching the other order-family builders' varying
//   tails in this session.
// UNSURE: `unknown_target` (unaff_EBX) is written into order[0] with no visible definition
//   in this function; its real source is whatever register-allocation decision the caller
//   made, not reproducible here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index); // 0x408f30, this session
extern int32_t actor_find_best_search_node(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_find_best_search_node at 0x409070
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t unit_seat_index_is_valid(void); // 0x565150, not yet rewritten
extern int32_t actor_avoid_obstacle_and_project(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_avoid_obstacle_and_project at 0x4095c0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, this module,
                                                    // blam-cc: EAX -> destination, stack -> the other three

uint8_t actor_build_order_search_object(uint32_t actor_index, uint32_t unknown_target, float target_x, float target_y, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;
    uint8_t result = 0;

    for (i = 0x13; i != 0; i--) {
        *body = 0;
        body++;
    }
    order[8] = *(uint32_t *)&target_x;
    order[9] = *(uint32_t *)&target_y;

    if (a->active_unit_index == (datum_index)k_datum_index_none && a->swarm == 0 && a->mode != _actor_mode_vocalize) {
        if (actor_is_within_alert_range(0, target_x, target_y, 0, 0, actor_index, unknown_target) != 0) {
            int16_t search_slot;

            order[0] = unknown_target;
            search_slot = actor_find_best_search_node(actor_index);
            *(int16_t *)(order + 1) = search_slot;
            if (search_slot != -1) {
                *((uint8_t *)order + 6) = 1;
                if (unit_seat_index_is_valid() != 0) {
                    uint8_t scratch[36];
                    if (actor_avoid_obstacle_and_project(scratch, 0, order + 0xc, order + 0x12) != 0) {
                        // 0x4089e5..0x408a0f: EAX = order + 0x30 bytes, then push ebx / edx / edi,
                        // i.e. (&order[0xc], actor_index, order[0x12], unknown_target).
                        if (actor_movement_set_destination_point((real_point3d *)(order + 0xc), actor_index,
                                                                 (int32_t)order[0x12], unknown_target) != 0) {
                            result = 1;
                        }
                    }
                }
            }
        }
    }
    return result;
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
