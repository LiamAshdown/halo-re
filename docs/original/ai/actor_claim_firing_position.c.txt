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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
                                                                 // blam-cc: EDX -> actor_index
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
    path_find_context *path_context); // 0x417830, EDI, stack
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);                   // 0x4141a0, rewritten in this module

// blam-cc: AL -> path_ok, CX -> firing_position_index, stack -> actor_index, previous_owner, path_context
// Moves the actor onto a firing position. Passing -1 just stops the current movement action
// and drops the claim. Otherwise any previously claimed, different position is pushed onto
// the recognition ring, an optional previous owner is evicted from the same slot, and the
// actor is sent to the position; if the movement request is refused the claim is dropped
// again. Returns the claim the actor ends up holding.
int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
                                    path_find_context *path_context, int16_t firing_position_index,
                                    uint8_t path_ok)
{
    actor *self;
    actor *other;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (firing_position_index == -1) {
        actor_movement_action_stop(actor_index);
    } else {
        // 0x414094: the position being given up goes into the recognition history as type 1 (DL)
        if (self->firing_position_index != -1 && self->firing_position_index != firing_position_index) {
            actor_push_recognition_entry(actor_index, self->firing_position_index, 1);
        }

        // 0x4140b1: the previous owner (EDX) stops and loses the position
        if (previous_owner != (datum_index)0xffffffff) {
            other = (actor *)((uint8_t *)actor_data->data + (previous_owner & 0xffff) * sizeof(actor));
            actor_movement_action_stop(previous_owner);
            other->firing_position_index = -1;
        }

        if (self->firing_position_index == firing_position_index) {
            return self->firing_position_index;
        }

        self->firing_position_index = firing_position_index;
        self->firing_position_without_path = (uint8_t)(path_ok == 0);
        self->grenade_evasion_active = 0;
        // 0x41410d: a found path's context is handed on so the move reuses it
        if (actor_movement_set_destination_firing_position(actor_index, firing_position_index,
                path_ok ? path_context : (path_find_context *)0) != 0) {
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
