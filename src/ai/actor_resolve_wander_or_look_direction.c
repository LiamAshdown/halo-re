// actor_resolve_wander_or_look_direction  (Ghidra: actor_resolve_wander_or_look_direction, renamed)
// address 0x4287a0, size 155 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/ai.h actor.swarm(0x06)/desired_direction_valid/movement_action_complete(0x4a8)/
//   movement_goal_position(real_point3d)/body_position(0x12c)/desired_direction(real_point3d, "look-direction
//   source"). Calls vector3d_normalize_with_length (0x401990, math module, already
//   established).
// register convention: EAX -> actor_index, ECX -> out_direction (real_vector3d*).
//   // blam-cc: EAX -> actor_index, ECX -> out_direction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990

// blam-cc: EAX -> actor_index, ECX -> out_direction
// Computes and normalizes a direction vector for the actor to look toward: for a non-swarm
// actor, either the delta from its body position to a cached wander destination
// (movement_goal_position, once movement_action_complete is set) or, failing that, its cached look
// vector (desired_direction, once desired_direction_valid is set); returns whether the result was
// non-degenerate. Always false for a swarm actor.
uint8_t actor_resolve_wander_or_look_direction(datum_index actor_index, real_vector3d *out_direction)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm != 0) {
        return 0;
    }

    if (self->desired_direction_valid == 0) {
        if (self->movement_action_complete == 0) {
            return 0;
        }
        out_direction->i = self->movement_goal_position.x - self->body_position.x;
        out_direction->j = self->movement_goal_position.y - self->body_position.y;
        out_direction->k = self->movement_goal_position.z - self->body_position.z;
    } else {
        out_direction->i = self->desired_direction.x;
        out_direction->j = self->desired_direction.y;
        out_direction->k = self->desired_direction.z;
    }

    return vector3d_normalize_with_length(out_direction) != 0.0f;
}

#if 0
Original Ghidra decompilation (0x4287a0):

uint FUN_004287a0(void)

{
  uint in_EAX;
  uint uVar1;
  uint uVar2;
  undefined2 extraout_var;
  uint3 uVar3;
  float *in_ECX;
  float10 fVar4;
  float10 fVar5;

  uVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar2 = uVar1 & 0xffffff00;
  if (*(char *)(uVar1 + 6) == '\0') {
    if (*(char *)(uVar1 + 0x504) == '\0') {
      if (*(char *)(uVar1 + 0x4a8) == '\0') {
        return uVar2;
      }
      *in_ECX = *(float *)(uVar1 + 0x488) - *(float *)(uVar1 + 300);
      in_ECX[1] = *(float *)(uVar1 + 0x48c) - *(float *)(uVar1 + 0x130);
      in_ECX[2] = *(float *)(uVar1 + 0x490) - *(float *)(uVar1 + 0x134);
    }
    else {
      *in_ECX = *(float *)(uVar1 + 0x518);
      in_ECX[1] = *(float *)(uVar1 + 0x51c);
      in_ECX[2] = *(float *)(uVar1 + 0x520);
    }
    fVar4 = (float10)vector3d_normalize_with_length();
    fVar5 = (float10)0.0;
    uVar3 = (uint3)(CONCAT22(extraout_var,
                             (ushort)(fVar5 < fVar4) << 8 | (ushort)(NAN(fVar5) || NAN(fVar4)) << 10
                             | (ushort)(fVar5 == fVar4) << 0xe) >> 8);
    if (fVar5 == fVar4) {
      return (uint)uVar3 << 8;
    }
    uVar2 = CONCAT31(uVar3,1);
  }
  return uVar2;
}
#endif
