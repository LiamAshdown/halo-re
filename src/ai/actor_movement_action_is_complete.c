// actor_movement_action_is_complete  (Ghidra: actor_movement_action_is_complete, already named)
// address 0x41a960, size 28 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: returns actor.movement_action_complete (0x4a8) directly.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index
uint8_t actor_movement_action_is_complete(datum_index actor_index)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    return self->movement_action_complete;
}

#if 0
Original Ghidra decompilation (0x41a960):

undefined4 actor_movement_action_is_complete(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724;
  return CONCAT31((int3)((uint)iVar1 >> 8),
                  *(undefined1 *)(iVar1 + 0x4a8 + *(int *)(DAT_00880360 + 0x34)));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
