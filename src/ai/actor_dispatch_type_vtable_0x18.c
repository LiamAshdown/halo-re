// actor_dispatch_type_vtable_0x18  (Ghidra: actor_dispatch_type_vtable_0x18, already named)
// address 0x4266a0, size 41 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop: objdump 0x4266a0..0x4266c8, EAX actor pushed to the proc)
// evidence: types/ai.h actor_type_table_entry.proc_18 (0x18); same shape as
//   actor_dispatch_type_vtable_0x10 @0x426670, but this one calls the slot unconditionally
//   with no NULL check -- preserved exactly, including the resulting crash if a type has no
//   proc registered there.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index
// UNSURE: real per-type callee signature unknown; see actor_dispatch_type_vtable_0x10.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;      // 0x00880360
extern void *actor_type_procs[16];  // 0x006853b8

// blam-cc: EAX -> actor_index
// Calls the actor-type-specific callback at vtable slot 0x18 unconditionally.
void actor_dispatch_type_vtable_0x18(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    ((void (*)(datum_index))entry->proc_18)(actor_index);
}

#if 0
Original Ghidra decompilation (0x4266a0):

void actor_dispatch_type_vtable_0x18(void)

{
  uint in_EAX;

  (**(code **)((&PTR_PTR_006853b8)
               [*(short *)((in_EAX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 0x18))
            ();
  return;
}
#endif
