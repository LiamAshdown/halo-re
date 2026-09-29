// actor_action_has_queued_secondary  (Ghidra: actor_action_has_queued_secondary, already named)
// address 0x417b70, size 57 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: mirrors actor_queue_secondary_action's own gating exactly: true when
// secondary_action is already set, or when the actor's unit is busy per unit_is_in_busy_animation_state.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX = the unit (actor +0x18)

// blam-cc: EAX -> actor_index
uint8_t actor_action_has_queued_secondary(datum_index actor_index)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->secondary_action != (int16_t)-1) {
        return 1;
    }
    if (self->unit_index != (datum_index)k_datum_index_none && unit_is_in_busy_animation_state(self->unit_index)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x417b70):

undefined4 actor_action_has_queued_secondary(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(short *)(iVar2 + 0x418) != -1) {
    return 1;
  }
  if ((*(int *)(iVar2 + 0x18) != -1) && (cVar1 = FUN_00569c90(), cVar1 != '\0')) {
    return 1;
  }
  return 0;
}
#endif
