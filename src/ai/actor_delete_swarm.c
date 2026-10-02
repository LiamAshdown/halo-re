// actor_delete_swarm  (Ghidra: actor_delete_swarm, already named)
// address 0x4280b0, size 121 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: types/ai.h swarm.component_count (0x02), actor.swarm_index (0x28). Calls
//   datum_delete (0x4d0510, EAX -> array, EDX -> handle).
//   UNSURE: the per-component datum_delete calls have no visible arguments in Ghidra's
//   decompile; modeled as deleting swarm_component_data[i] for i in [0, component_count),
//   which is what the surrounding loop and swarm.component_index[] array strongly imply, but
//   not independently confirmed with objdump.
// register convention: EAX -> actor_index; no other register operands are read.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle

// blam-cc: EAX -> actor_index
// If the actor has an associated swarm, deletes every swarm-member datum and the swarm datum
// itself, then clears the actor's swarm reference.
void actor_delete_swarm(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index swarm_index = self->swarm_index;

    if (swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[swarm_index & 0xffff];
        int16_t i;

        for (i = 0; i < s->component_count; i++) {
            datum_delete(swarm_component_data, s->component_index[i]);
        }
        datum_delete(swarm_data, swarm_index);
        self->swarm_index = (datum_index)k_datum_index_none;
    }
}

#if 0
Original Ghidra decompilation (0x4280b0):

void actor_delete_swarm(void)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  short sVar4;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar1 = *(uint *)(iVar2 + 0x28);
  if (uVar1 != 0xffffffff) {
    iVar3 = (uVar1 & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    sVar4 = 0;
    if (0 < *(short *)(iVar3 + 2)) {
      do {
        datum_delete();
        sVar4 = sVar4 + 1;
      } while (sVar4 < *(short *)(iVar3 + 2));
    }
    datum_delete();
    *(undefined4 *)(iVar2 + 0x28) = 0xffffffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
