// actor_movement_action_stop  (Ghidra: actor_movement_action_stop, already named)
// address 0x417570, size 152 bytes
// name confidence: 0.55  rewrite confidence: 0.5
// evidence: when unknown_15e==4 and unknown_504 is set, forwards to
// actor_movement_set_destination_point with the actor's own body_position as the
// destination (objdump-verified: EAX = &self->body_position, parameter = unknown_164,
// extra = -1) -- i.e. "stop by targeting where you already are". Otherwise builds a plain
// type-1 (stop) queued/active movement action directly and resolves it.
// register convention: actor_index in EDX (Ghidra's in_EDX).
// blam-cc: EDX -> actor_index
// UNSURE: actor.unknown_164 (int32) has no established meaning beyond being forwarded here
// as the "parameter" field of the synthesized point-destination action.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index, int32_t parameter, uint32_t extra); // 0x417610, this module
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context); // 0x41a460, this module

// blam-cc: EDX -> actor_index
void actor_movement_action_stop(datum_index actor_index)
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->vehicle_driving_type == 4 && self->moving != 0) {
        actor_movement_set_destination_point(&self->body_position, actor_index, self->pathfinding_surface_index, (uint32_t)-1);
        return;
    }

    self->firing_position_index = -1;
    if (self->active_movement.type != 1) {
        self->queued_movement.type = 1;
        self->active_movement = self->queued_movement;
    }
    actor_movement_action_resolve(actor_index, 1, 0);
}

#if 0
Original Ghidra decompilation (0x417570):

void actor_movement_action_stop(void)

{
  int iVar1;
  uint in_EDX;

  iVar1 = (in_EDX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(short *)(iVar1 + 0x15e) == 4) && (*(char *)(iVar1 + 0x504) != '\0')) {
    actor_movement_set_destination_point();
    return;
  }
  *(undefined2 *)(iVar1 + 0x3b8) = 0xffff;
  if (*(short *)(iVar1 + 0x46c) != 1) {
    *(undefined2 *)(iVar1 + 0x400) = 1;
    *(undefined4 *)(iVar1 + 0x46c) = *(undefined4 *)(iVar1 + 0x400);
    *(undefined4 *)(iVar1 + 0x470) = *(undefined4 *)(iVar1 + 0x404);
    *(undefined4 *)(iVar1 + 0x474) = *(undefined4 *)(iVar1 + 0x408);
    *(undefined4 *)(iVar1 + 0x478) = *(undefined4 *)(iVar1 + 0x40c);
    *(undefined4 *)(iVar1 + 0x47c) = *(undefined4 *)(iVar1 + 0x410);
    *(undefined4 *)(iVar1 + 0x480) = *(undefined4 *)(iVar1 + 0x414);
  }
  actor_movement_action_resolve();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
