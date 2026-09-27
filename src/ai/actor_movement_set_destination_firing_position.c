// actor_movement_set_destination_firing_position  (Ghidra: actor_movement_set_destination_formation_point, renamed)
// address 0x417830, size 220 bytes
// name confidence: 0.55  rewrite confidence: 0.6
// evidence: identical shape to actor_movement_set_destination_move_position but for a type-3
// (formation slot) action, and additionally clears actor+0x3bb on the build-new-action path
// (unlike the firing-point setter, and without the 0x3b8 = -1 reset the other setters do).
// register convention: actor_index in EDI (Ghidra's unaff_EDI); formation_slot is a genuine
// stack parameter, matching actor_movement_set_destination_move_position's own layout exactly.
// blam-cc: EDI -> actor_index, stack -> formation_slot
// UNSURE: actor.unknown_3bb has no established meaning beyond being cleared here.
// RENAMED by the Opus module review: this setter queues movement-action type 3, and the
// dispatcher's type-3 case (objdump jump table at 0x41a948 -> 0x41a611) indexes the owning
// ScenarioEncounter's firing_positions block (+0x9c, stride 0x18 == ScenarioFiringPosition).
// The phase-2 note that named it ..._formation_point had cases 3 and 4 the wrong way round.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, blam-cc: EAX, BL
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context); // 0x41a460, this module

// blam-cc: EDI -> actor_index, stack -> formation_slot
uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot)
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_set_units_active(actor_index, 0);

    if (self->active_movement.type != 3 || *(int16_t *)&self->active_movement.destination != formation_slot) {
        self->queued_movement.type = 3;
        self->queued_movement.cancelled = 0;
        *(int16_t *)&self->queued_movement.destination = formation_slot;
        self->queued_movement.extra = (uint32_t)-1;
        self->active_movement = self->queued_movement;
        self->unknown_3bb = 0;
        return actor_movement_action_resolve(actor_index, 1, 0);
    }
    if (self->needs_new_path != 0 && self->unknown_4a4 == 0) {
        return actor_movement_action_resolve(actor_index, 0, 0);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x417830):

undefined4 actor_movement_set_destination_firing_position(short param_1)

{
  undefined4 uVar1;
  int iVar2;
  uint unaff_EDI;

  iVar2 = (unaff_EDI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  actor_set_units_active();
  if ((*(short *)(iVar2 + 0x46c) != 3) || (*(short *)(iVar2 + 0x470) != param_1)) {
    *(undefined2 *)(iVar2 + 0x400) = 3;
    *(undefined1 *)(iVar2 + 0x402) = 0;
    *(short *)(iVar2 + 0x404) = param_1;
    *(undefined4 *)(iVar2 + 0x414) = 0xffffffff;
    *(undefined4 *)(iVar2 + 0x46c) = *(undefined4 *)(iVar2 + 0x400);
    *(undefined4 *)(iVar2 + 0x470) = *(undefined4 *)(iVar2 + 0x404);
    *(undefined4 *)(iVar2 + 0x474) = *(undefined4 *)(iVar2 + 0x408);
    *(undefined4 *)(iVar2 + 0x478) = *(undefined4 *)(iVar2 + 0x40c);
    *(undefined4 *)(iVar2 + 0x47c) = *(undefined4 *)(iVar2 + 0x410);
    *(undefined4 *)(iVar2 + 0x480) = *(undefined4 *)(iVar2 + 0x414);
    *(undefined1 *)(iVar2 + 0x3bb) = 0;
    uVar1 = actor_movement_action_resolve();
    return uVar1;
  }
  if ((*(char *)(iVar2 + 0x4c) != '\0') && (*(char *)(iVar2 + 0x4a4) == '\0')) {
    uVar1 = actor_movement_action_resolve();
    return uVar1;
  }
  return 1;
}
#endif
