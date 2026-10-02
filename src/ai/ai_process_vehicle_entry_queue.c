// ai_process_vehicle_entry_queue  (Ghidra: ai_process_vehicle_entry_queue, already named)
// address 0x42bf90, size 423 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// VERIFIED against disassembly 0x42bf90..0x42c136 (2026-09-30); rewritten from it (the draft left the placement request out of actor_place_new_unit,
//   never used the transformed point and entered the seat without the gunner's unit).
//   For every queued vehicle (ai_globals +0x8bc, count +0x8b8), every seat (tag +0x2e4/+0x2e8, 0x11c each) with a
//   built-in gunner (seat +0x104) gets a zeroed placement request (+0x1a = -1) at the vehicle's position (+0x5c),
//   or that position through its parent's node matrix (parent +0x1f2 node array, vehicle +0x120 node) when it is
//   carried; the gunner actor is placed (actor_place_new_unit(tag, -1, -1, 0, 0, EAX request)) and its unit
//   (actor +0x18) enters the seat (EAX unit, stack vehicle, seat). The queue is then emptied.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#include <string.h>
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern data_array *object_data;     // 0x008603b0
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index actor_place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index,
    int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index,
    const actor_placement_request *placement_request); // 0x427080, stack, EAX request
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

void ai_process_vehicle_entry_queue(void)
{
    int16_t queue_index;

    for (queue_index = 0; queue_index < ai_globals_ptr->vehicle_entry_count; queue_index++) {
        datum_index vehicle_index = ai_globals_ptr->vehicle_entry_queue[queue_index];
        uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)OBJECT_DATA(vehicle_index) & 0xffff].data;
        int16_t seat_index;

        for (seat_index = 0; seat_index < *(int32_t *)(vehicle_tag + 0x2e4); seat_index++) {
            datum_index gunner_tag = *(datum_index *)(*(uint8_t **)(vehicle_tag + 0x2e8) + seat_index * 0x11c + 0x104);
            actor_placement_request request;
            uint8_t *vehicle;
            datum_index actor_index;

            if (gunner_tag == k_datum_index_none) {
                continue;
            }
            memset(&request, 0, 0x1c);
            *(int16_t *)((uint8_t *)&request + 0x1a) = -1;
            vehicle = OBJECT_DATA(vehicle_index);
            if (((vehicle_object *)vehicle)->base.parent_object == k_datum_index_none) {
                request.position = *(real_point3d *)&((vehicle_object *)vehicle)->base.position.x;
            } else {
                uint8_t *parent = OBJECT_DATA(((vehicle_object *)vehicle)->base.parent_object);

                matrix4x3_transform_point(&request.position, (real_point3d *)(vehicle + 0x5c),
                    (real_matrix4x3 *)(parent + ((struct object *)parent)->nodes.offset + (int8_t)vehicle[0x120] * 0x34));
            }
            actor_index = actor_place_new_unit(gunner_tag, k_datum_index_none, -1, 0, 0, &request);
            if (actor_index != k_datum_index_none) {
                unit_enter_vehicle_seat(vehicle_index, seat_index,
                    *(datum_index *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724 + 0x18));
            }
        }
    }
    ai_globals_ptr->vehicle_entry_count = 0;
}

#if 0
Original Ghidra decompilation (0x42bf90):

void __cdecl ai_process_vehicle_entry_queue(void)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  short sVar7;
  int iVar8;
  int iVar9;

  sVar7 = 0;
  iVar6 = DAT_00880354;
  if (*(short *)(DAT_00880354 + 0x8b8) < 1) {
    *(undefined2 *)(DAT_00880354 + 0x8b8) = 0;
    return;
  }
  do {
    uVar1 = *(uint *)(iVar6 + 0x8bc + sVar7 * 4);
    iVar9 = (uVar1 & 0xffff) * 0xc;
    iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9) & 0xffff) * 0x20 + 0x14
                    + DAT_0087bc14);
    iVar8 = 0;
    if (0 < *(int *)(iVar2 + 0x2e4)) {
      iVar5 = 0;
      do {
        iVar6 = *(int *)(iVar5 * 0x11c + 0x104 + *(int *)(iVar2 + 0x2e8));
        if (iVar6 != -1) {
          iVar5 = *(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34));
          uVar3 = *(uint *)(iVar5 + 0x11c);
          if (uVar3 != 0xffffffff) {
            iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
            matrix4x3_transform_point
                      ((int)*(short *)(iVar4 + 0x1f2) + *(char *)(iVar5 + 0x120) * 0x34 + iVar4);
          }
          iVar6 = actor_place_new_unit(iVar6,0xffffffff,0xffffffff,0,0);
          if (iVar6 != -1) {
            unit_enter_vehicle_seat(uVar1,iVar8);
          }
        }
        iVar8 = iVar8 + 1;
        iVar5 = (int)(short)iVar8;
        iVar6 = DAT_00880354;
      } while (iVar5 < *(int *)(iVar2 + 0x2e4));
    }
    sVar7 = sVar7 + 1;
  } while (sVar7 < *(short *)(iVar6 + 0x8b8));
  *(undefined2 *)(iVar6 + 0x8b8) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
