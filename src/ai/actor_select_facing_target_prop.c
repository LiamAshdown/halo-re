// actor_select_facing_target_prop  (Ghidra: actor_select_facing_target_prop, renamed)
// address 0x414a90, size 613 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (VERIFIED against objdump 0x414a90..0x414cf4)
// evidence: phase-4 summary "scans nearby recognized threats for the best-weighted one
// matching lane/direction criteria, used to pick which threat to react or dodge to"; its two
// callees in this address range are actor_point_in_directional_lane (0x414990, the full
// lane test using the actor's look-delta cosines) and the not-yet-rewritten point3d_within_horizontal_cone (a
// plain single-cone dot test, listed as a math-module function in
// out/phase4/ai_types_notes.md and skipped from this rewrite pass); one caller, inside
// 0x414d00, passes param_3 = &actor.unknown_56c (an 8-byte {int16, pad, datum} record) and
// param_1 = param_2 = 0 the other caller passes.
// register convention: actor_index in EAX (Ghidra's in_EAX); param_1/param_2/param_3/param_4
// are genuine stack arguments (Ghidra recognized all four correctly here).
// blam-cc: EAX -> actor_index, stack -> require_trust, stack -> skip_lane_test,
//   stack -> out_result, stack -> out_in_front
// UNSURE: the outer break/goto Ghidra reconstructs around the two candidate-acceptance tests
// (LAB_00414c7d / LAB_00414c85) re-enters the very loop it just broke out of and falls
// through to the same loop-continuation point either way; verified against
// objdump -d -M intel (0x414bd0..0x414c85) that this is not an early scan termination, just
// the decompiler's rendering of a compiler-shared tail block, so it is rewritten here as a
// plain if/else inside the scan loop with identical per-iteration behavior.
// UNSURE: require_trust/skip_lane_test are a guess at intent from their effect, not from any
// string or caller name; both observed callers pass (0,0) or a caller-chosen (param_3,param_4)
// pair from 0x414d00's own stack parameters, so their true meaning is not established.
// TYPES-GAP: actor_recognition_scan_result -- the ad hoc 8-byte {int16 flag; datum_index
// candidate;} output record every caller points into a different actor field (0x56c, 0x57c);
// listed in the module summary, not added to types/ai.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "ai.h"
#include "fn_ai.h"

// actor_recognition_scan_result now lives in types/ai.h (folded from this file).

extern data_array *actor_data;         // 0x00880360
extern data_array *prop_data;          // 0x008802c0
extern tag_instance *tag_instances;    // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern double cos(double x); // FCOS

extern uint8_t point3d_within_horizontal_cone(real_point3d *to_point, real_point3d *reference, float min_cos_threshold); // 0x414910, not in this module (math), EAX->to_point, EDX->reference, stack->threshold


