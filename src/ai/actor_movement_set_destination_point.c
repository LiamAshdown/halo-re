// actor_movement_set_destination_point  (Ghidra: actor_movement_set_destination_point, already named)
// address 0x417610, size 308 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: types/ai.h's actor.queued_movement/active_movement comment already credits this
// address with writing the queued copy at actor+0x400 and copying all six dwords to
// active_movement (0x46c); builds an explicit-point (type 2) movement action, short-circuiting
// if the new destination is within 0.1 units of an already-active type-2 action with the same
// parameter.
// register convention: destination point in EAX (Ghidra's in_EAX, dropped from the visible
// signature); actor_index, parameter and extra are genuine stack parameters.
// blam-cc: EAX -> destination, stack -> actor_index, stack -> parameter, stack -> extra
// UNSURE: the early-out path's return value is a NaN-safe rendering of "fVar1 < 0.01 ||
// fVar1 == 0.01" that Ghidra folds into a CONCAT31 with garbage high bits; only the low byte
// (always 1 on this path) is meaningful to callers, so this rewrite returns plain 1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, blam-cc: EAX, BL
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context); // 0x41a460, this module

// blam-cc: EAX -> destination, stack -> actor_index, stack -> parameter, stack -> extra
uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index, int32_t parameter, uint32_t extra)
{
    actor *self;
    float dx, dy, dz;
    float dist2;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->firing_position_index = -1;
    actor_set_units_active(actor_index, 0);

    if (self->active_movement.type == 2 && self->active_movement.parameter == parameter) {
        dx = self->active_movement.destination.x - destination->x;
        dy = self->active_movement.destination.y - destination->y;
        dz = self->active_movement.destination.z - destination->z;
        dist2 = dz * dz + dy * dy + dx * dx;
        if (dist2 <= 0.010000001f) {
            if (self->needs_new_path != 0 && self->path_resolved_this_tick == 0) {
                return actor_movement_action_resolve(actor_index, 0, 0);
            }
            return 1;
        }
    }

    self->queued_movement.type = 2;
    self->queued_movement.cancelled = 0;
    self->queued_movement.destination = *destination;
    self->queued_movement.parameter = parameter;
    self->queued_movement.extra = extra;
    self->active_movement = self->queued_movement;
    return actor_movement_action_resolve(actor_index, 1, 0);
}

#if 0
Original Ghidra decompilation (0x417610):

undefined4 actor_movement_set_destination_point(uint param_1,int param_2,undefined4 param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  undefined2 extraout_var;
  undefined4 uVar4;
  int iVar5;

  iVar5 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined2 *)(iVar5 + 0x3b8) = 0xffff;
  actor_set_units_active();
  if ((*(short *)(iVar5 + 0x46c) == 2) && (*(int *)(iVar5 + 0x47c) == param_2)) {
    fVar1 = *(float *)(iVar5 + 0x470) - *in_EAX;
    fVar3 = *(float *)(iVar5 + 0x474) - in_EAX[1];
    fVar2 = *(float *)(iVar5 + 0x478) - in_EAX[2];
    fVar1 = fVar2 * fVar2 + fVar3 * fVar3 + fVar1 * fVar1;
    if (fVar1 < 0.010000001 != 0 || (fVar1 == 0.010000001) != 0) {
      if ((*(char *)(iVar5 + 0x4c) != '\0') && (*(char *)(iVar5 + 0x4a4) == '\0')) {
        uVar4 = actor_movement_action_resolve(param_1,0,0);
        return uVar4;
      }
      return CONCAT31((int3)(CONCAT22(extraout_var,
                                      (ushort)(fVar1 < 0.010000001) << 8 | (ushort)NAN(fVar1) << 10
                                      | (ushort)(fVar1 == 0.010000001) << 0xe) >> 8),1);
    }
  }
  *(undefined2 *)(iVar5 + 0x400) = 2;
  *(undefined1 *)(iVar5 + 0x402) = 0;
  *(float *)(iVar5 + 0x404) = *in_EAX;
  *(float *)(iVar5 + 0x408) = in_EAX[1];
  *(float *)(iVar5 + 0x40c) = in_EAX[2];
  *(int *)(iVar5 + 0x410) = param_2;
  *(undefined4 *)(iVar5 + 0x414) = param_3;
  *(undefined4 *)(iVar5 + 0x46c) = *(undefined4 *)(iVar5 + 0x400);
  *(undefined4 *)(iVar5 + 0x470) = *(undefined4 *)(iVar5 + 0x404);
  *(undefined4 *)(iVar5 + 0x474) = *(undefined4 *)(iVar5 + 0x408);
  *(undefined4 *)(iVar5 + 0x478) = *(undefined4 *)(iVar5 + 0x40c);
  *(undefined4 *)(iVar5 + 0x47c) = *(undefined4 *)(iVar5 + 0x410);
  *(undefined4 *)(iVar5 + 0x480) = *(undefined4 *)(iVar5 + 0x414);
  uVar4 = actor_movement_action_resolve(param_1,1,0);
  return uVar4;
}
#endif
