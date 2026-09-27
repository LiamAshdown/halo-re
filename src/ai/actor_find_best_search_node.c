// actor_find_best_search_node  (Ghidra: actor_find_best_search_node, renamed)
// address 0x409070, size 342 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/tags.h Unit tag (referenced via the unit's ActorVariant.unit dependency,
//   read here through the unit object's definition_tag); calls actor_evaluate_search_node
//   (this session, 0x4091d0) once per candidate index up to a per-unit-type table count at
//   offset 0x2e4; phase-4 summary "scans a cluster's list of candidate search positions and
//   returns the index and data for the highest-scoring one".
// register convention: already a full stack-parameter signature in the decompilation.
// UNSURE: the table this function bounds by `unit_definition+0x2e4` is not identified in
// types/tags.h (no Unit struct field is named at that offset here); kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index, void *out_entry, void *out_direction, void *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front); // 0x4091d0, returns AL

// Scans candidate search-node indices 0..count-1 (count from the target unit's type
// definition at +0x2e4) via actor_evaluate_search_node, keeping the highest-scoring one, and
// writes its position/direction/extra data out through out_position/out_direction/
// out_extra (each optional). Returns the winning index, or -1 if none qualified.
int32_t actor_find_best_search_node(uint32_t actor_index, uint32_t unit_object_index, float *out_position, float *out_direction, uint32_t *out_extra)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_object_index & 0xffff].data;
    uint8_t *unit_definition = (uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    int32_t count = *(int32_t *)(unit_definition + 0x2e4);
    int16_t i;
    int32_t best_index = -1;
    float best_score = 0.0f;
    float best_position[3] = {0, 0, 0};
    float best_direction[3] = {0, 0, 0};
    uint32_t best_extra[3] = {0, 0, 0};

    for (i = 0; i < count; i++) {
        float position[3];
        float direction[3];
        uint32_t extra[3];
        float score;

        if (actor_evaluate_search_node(actor_index, unit_object_index, (uint32_t)i, position, direction, extra, &score, 0, 0, 0) != 0 && best_score < score) {
            best_score = score;
            best_position[0] = position[0]; best_position[1] = position[1]; best_position[2] = position[2];
            best_direction[0] = direction[0]; best_direction[1] = direction[1]; best_direction[2] = direction[2];
            best_extra[0] = extra[0]; best_extra[1] = extra[1]; best_extra[2] = extra[2];
            best_index = i;
        }
    }

    // UNSURE/faithful: the original never writes through its "direction" out-parameter
    // (param_4) at all -- only position (param_3) and extra (param_5) -- so out_direction is
    // accepted (matching the recognized stack parameter) but intentionally left unused here,
    // exactly as the decompiled function leaves it.
    (void)out_direction;
    (void)best_direction;
    if (out_position != 0) {
        out_position[0] = best_position[0]; out_position[1] = best_position[1]; out_position[2] = best_position[2];
    }
    if (out_extra != 0) {
        out_extra[0] = best_extra[0]; out_extra[1] = best_extra[1]; out_extra[2] = best_extra[2];
    }
    return best_index;
}

#if 0
Original Ghidra decompilation (0x409070):

int FUN_00409070(undefined4 param_1,uint param_2,undefined4 *param_3,undefined4 *param_4,
                undefined4 *param_5)

{
  char cVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 uVar6;
  float local_4c;
  float local_48;
  int local_44;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_40 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc) &
                      0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar5 = 0;
  local_44 = -1;
  local_48 = 0.0;
  iVar2 = -1;
  uVar3 = local_38;
  uVar4 = local_3c;
  uVar6 = local_34;
  if (0 < *(int *)(local_40 + 0x2e4)) {
    do {
      cVar1 = FUN_004091d0(param_1,param_2,iVar5,&local_24,&local_18,&local_c,&local_4c,0,0,0);
      if ((cVar1 != '\0') && (local_48 < local_4c)) {
        local_48 = local_4c;
        local_3c = local_18;
        local_38 = local_14;
        local_34 = local_10;
        local_30 = local_c;
        local_2c = local_8;
        local_28 = local_4;
        uVar3 = local_20;
        uVar4 = local_24;
        uVar6 = local_1c;
        local_44 = iVar5;
      }
      iVar5 = iVar5 + 1;
      iVar2 = local_44;
    } while ((int)(short)iVar5 < *(int *)(local_40 + 0x2e4));
  }
  if (param_3 != (undefined4 *)0x0) {
    *param_3 = uVar4;
    param_3[1] = uVar3;
    param_3[2] = uVar6;
  }
  if (param_5 != (undefined4 *)0x0) {
    *param_5 = local_30;
    param_5[1] = local_2c;
    param_5[2] = local_28;
  }
  return iVar2;
}
#endif
