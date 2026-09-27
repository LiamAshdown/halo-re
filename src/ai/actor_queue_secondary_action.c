// actor_queue_secondary_action  (Ghidra: actor_queue_secondary_action, renamed)
// address 0x417a60, size 127 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/ai.h already credits this address with queuing actor.secondary_action
// (0x418), read back by actor_action_has_queued_secondary (0x417b70, same module); refuses
// to queue a new one while a secondary action is already pending, or while the actor's unit
// is busy per unit_is_in_busy_animation_state.
// register convention: actor_index in EAX (Ghidra's in_EAX); action and its two-dword
// payload are genuine stack parameters.
// blam-cc: EAX -> actor_index, stack -> action, stack -> payload
// UNSURE: the two dwords at actor+0x41c/0x420 (inside unknown_41a[16]) have no established
// meaning beyond being the payload this function copies in verbatim.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, blam-cc: EAX, BL
extern uint8_t unit_is_in_busy_animation_state(datum_index actor_index); // 0x569c90, not yet rewritten

// blam-cc: EAX -> actor_index, stack -> action, stack -> payload
uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2])
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_set_units_active(actor_index, 0);

    if (self->secondary_action != (int16_t)-1) {
        return 0;
    }
    if (self->unit_index != (datum_index)k_datum_index_none && unit_is_in_busy_animation_state(actor_index)) {
        return 0;
    }

    self->secondary_action = action;
    *(uint32_t *)&self->unknown_41a[2] = payload[0]; // 0x41c
    *(uint32_t *)&self->unknown_41a[6] = payload[1]; // 0x420
    return 1;
}

#if 0
Original Ghidra decompilation (0x417a60):

undefined4 FUN_00417a60(undefined2 param_1,undefined4 *param_2)

{
  int iVar1;
  char cVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;

  iVar1 = DAT_00880360;
  iVar3 = (in_EAX & 0xffff) * 0x724;
  iVar4 = *(int *)(DAT_00880360 + 0x34) + iVar3;
  actor_set_units_active();
  iVar3 = *(int *)(iVar1 + 0x34) + iVar3;
  if (*(short *)(iVar3 + 0x418) != -1) {
    return 0;
  }
  if ((*(int *)(iVar3 + 0x18) != -1) && (cVar2 = FUN_00569c90(), cVar2 != '\0')) {
    return 0;
  }
  *(undefined2 *)(iVar4 + 0x418) = param_1;
  *(undefined4 *)(iVar4 + 0x41c) = *param_2;
  *(undefined4 *)(iVar4 + 0x420) = param_2[1];
  return 1;
}
#endif
