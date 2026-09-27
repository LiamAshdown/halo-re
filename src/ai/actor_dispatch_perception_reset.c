// actor_dispatch_perception_reset  (Ghidra: actor_dispatch_perception_reset, renamed)
// address 0x429000, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (FIXED against objdump)
// evidence: types/ai.h actor.swarm(0x06)/unknown_07/swarm_index(0x28); swarm.component_count.
//   Calls actor_reset_perception_scratch (0x428f40, already rewritten in this module, though
//   with low confidence).
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *swarm_data; // 0x0088035c

extern void actor_reset_perception_scratch(datum_index unit_index); // 0x428f40

// blam-cc: EAX -> actor_index
// Runs the per-unit perception reset either once for a solo actor (using its own unit) or
// once per swarm member (using the swarm's component count as the repeat count -- the
// original does not thread each member's own unit index through, an apparent quirk
// preserved here), then marks the actor as initialized.
void actor_dispatch_perception_reset(datum_index actor_index)
{
    // FIXED (objdump 0x429000..0x429077): the reset takes a UNIT in ESI -- the actor's unit (+0x18) or, for a swarm,
    //   each component unit (swarm +0x18 + i*4). The draft passed the actor index every time.
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm == 0) {
        actor_reset_perception_scratch(self->unit_index);
        self->unknown_07 = 1;
        return;
    }

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            actor_reset_perception_scratch(s->unit_index[i]);
        }
    }
    self->unknown_07 = 1;
}

#if 0
Original Ghidra decompilation (0x429000):

void FUN_00429000(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  short sVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar1 + 6) == '\0') {
    FUN_00428f40();
    *(undefined1 *)(iVar1 + 7) = 1;
    return;
  }
  if (*(uint *)(iVar1 + 0x28) != 0xffffffff) {
    iVar2 = (*(uint *)(iVar1 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    sVar3 = 0;
    if (0 < *(short *)(iVar2 + 2)) {
      do {
        FUN_00428f40();
        sVar3 = sVar3 + 1;
      } while (sVar3 < *(short *)(iVar2 + 2));
    }
  }
  *(undefined1 *)(iVar1 + 7) = 1;
  return;
}
#endif
