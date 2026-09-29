// actor_is_burst_pending  (Ghidra: actor_is_burst_pending, renamed)
// address 0x428180, size 45 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x428180..0x4281ac.)
// evidence: types/ai.h actor.awareness_level(0x6a)/alert_floor/alert_level. Phase-4 summary:
// "Returns whether the actor's combat sub-state (0x6a) is 3 and a per-burst counter (0x72)
// has not yet reached its configured length (0x6e); exact semantics of the state value are
// not confirmed."
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index
uint8_t actor_is_burst_pending(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    return self->awareness_level == 3 && self->alert_floor < self->alert_level;
}

#if 0
Original Ghidra decompilation (0x428180):

int FUN_00428180(void)

{
  uint in_EAX;
  int iVar1;
  uint3 uVar2;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar2 = (uint3)((uint)iVar1 >> 8);
  if ((*(short *)(iVar1 + 0x6a) == 3) && (*(short *)(iVar1 + 0x72) < *(short *)(iVar1 + 0x6e))) {
    return CONCAT31(uVar2,1);
  }
  return (uint)uVar2 << 8;
}
#endif
