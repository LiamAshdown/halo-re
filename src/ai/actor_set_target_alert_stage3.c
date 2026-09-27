// actor_set_target_alert_stage3  (Ghidra: actor_set_target_alert_stage3; named from out/phase2/results/ai_02.json)
// address 0x41fbc0, size 158 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase2/results/ai_02.json -- largest of the 0xb9/0xba/0xbb trio: for a valid
//   prop (EDX) sets prop.noticed_c (0xbb), additionally promoting a kind-4 prop to kind 5; for
//   the sentinel case EDX==none it instead clears several actor-local perception fields
//   (unknown_3c4, unknown_3bc, unknown_3bd[0], unknown_72, unknown_74) that
//   actor_update_awareness_level (0x420290) consumes.
// register convention: EDX -> target_prop_index (or k_datum_index_none), ESI -> actor_index
//   (unaff_ESI).
//   // blam-cc: EDX -> target_prop_index, ESI -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void actor_update_target_combat_status(datum_index actor_index); // 0x4200d0, UNSURE signature
extern void actor_update_awareness_level(datum_index actor_index);       // 0x420290, UNSURE signature

// blam-cc: EDX -> target_prop_index, ESI -> actor_index
// Marks the third per-prop alert flag and promotes the prop's kind, or, when no target prop is
// supplied, clears the actor's own perception/awareness accumulator fields.
void actor_set_target_alert_stage3(datum_index target_prop_index, datum_index actor_index)
{
    actor *self;
    prop *target;

    if (target_prop_index == k_datum_index_none) {
        self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
        *(int16_t *)&self->unknown_3c4 = 0;
        self->unknown_3bc = 0;
        self->unknown_3bd[0] = 0;
        self->unknown_72 = 0;
        self->unknown_74 = 0;
        actor_update_awareness_level(actor_index);
        return;
    }

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    if (target->kind == 4) {
        target->kind = 5;
    }
    target->noticed_c = 1;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (target_prop_index == self->target_unit_index) {
        actor_update_target_combat_status(actor_index);
        actor_update_awareness_level(actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x41fbc0):

void FUN_0041fbc0(void)

{
  int iVar1;
  int iVar2;
  uint in_EDX;
  uint unaff_ESI;

  if (in_EDX == 0xffffffff) {
    iVar1 = (unaff_ESI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    *(undefined2 *)(iVar1 + 0x3c4) = 0;
    *(undefined1 *)(iVar1 + 0x3bc) = 0;
    *(undefined1 *)(iVar1 + 0x3bd) = 0;
    *(undefined2 *)(iVar1 + 0x72) = 0;
    *(undefined2 *)(iVar1 + 0x74) = 0;
    actor_update_awareness_level();
    return;
  }
  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar2 = (in_EDX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (*(short *)(iVar2 + 0x24) == 4) {
    *(undefined2 *)(iVar2 + 0x24) = 5;
  }
  *(undefined1 *)(iVar2 + 0xbb) = 1;
  if (in_EDX == *(uint *)((unaff_ESI & 0xffff) * 0x724 + iVar1 + 0x270)) {
    actor_update_target_combat_status();
    actor_update_awareness_level();
    return;
  }
  return;
}
#endif
