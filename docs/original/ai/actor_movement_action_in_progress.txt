// actor_movement_action_in_progress  (Ghidra: actor_movement_action_in_progress, already named)
// address 0x41a980, size 52 bytes
// name confidence: 0.55  rewrite confidence: 0.7
// evidence: returns false only when movement_action_complete is set AND movement_completed
// is still clear (i.e. an action is pending completion but hasn't finished); true otherwise
// (no action pending, or it already finished).
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
uint8_t actor_movement_action_in_progress(datum_index actor_index)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->movement_action_complete != 0 && self->movement_completed == 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x41a980):

undefined4 actor_movement_action_in_progress(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724;
  if ((*(char *)(iVar1 + 0x4a8 + *(int *)(DAT_00880360 + 0x34)) != '\0') &&
     (*(char *)(iVar1 + *(int *)(DAT_00880360 + 0x34) + 0x484) == '\0')) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
