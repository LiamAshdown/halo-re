// actor_claim_firing_position  (Ghidra: actor_claim_firing_position, renamed)
// address 0x414060, size 214 bytes
// name confidence: 0.55  rewrite confidence: 0.7
// evidence: types/ai.h names this address as one of the two writers of
//   actor.firing_position_index (0x3b8) together with the 0x3ba / 0x3bb pair, and its
//   callees are actor_movement_action_stop @0x417570 and
//   actor_movement_set_destination_firing_position @0x417830.
// register convention: AL -> a "released voluntarily" flag, CX -> the firing position
//   index, and the two datum handles are the Ghidra-recognized stack parameters. Both
//   callees are invoked with no visible arguments, so their register arguments are
//   carried in from this frame -- see UNSURE below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
                                                                 // blam-cc: EDX -> actor_index
extern uint8_t actor_movement_set_destination_firing_position(void);      // 0x417830, not yet rewritten
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);                   // 0x4141a0, rewritten in this module

// blam-cc: AL -> keep_claim, CX -> firing_position_index, stack -> actor_index, previous_owner
// Moves the actor onto a firing position. Passing -1 just stops the current movement action
// and drops the claim. Otherwise any previously claimed, different position is pushed onto
// the recognition ring, an optional previous owner is evicted from the same slot, and the
// actor is sent to the position; if the movement request is refused the claim is dropped
// again. Returns the claim the actor ends up holding.
int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
                                    int16_t firing_position_index, uint8_t keep_claim)
{
    actor *self;
    actor *other;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (firing_position_index == -1) {
        actor_movement_action_stop(actor_index);
    } else {
        if (self->firing_position_index != -1 && self->firing_position_index != firing_position_index) {
            // UNSURE: Ghidra shows a bare actor_push_recognition_entry() here. The recognition entry it
            // pushes must be the position being given up, with the type byte already in DL;
            // both are register arguments this frame happens to hold.
            actor_push_recognition_entry(actor_index, self->firing_position_index, 0);
        }

        if (previous_owner != (datum_index)0xffffffff) {
            // The base pointer is reloaded before the call in the original, so the eviction
            // below is not disturbed by anything actor_movement_action_stop does.
            other = (actor *)((uint8_t *)actor_data->data + (previous_owner & 0xffff) * sizeof(actor));
            actor_movement_action_stop(actor_index);
            other->firing_position_index = -1;
        }

        if (self->firing_position_index == firing_position_index) {
            return self->firing_position_index;
        }

        self->firing_position_index = firing_position_index;
        self->unknown_3ba = (uint8_t)(keep_claim == 0);
        self->unknown_3bb = 0;
        if (actor_movement_set_destination_firing_position() != 0) {
            return self->firing_position_index;
        }
    }

    self->firing_position_index = -1;
    return self->firing_position_index;
}

#if 0
Original Ghidra decompilation (0x414060):

undefined2 FUN_00414060(uint param_1,uint param_2)

{
  int iVar1;
  char in_AL;
  char cVar2;
  short in_CX;
  int iVar3;

  iVar3 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (in_CX == -1) {
    actor_movement_action_stop();
  }
  else {
    if ((*(short *)(iVar3 + 0x3b8) != -1) && (*(short *)(iVar3 + 0x3b8) != in_CX)) {
      FUN_004141a0();
    }
    if (param_2 != 0xffffffff) {
      iVar1 = *(int *)(DAT_00880360 + 0x34);
      actor_movement_action_stop();
      *(undefined2 *)((param_2 & 0xffff) * 0x724 + iVar1 + 0x3b8) = 0xffff;
    }
    if (*(short *)(iVar3 + 0x3b8) == in_CX) goto LAB_0041412a;
    *(short *)(iVar3 + 0x3b8) = in_CX;
    *(bool *)(iVar3 + 0x3ba) = in_AL == '\0';
    *(undefined1 *)(iVar3 + 0x3bb) = 0;
    cVar2 = actor_movement_set_destination_firing_position();
    if (cVar2 != '\0') goto LAB_0041412a;
  }
  *(undefined2 *)(iVar3 + 0x3b8) = 0xffff;
LAB_0041412a:
  return *(undefined2 *)(iVar3 + 0x3b8);
}
#endif
