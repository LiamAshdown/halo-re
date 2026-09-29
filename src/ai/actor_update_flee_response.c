// actor_update_flee_response  (Ghidra: actor_update_flee_response, renamed)
// address 0x414250, size 113 bytes
// name confidence: 0.45  rewrite confidence: 0.6
// evidence: the single callee is 0x4146c0, which types/ai.h records as the resolver of
//   actor.flee_from_point (0x2b0); the two counters it gates on are the pair at 0x3e8 and
//   0x3ec that actor_update_firing_state @0x40e7b0 and actor_begin_vocalization @0x4142d0
//   also read. Note those two fields carry the vocalization_unknown names in the header
//   because the vocalization path was the first reader found; this function shows the same
//   pair driving the flee reaction, so the names are a description of one user, not of the
//   field meaning. UNSURE.
// register convention: actor_index in EDX (Ghidra in_EDX); no stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out,
    datum_index actor_index); // 0x4146c0, EAX, EDI, stack

// blam-cc: EDX -> actor_index
// Tracks how long the flee condition at 0x3ec has held. While it is clear and the movement
// action has finished, the persistence counter at 0x3e8 is reset; once the counter passes
// two ticks with the condition still set, the flee-point resolver runs and its result is
// latched into unknown_505.
uint8_t actor_update_flee_response(datum_index actor_index)
{
    actor *self;
    uint8_t result;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    // UNSURE: in the original the fallback result is the address (actor + 0x3ec), which
    // cannot be a value the caller uses. The one caller only tests AL, so the failure path
    // returns zero here.
    result = 0;

    if (self->vocalization_unknown_3ec == 0 && self->movement_action_complete == 0) {
        self->vocalization_unknown_3e8 = 0;
    }

    if (self->vocalization_unknown_3e8 > 2 && self->vocalization_unknown_3ec != 0) {
        // FIXED (objdump 0x414274..0x4142a2): EAX = actor +0x3ec, EDI = actor +0x524, stack = the actor
        result = actor_resolve_flee_source_point((actor_flee_source_reason *)((uint8_t *)self + 0x3ec),
            &self->forced_aim_direction, actor_index);
        if (result != 0) {
            self->forced_aim = 1;
            return result;
        }
    }

    self->forced_aim = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x414250):

int FUN_00414250(void)

{
  int iVar1;
  int iVar2;
  uint in_EDX;

  iVar1 = (in_EDX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar2 = iVar1 + 0x3ec;
  if ((*(short *)(iVar1 + 0x3ec) == 0) && (*(char *)(iVar1 + 0x4a8) == '\0')) {
    *(undefined2 *)(iVar1 + 1000) = 0;
  }
  if ((2 < *(short *)(iVar1 + 1000)) && (*(short *)(iVar1 + 0x3ec) != 0)) {
    iVar2 = FUN_004146c0();
    if ((char)iVar2 != '\0') {
      *(undefined1 *)(iVar1 + 0x505) = 1;
      return iVar2;
    }
  }
  *(undefined1 *)(iVar1 + 0x505) = 0;
  return iVar2;
}
#endif
