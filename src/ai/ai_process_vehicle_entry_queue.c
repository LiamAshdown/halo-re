// ai_process_vehicle_entry_queue  (Ghidra: ai_process_vehicle_entry_queue, already named)
// address 0x42bf90, size 423 bytes
// name confidence: 0.9   rewrite confidence: 0.4
// evidence: out/phase4/ai_functions.md signature; types/tags.h Unit.seats (TagReflexive at
// Unit+0x2e4, inherited by Vehicle) and UnitSeat.built_in_gunner (TagDependency at
// UnitSeat+0xf8, its tag_id therefore at +0x104) account for the seat-walk and the
// built-in-gunner tag id test; types/objects.h object.parent_object (0x11c) accounts for the
// vehicle's carrier lookup before the marker-transform call.
// register convention: plain __cdecl, no parameters (drains ai_globals's own queue).
// blam-cc: (no arguments)
//
// UNSURE: matrix4x3_transform_point's EAX (out point) and EDX (source point) registers are
// not traced by this function -- its own result is never read back here, so the call is
// reproduced for its side effect only, with UNSURE placeholders for out/point.
// UNSURE: object+0x1f2 (an int16 read as a node/point count) and object+0x120 (a per-vehicle
// byte, the placed unit's assigned marker/node index) are not named in types/objects.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr;  // 0x00880354
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index actor_place_new_unit(datum_index actor_variant_tag_id, datum_index param_2,
                                         datum_index param_3, uint32_t param_4, uint32_t param_5); // 0x427080, not yet rewritten
extern void matrix4x3_transform_point(real_point3d *out, const real_point3d *point,
                                      const real_matrix4x3 *matrix); // 0x4cbde0, UNSURE: EAX/EDX not traced here
extern char unit_enter_vehicle_seat(uint32_t unit_object_index, int16_t seat); // 0x566970, not yet rewritten

// blam-cc: (no arguments)
// Drains ai_globals's queued "unit wants to enter this vehicle" requests. For each queued
// vehicle, walks its Vehicle/Unit tag's seat list; every seat with a built-in gunner
// actor_variant assigned gets a freshly placed unit (optionally repositioning the vehicle's
// carrying parent object's matching node first), which then boards that seat.
void ai_process_vehicle_entry_queue(void)
{
    int16_t queue_index;
    datum_index vehicle_object_index;
    object *vehicle_object;
    Vehicle *vehicle_tag;
    int32_t seat_count;
    int32_t seat_index;
    UnitSeat *seats;
    datum_index built_in_gunner_tag_id;
    datum_index parent_object_index;
    object *parent_object;
    datum_index placed_unit;
    real_point3d out_point;
    real_point3d source_point;

    if (ai_globals_ptr->vehicle_entry_count < 1) {
        ai_globals_ptr->vehicle_entry_count = 0;
        return;
    }

    queue_index = 0;
    do {
        vehicle_object_index = ai_globals_ptr->vehicle_entry_queue[queue_index];
        vehicle_object = ((object_header *)object_data->data)[vehicle_object_index & 0xffff].data;
        vehicle_tag = (Vehicle *)tag_instances[vehicle_object->definition_tag & 0xffff].data;

        seat_count = vehicle_tag->base.seats.count;
        if (0 < seat_count) {
            seats = (UnitSeat *)vehicle_tag->base.seats.pointer;
            seat_index = 0;
            do {
                built_in_gunner_tag_id = *(datum_index *)&seats[seat_index].built_in_gunner.tag_id;
                if (built_in_gunner_tag_id != (datum_index)k_datum_index_none) {
                    vehicle_object = ((object_header *)object_data->data)[vehicle_object_index & 0xffff].data;
                    parent_object_index = vehicle_object->parent_object;
                    if (parent_object_index != (datum_index)k_datum_index_none) {
                        parent_object = ((object_header *)object_data->data)[parent_object_index & 0xffff].data;
                        matrix4x3_transform_point(&out_point, &source_point,
                            (real_matrix4x3 *)((uint8_t *)parent_object +
                                *(int16_t *)((uint8_t *)parent_object + 0x1f2) +
                                (int32_t)*(int8_t *)((uint8_t *)vehicle_object + 0x120) * 0x34));
                    }
                    placed_unit = actor_place_new_unit(built_in_gunner_tag_id, (datum_index)k_datum_index_none,
                                                        (datum_index)k_datum_index_none, 0, 0);
                    if (placed_unit != (datum_index)k_datum_index_none) {
                        unit_enter_vehicle_seat(vehicle_object_index, seat_index);
                    }
                }
                seat_index = seat_index + 1;
            } while ((int16_t)seat_index < seat_count);
        }
        queue_index = queue_index + 1;
    } while (queue_index < ai_globals_ptr->vehicle_entry_count);

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
