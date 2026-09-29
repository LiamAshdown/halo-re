// actor_target_reset_shot_counters  (Ghidra: actor_target_reset_shot_counters, already named)
// address 0x41fa20, size 84 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase2/results/ai_02.json -- walks the same actor+0x50 target-data linked
//   list as actor_target_reset_seen_flags, zeroing three adjacent short counters at
//   +0xaa/+0xac/+0xae per entry. Matches prop.shots_fired/shots_hit/shots_unknown_ae in
//   types/ai.h.
// register convention: EAX -> actor_index; no other register operands are read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index
// Resets per-target shot/hit statistics counters for every prop (target-data record) the
// actor is tracking.
void actor_target_reset_shot_counters(datum_index actor_index)
{
    actor *self;
    datum_index prop_index;
    prop *target;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    prop_index = self->first_prop;
    while (prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        prop_index = target->next_in_actor;
        target->shots_fired = 0;
        target->shots_unknown_ae = 0;
        target->shots_hit = 0;
    }
}

#if 0
Original Ghidra decompilation (0x41fa20):

void actor_target_reset_shot_counters(void)

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
    *(undefined2 *)(iVar2 + 0xaa) = 0;
    *(undefined2 *)(iVar2 + 0xae) = 0;
    *(undefined2 *)(iVar2 + 0xac) = 0;
  }
  return;
}
#endif
