// unit_get_move_speed_for_range  (Ghidra: unit_get_move_speed_for_range; named from out/phase2/results/ai_02.json)
// address 0x41bed0, size 348 bytes, 0 callers in this build
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: out/phase2/results/ai_02.json describes this as a "pure math helper interpolating
//   forward/turn speed outputs from unit speed-profile thresholds at unit+0x1c/0x20/0x28/0x2c
//   based on a distance", but tracing the base pointer shows it is not a unit at all: it is
//   actor.actor_definition_tag (0x58) resolved through tag_instances, i.e. the Actor tag data
//   block, and types/tags.h (generated from invader's own Actor tag definition, so its offsets
//   are exact) names 0x1c/0x20/0x28/0x2c central_vision_angle / max_vision_angle /
//   peripheral_vision_angle / peripheral_distance.
// register convention: EAX -> actor_index; param_1..param_5 are Ghidra's recognized stack
//   parameters.
//   // blam-cc: EAX -> actor_index, stack -> param_a, param_b, param_dist, out_a, out_b
//
// UNSURE, substantially: this function has zero callers in this build, so none of its
// parameter meanings can be cross-checked against a call site. The tag fields it reads are
// vision angles/a distance, not a speed profile, so the phase-2 "forward/turn speed" framing
// is very likely wrong; the computation is preserved exactly, but param_1/param_2/param_3 and
// the two outputs are named generically (a, b, dist) rather than asserting they are speeds.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> actor_index, stack -> param_a, param_b, param_dist, out_a, out_b
// Blends two caller-supplied values (param_a, param_b) against the actor's Actor tag vision
// thresholds based on param_dist, writing the result through out_a/out_b. See UNSURE above.
void unit_get_move_speed_for_range(datum_index actor_index, float param_a, float param_b, float param_dist,
                                   float *out_a, float *out_b)
{
    actor *self;
    Actor *definition;
    float far_value;
    float near_value_07;
    float far_value_07_clamped;
    float fraction;
    float low_break;      // central_vision_angle, the first breakpoint
    float low_break_08;   // low_break * 0.8, the second (lower) breakpoint

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    if (definition->peripheral_vision_angle < param_dist) {
        *out_b = 0.0f;
        *out_a = 0.0f;
        return;
    }

    param_a = param_a * param_b;
    far_value = param_b * definition->peripheral_distance;
    near_value_07 = param_a * 0.7f;
    far_value_07_clamped = far_value * 0.7f;
    if (3.5f < far_value_07_clamped) {
        far_value_07_clamped = 3.5f;
    }

    if (param_dist <= definition->max_vision_angle) {
        low_break = definition->central_vision_angle;
        low_break_08 = low_break * 0.8f;
        if (low_break <= param_dist) {
            fraction = (param_dist - low_break) / (definition->max_vision_angle - low_break);
            param_a = fraction * far_value + (1.0f - fraction) * param_a;
        }
        if (low_break_08 <= param_dist) {
            fraction = (param_dist - low_break_08) / (definition->max_vision_angle - low_break_08);
            *out_b = param_a;
            *out_a = fraction * far_value_07_clamped + (1.0f - fraction) * near_value_07;
            return;
        }
        *out_b = param_a;
        *out_a = near_value_07;
        return;
    }

    *out_b = param_b;
    *out_a = far_value_07_clamped;
}

#if 0
Original Ghidra decompilation (0x41bed0):

void FUN_0041bed0(float param_1,float param_2,float param_3,float *param_4,float *param_5)

{
  float fVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  uint in_EAX;
  float local_8;

  iVar2 = *(int *)((*(uint *)((in_EAX & 0xffff) * 0x724 + 0x58 + *(int *)(DAT_00880360 + 0x34)) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(float *)(iVar2 + 0x28) < param_3) {
    *param_5 = 0.0;
    *param_4 = 0.0;
    return;
  }
  param_1 = param_1 * param_2;
  param_2 = param_2 * *(float *)(iVar2 + 0x2c);
  fVar3 = param_1 * 0.7;
  local_8 = param_2 * 0.7;
  if (3.5 < local_8) {
    local_8 = 3.5;
  }
  if (param_3 <= *(float *)(iVar2 + 0x20)) {
    fVar1 = *(float *)(iVar2 + 0x1c);
    fVar4 = fVar1 * 0.8;
    if (fVar1 <= param_3) {
      fVar1 = (param_3 - fVar1) / (*(float *)(iVar2 + 0x20) - fVar1);
      param_1 = fVar1 * param_2 + (1.0 - fVar1) * param_1;
    }
    if (fVar4 <= param_3) {
      fVar1 = (param_3 - fVar4) / (*(float *)(iVar2 + 0x20) - fVar4);
      *param_5 = param_1;
      *param_4 = fVar1 * local_8 + (1.0 - fVar1) * fVar3;
      return;
    }
    *param_5 = param_1;
    *param_4 = fVar3;
    return;
  }
  *param_5 = param_2;
  *param_4 = local_8;
  return;
}
#endif
