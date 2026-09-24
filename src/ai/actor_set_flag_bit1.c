// actor_set_flag_bit1  (Ghidra: actor_set_flag_bit1, renamed)
// address 0x42a5b0, size 39 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: types/ai.h actor.flags (0x6d0), _actor_flag_unknown_bit1 (0x2, "0x42a5b0 sets it
//   and nothing else reads it here"). Phase-4 summary: "Sets a single status-flag bit (0x2)
//   on the actor's general flags field (+0x6d0); the precise meaning of the bit is not
//   confirmed."
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index
void actor_set_flag_bit1(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    self->flags |= _actor_flag_unknown_bit1;
}

#if 0
Original Ghidra decompilation (0x42a5b0):

void FUN_0042a5b0(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724;
  *(uint *)(iVar1 + *(int *)(DAT_00880360 + 0x34) + 0x6d0) =
       *(uint *)(iVar1 + 0x6d0 + *(int *)(DAT_00880360 + 0x34)) | 2;
  return;
}
#endif