// blam-cc: EAX -> actor_index, stack -> require_trust, stack -> skip_lane_test,
//   stack -> out_result, stack -> out_in_front
// Walks actor.first_prop's chain looking for the highest-weighted prop of kind 2 or 3 (with
// prop.perception_grade set) that also passes a directional test against the actor's look cones,
// and reports it through out_result / out_in_front. Every prop of the wrong kind has its
// priority_weight_spent reset to 0 in passing; the winning prop has priority_weight_spent raised to priority_weight and
// last_selected_tick stamped with the current tick, matching the pairing actor_begin_vocalization
// uses elsewhere in this module.
uint8_t actor_select_facing_target_prop(datum_index actor_index, uint8_t require_trust, uint8_t skip_lane_test,
                                         actor_recognition_scan_result *out_result, uint8_t *out_in_front)
{
    actor *self;
    Actor *definition;
    float side_thresholds[2];
    float delta_r;
    float aiming_cos_threshold;
    float looking_cos_threshold;
    int32_t now;
    datum_index next_handle;
    datum_index current_handle;
    prop *cur;
    float score;
    uint8_t still_valid;
    uint8_t best_found;
    datum_index best_handle;
    prop *best_prop;
    uint8_t best_in_front;
    float best_score;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    now = game_time->game_time; // +0x0c

    if (self->awareness_level == 3) { // 0x6a, exactly as Ghidra reads it
        side_thresholds[0] = (float)cos((double)definition->combat_look_delta_l);
        delta_r = definition->combat_look_delta_r;
    } else {
        side_thresholds[0] = (float)cos((double)definition->noncombat_look_delta_l);
        delta_r = definition->noncombat_look_delta_r;
    }
    side_thresholds[1] = (float)cos((double)delta_r);

    aiming_cos_threshold = definition->cosine_maximum_aiming_deviation.yaw;
    looking_cos_threshold = definition->cosine_maximum_looking_deviation.yaw;

    best_handle = (datum_index)k_datum_index_none;
    best_score = 0.0f;
    best_prop = 0;
    best_in_front = 0;
    best_found = 0;

    next_handle = self->first_prop;
    for (;;) {
        current_handle = next_handle;
        if (next_handle == (datum_index)k_datum_index_none) {
            break;
        }
        cur = &((prop *)prop_data->data)[next_handle & 0xffff];
        next_handle = cur->next_in_actor;

        if (!(cur->kind > 1 && cur->kind < 4 && cur->perception_grade != 0)) {
            cur->priority_weight_spent = 0.0f;
            continue;
        }
        if (cur->priority_weight <= 0.0f) {
            continue;
        }

        if (cur->last_selected_tick == -1) {
            score = 1.0f;
        } else {
            score = ((float)now - (float)cur->last_selected_tick) * 0.0016666667f - 1.0f; // 1/600
        }
        score = (cur->priority_weight - cur->priority_weight_spent) / cur->priority_weight + score;
        if (1.0f < score) {
            score = 1.0f;
        }
        score = score * cur->priority_weight;
        still_valid = (cur->priority_weight_spent < cur->priority_weight) ? 1 : 0;

        if (score <= 0.0f) {
            continue;
        }

        if (skip_lane_test) {
            uint8_t accepted;
            if (require_trust == 0 || !still_valid) {
                accepted = point3d_within_horizontal_cone((real_point3d *)&cur->look_point, &self->position_cache_a, aiming_cos_threshold);
            } else {
                accepted = 1; // trusted and still valid: accept without a directional check
            }
            if (accepted && best_score < score) {
                best_handle = current_handle;
                best_prop = cur;
                best_in_front = still_valid;
                best_score = score;
                best_found = 1;
            }
        } else {
            uint8_t accepted = actor_point_in_directional_lane((real_point3d *)&cur->look_point, &self->position_cache_b,
                                                                 &self->position_cache_a, looking_cos_threshold, side_thresholds);
            if (accepted && best_score < score) {
                best_handle = current_handle;
                best_prop = cur;
                best_in_front = still_valid;
                best_score = score;
                best_found = 1;
            }
        }
    }

    if (best_found && best_handle != (datum_index)k_datum_index_none) {
        best_prop->priority_weight_spent = best_prop->priority_weight;
        best_prop->last_selected_tick = now;
        out_result->candidate = best_handle;
        out_result->flag = 1;
        *out_in_front = best_in_front;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x414a90):

undefined4 FUN_00414a90(char param_1,char param_2,undefined2 *param_3,undefined4 param_4)

{
  float fVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  bool bVar7;
  char cVar8;
  uint in_EAX;
  int iVar9;
  int iVar10;
  float10 fVar11;
  bool local_2d;
  int local_28;
  float local_24;
  uint local_20;
  float local_10;
  float local_c;
  uint local_8;
  uint local_4;

  iVar9 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar3 = *(int *)((*(uint *)(iVar9 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)(DAT_006f1d6c + 0xc);
  uVar5 = *(undefined4 *)(iVar3 + 300);
  uVar6 = *(undefined4 *)(iVar3 + 0x134);
  local_20 = 0xffffffff;
  local_24 = 0.0;
  local_28 = 0;
  local_2d = false;
  if (*(short *)(iVar9 + 0x6a) == 3) {
    fVar11 = (float10)fcos((float10)*(float *)(iVar3 + 0xbc));
    fVar1 = *(float *)(iVar3 + 0xc0);
  }
  else {
    fVar11 = (float10)fcos((float10)*(float *)(iVar3 + 0xb4));
    fVar1 = *(float *)(iVar3 + 0xb8);
  }
  local_10 = (float)fVar11;
  fVar11 = (float10)fcos((float10)fVar1);
  local_4 = *(uint *)(iVar9 + 0x50);
  local_c = (float)fVar11;
  iVar9 = 0;
  while( true ) {
    do {
      do {
        while( true ) {
          local_8 = local_4;
          if (local_4 == 0xffffffff) {
            if (local_20 != 0xffffffff) {
              *(undefined4 *)(iVar9 + 0x58) = *(undefined4 *)(iVar9 + 0x54);
              *(int *)(iVar9 + 0x5c) = iVar4;
              *(uint *)(param_3 + 2) = local_20;
              *param_3 = 1;
              *(bool *)param_4 = local_2d;
              return 1;
            }
            return 0;
          }
          iVar3 = *(int *)(DAT_008802c0 + 0x34);
          iVar10 = (local_4 & 0xffff) * 0x138;
          sVar2 = *(short *)(iVar10 + 0x24 + iVar3);
          local_4 = *(uint *)(iVar10 + 8 + iVar3);
          iVar10 = iVar10 + iVar3;
          if (((1 < sVar2) && (sVar2 < 4)) && (*(short *)(iVar10 + 0x32) != 0)) break;
          *(undefined4 *)(iVar10 + 0x58) = 0;
        }
      } while (*(float *)(iVar10 + 0x54) <= 0.0);
      if (*(int *)(iVar10 + 0x5c) == -1) {
        fVar1 = 1.0;
      }
      else {
        fVar1 = ((float)iVar4 - (float)*(int *)(iVar10 + 0x5c)) * 0.0016666667 - 1.0;
      }
      fVar1 = (*(float *)(iVar10 + 0x54) - *(float *)(iVar10 + 0x58)) / *(float *)(iVar10 + 0x54) +
              fVar1;
      if (1.0 < fVar1) {
        fVar1 = 1.0;
      }
      fVar1 = fVar1 * *(float *)(iVar10 + 0x54);
      bVar7 = *(float *)(iVar10 + 0x58) < *(float *)(iVar10 + 0x54);
    } while (fVar1 <= 0.0);
    if (param_2 != '\0') break;
    cVar8 = FUN_00414990(uVar6,&local_10);
    iVar9 = local_28;
LAB_00414c7d:
    if (cVar8 != '\0') {
LAB_00414c85:
      if (local_24 < fVar1) {
        local_20 = local_8;
        iVar9 = iVar10;
        local_2d = bVar7;
        local_28 = iVar10;
        local_24 = fVar1;
      }
    }
  }
  if ((param_1 == '\0') || (!bVar7)) {
    cVar8 = FUN_00414910(uVar5);
    goto LAB_00414c7d;
  }
  goto LAB_00414c85;
}

Struct offsets recovered from the Actor tag layout (offsetof against types/tags.h, #pragma pack(1)):
  0xb4 noncombat_look_delta_l   0xb8 noncombat_look_delta_r
  0xbc combat_look_delta_l      0xc0 combat_look_delta_r
  0x12c cosine_maximum_aiming_deviation (Euler2D, .yaw read)
  0x134 cosine_maximum_looking_deviation (Euler2D, .yaw read)

Disassembly cross-check for the call site of FUN_00414910/actor_point_in_directional_lane
(objdump -d -M intel bin/halo.exe, 0x414c2a..0x414c76): EAX = &current_prop.look_point in both
calls, EDX = &actor.position_cache_a in both; ECX = &actor.position_cache_b only for the
0x414990 call; the float threshold is the only stack argument to either.
#endif
