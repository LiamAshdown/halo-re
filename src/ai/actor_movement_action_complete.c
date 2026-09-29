// actor_movement_action_complete  (Ghidra: actor_movement_action_complete, already named)
// address 0x41a430, size 44 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: clears movement_action_complete (0x4a8), sets movement_completed (0x484), and
// zeroes movement_timer (0x4a0), matching its established name and the fields already
// documented as its own in types/ai.h.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index
void actor_movement_action_complete(datum_index actor_index)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->movement_action_complete = 0;
    self->movement_completed = 1;
    self->movement_timer = 0;
}

#if 0
Original Ghidra decompilation (0x41a430):

void actor_movement_action_complete(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined1 *)(iVar1 + 0x4a8) = 0;
  *(undefined1 *)(iVar1 + 0x484) = 1;
  *(undefined4 *)(iVar1 + 0x4a0) = 0;
  return;
}
#endif
