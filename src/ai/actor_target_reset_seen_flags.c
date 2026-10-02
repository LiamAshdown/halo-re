// actor_target_reset_seen_flags  (Ghidra: actor_target_reset_seen_flags, already named)
// address 0x41f9d0, size 71 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase2/results/ai_02.json -- walks the actor's linked target-data list
//   (actor+0x50 chain, stride 0x138) clearing the 'seen' byte at +0x74 and resetting the
//   last-seen-node id at +0x6c to -1 for every entry. Matches prop.seen (0x74) and
//   prop.seen_state (0x6c) in types/ai.h.
// register convention: EAX -> actor_index; no other register operands are read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index
// Clears the visibility/'seen' state of every prop (target-data record) the actor is
// currently tracking.
void actor_target_reset_seen_flags(datum_index actor_index)
{
    actor *self;
    datum_index prop_index;
    prop *target;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    prop_index = self->first_prop;
    while (prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        prop_index = target->next_in_actor;
        target->seen = 0;
        target->seen_state = -1;
    }
}

#if 0
Original Ghidra decompilation (0x41f9d0):

void actor_target_reset_seen_flags(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  uint uVar3;

  iVar1 = DAT_008802c0;
  uVar3 = *(uint *)((in_EAX & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
  while (uVar3 != 0xffffffff) {
    iVar2 = (uVar3 & 0xffff) * 0x138 + *(int *)(iVar1 + 0x34);
    uVar3 = *(uint *)(iVar2 + 8);
    *(undefined1 *)(iVar2 + 0x74) = 0;
    *(undefined2 *)(iVar2 + 0x6c) = 0xffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
