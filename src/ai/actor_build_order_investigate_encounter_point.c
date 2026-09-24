// actor_build_order_investigate_encounter_point  (Ghidra: already named)
// address 0x408a30, size 287 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h actor.active_unit_index (0x158)/swarm (0x06); types/objects.h
//   object.up (0x80, checked upright via up.k >= 0.5)/vitality_flags (0x106,
//   _object_health_frozen_bit already named); phase-4 summary "builds an order sending the
//   actor to investigate an encounter-derived point of interest, subject to vitality and
//   perception checks".
// register convention: actor index in EAX, an object index in EBX, a category/kind word and
//   the order/output record as the recognized stack parameters.
//   // blam-cc: EAX -> actor_index, EBX -> object_index, stack -> kind/order
// TYPES-GAP: `order`'s tail matches actor_build_order_search_object.c's layout exactly
//   (same offsets 0/2/6/0xc/0x12), reinforcing that both share an unnamed order-tail shape
//   not present in types/ai.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern uint8_t unit_seat_index_is_valid(void); // 0x565150, not yet rewritten
extern int32_t actor_evaluate_search_node(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_evaluate_search_node at 0x4091d0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int32_t actor_avoid_obstacle_and_project(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_avoid_obstacle_and_project at 0x4095c0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, this module,
                                                    // blam-cc: EAX -> destination, stack -> the other three

uint8_t actor_build_order_investigate_encounter_point(uint32_t actor_index, uint32_t object_index, int16_t kind, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;
    uint8_t result = 0;

    for (i = 0x13; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->active_unit_index == (datum_index)k_datum_index_none && a->swarm == 0) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (obj->up.k >= 0.5f && (obj->vitality_flags & _object_health_frozen_bit) == 0) {
            order[0] = object_index;
            *(int16_t *)(order + 1) = kind;
            *((uint8_t *)order + 6) = 0;
            if (unit_seat_index_is_valid() != 0 && actor_evaluate_search_node(actor_index) != 0) {
                uint8_t scratch[36];
                if (actor_avoid_obstacle_and_project(scratch, 0, order + 0xc, order + 0x12) != 0) {
                    // 0x408b09..0x408b33: EAX = order + 0x30 bytes, then push ebx / edx / edi,
                    // i.e. (&order[0xc], actor_index, order[0x12], object_index). Ghidra hides
                    // both the EAX destination and the trailing object-index argument.
                    if (actor_movement_set_destination_point((real_point3d *)(order + 0xc), actor_index,
                                                             (int32_t)order[0x12], object_index) != 0) {
                        result = 1;
                    }
                }
            }
        }
    }
    return result;
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
