// actor_clear_vocalization  (Ghidra: actor_clear_vocalization, already named)
// address 0x414560, size 46 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: phase-4 summary; zeroes actor.vocalization_line/_variant/_state (actor+0x544,
// +0x546, +0x548), the same three fields types/ai.h documents this function as pinning.
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
void actor_clear_vocalization(datum_index actor_index)
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->vocalization_variant = 0;
    self->vocalization_line = 0;
    self->vocalization_state = 0;
}

#if 0
Original Ghidra decompilation (0x414560):

void actor_clear_vocalization(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined2 *)(iVar1 + 0x546) = 0;
  *(undefined2 *)(iVar1 + 0x544) = 0;
  *(undefined2 *)(iVar1 + 0x548) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
