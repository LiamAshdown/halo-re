// actor_prop_iterator_init  (Ghidra: actor_prop_iterator_init, renamed)
// address 0x43ecd0, size 32 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump.)
// evidence: types/ai.h actor.first_prop(+0x50). phase-4 summary "returns the head index of
// the current object's firing-position node linked list" (actually the owning actor's own
// prop list, chained through prop.next_in_actor, per the sibling functions in this cluster).
// Paired with actor_prop_iterator_next.c (0x43ecf0), which advances the {current,next}
// record this writes into.
// register convention: EAX -> actor_index; stack -> out_iterator.
//   // blam-cc: EAX -> actor_index, stack -> out_iterator
//
// UNSURE: `out_iterator`'s own field at +0x00 is never written here; only +0x04. The
// iterator record's full shape is inferred from actor_prop_iterator_next.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, stack -> out_iterator
void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    out_iterator->next = self->first_prop;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043ecd0 @ 0x43ecd0) ----
void FUN_0043ecd0(int param_1)

{
  uint in_EAX;

  *(undefined4 *)(param_1 + 4) =
       *(undefined4 *)((in_EAX & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
  return;
}
#endif
