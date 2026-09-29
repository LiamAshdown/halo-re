// actor_movement_check_arrival  (Ghidra: actor_movement_check_arrival, renamed)
// address 0x416700, size 138 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (VERIFIED against 0x416700 (type gate, radius from actor_compute_accuracy_scale, squared-distance compare))
// evidence: reads actor.active_movement.type (0x46c) and, for any type other than 0
// (stop) or 1, compares the squared distance from actor.body_position to actor.movement_goal_position
// against actor_compute_accuracy_scale()'s squared engagement-range radius; if still outside that radius it
// leaves movement_completed alone, otherwise (or for type 0/1) it sets movement_completed.
// Returns the (possibly just-set) movement_completed flag.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360


// blam-cc: EAX -> actor_index
uint8_t actor_movement_check_arrival(datum_index actor_index)
{
    actor *self;
    float radius;
    float dx, dy, dz;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->active_movement.type != 0 && self->active_movement.type != 1) {
        radius = actor_compute_accuracy_scale(actor_index);
        dx = self->movement_goal_position.x - self->body_position.x;
        dy = self->movement_goal_position.y - self->body_position.y;
        dz = self->movement_goal_position.z - self->body_position.z;
        if (radius * radius <= dy * dy + dx * dx + dz * dz) {
            return self->movement_completed;
        }
    }
    self->movement_completed = 1;
    return self->movement_completed;
}

#if 0
Original Ghidra decompilation (0x416700):

undefined1 FUN_00416700(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;
  float10 fVar6;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  sVar1 = *(short *)(iVar2 + 0x46c + *(int *)(DAT_00880360 + 0x34));
  iVar2 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if ((sVar1 != 0) && (sVar1 != 1)) {
    fVar3 = (float10)FUN_00429620();
    fVar4 = (float10)*(float *)(iVar2 + 0x488) - (float10)*(float *)(iVar2 + 300);
    fVar5 = (float10)*(float *)(iVar2 + 0x48c) - (float10)*(float *)(iVar2 + 0x130);
    fVar6 = (float10)*(float *)(iVar2 + 0x490) - (float10)*(float *)(iVar2 + 0x134);
    if (fVar3 * fVar3 <= fVar5 * fVar5 + fVar4 * fVar4 + fVar6 * fVar6) goto LAB_00416782;
  }
  *(undefined1 *)(iVar2 + 0x484) = 1;
LAB_00416782:
  return *(undefined1 *)(iVar2 + 0x484);
}
#endif
