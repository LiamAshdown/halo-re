// actor_set_target_alert_stage2  (Ghidra: actor_set_target_alert_stage2; named from out/phase2/results/ai_02.json)
// address 0x41fb60, size 85 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase2/results/ai_02.json -- byte-for-byte identical logic to
//   actor_set_target_alert_stage1 (0x41fb00) except it writes prop.noticed_b (0xba) instead of
//   noticed_a (0xb9).
// register convention: ECX -> target_prop_index, ESI -> actor_index (unaff_ESI).
//   // blam-cc: ECX -> target_prop_index, ESI -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void actor_update_target_combat_status(datum_index actor_index); // 0x4200d0, UNSURE signature
extern void actor_update_awareness_level(datum_index actor_index);       // 0x420290, UNSURE signature

// blam-cc: ECX -> target_prop_index, ESI -> actor_index
// Marks a second per-prop alert/notice flag (noticed_b) and refreshes the actor's target
// combat status if that prop is the actor's current target.
void actor_set_target_alert_stage2(datum_index target_prop_index, datum_index actor_index)
{
    prop *target;
    actor *self;

    if (target_prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
        target->noticed_b = 1;

        self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
        if (target_prop_index == self->target_unit_index) {
            actor_update_target_combat_status(actor_index);
            actor_update_awareness_level(actor_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x41fb60):

void FUN_0041fb60(void)

{
  int iVar1;
  uint in_ECX;
  uint unaff_ESI;

  if (in_ECX != 0xffffffff) {
    iVar1 = *(int *)(DAT_00880360 + 0x34);
    *(undefined1 *)((in_ECX & 0xffff) * 0x138 + 0xba + *(int *)(DAT_008802c0 + 0x34)) = 1;
    if (in_ECX == *(uint *)((unaff_ESI & 0xffff) * 0x724 + iVar1 + 0x270)) {
      actor_update_target_combat_status();
      actor_update_awareness_level();
      return;
    }
  }
  return;
}
#endif
