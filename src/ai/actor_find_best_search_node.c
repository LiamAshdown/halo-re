// actor_find_best_search_node  (Ghidra: FUN_00409070; really: pick the best vehicle seat for an actor to board)
// address 0x409070, size 342 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// REWRITTEN from objdump 0x409070..0x4091c5 (the draft dropped the direction output and returned the hint
//   where the entry point belongs). Stack (actor, vehicle, out_entry, out_direction, out_hint). Every seat of
//   the vehicle tag (+0x2e4) is scored by 0x4091d0; the highest score above 0 wins. Returns its seat index, or
//   -1; the optional outputs get that seat's entry point, approach direction and enter-hint point (the
//   original leaves stale stack there when nothing qualified; zero here).
// blam-cc: stack -> (actor_index, vehicle_index, out_entry, out_direction, out_hint)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score,
    uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front); // 0x4091d0

int16_t actor_find_best_search_node(datum_index actor_index, datum_index vehicle_index, real_point3d *out_entry,
                                    real_vector3d *out_direction, real_point3d *out_hint)
{
    uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)((object_header *)object_data->data)
                                                        [vehicle_index & 0xffff].data & 0xffff].data;
    int16_t best_seat = -1;
    float best_score = 0.0f;
    real_point3d best_entry = {0.0f, 0.0f, 0.0f};
    real_vector3d best_direction = {0.0f, 0.0f, 0.0f};
    real_point3d best_hint = {0.0f, 0.0f, 0.0f};
    int16_t i;

    for (i = 0; i < *(int32_t *)(vehicle_tag + 0x2e4); i++) {
        real_point3d entry;
        real_vector3d direction;
        real_point3d hint;
        float score;

        if (actor_evaluate_search_node(actor_index, vehicle_index, i, &entry, &direction, &hint, &score, 0, 0, 0) &&
            score > best_score) {
            best_score = score;
            best_entry = entry;
            best_direction = direction;
            best_hint = hint;
            best_seat = i;
        }
    }
    if (out_entry != 0) {
        *out_entry = best_entry;
    }
    if (out_direction != 0) {
        *out_direction = best_direction;
    }
    if (out_hint != 0) {
        *out_hint = best_hint;
    }
    return best_seat;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
