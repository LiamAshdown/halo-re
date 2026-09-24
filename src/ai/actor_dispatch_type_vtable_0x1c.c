// actor_dispatch_type_vtable_0x1c  (Ghidra: actor_dispatch_type_vtable_0x1c, already named)
// address 0x4266d0, size 63 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: types/ai.h actor_type_table_entry.proc_1c (0x1c); same shape as
//   actor_dispatch_type_vtable_0x10 @0x426670 (guarded by a NULL check).
// register convention: ECX -> actor_index.
//   // blam-cc: ECX -> actor_index
// UNSURE: real per-type callee signature unknown; see actor_dispatch_type_vtable_0x10.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern void *actor_type_procs[16];  // 0x006853b8

// blam-cc: ECX -> actor_index
// Calls the actor-type-specific callback at vtable slot 0x1c, if one is registered.
void actor_dispatch_type_vtable_0x1c(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    if (entry->proc_1c != 0) {
        ((void (*)(datum_index))entry->proc_1c)(actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x4266d0):

void actor_dispatch_type_vtable_0x1c(void)

{
  uint in_ECX;

  if (*(code **)((&PTR_PTR_006853b8)
                 [*(short *)((in_ECX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 0x1c)
      != (code *)0x0) {
    (**(code **)((&PTR_PTR_006853b8)
                 [*(short *)((in_ECX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 0x1c)
    )();
  }
  return;
}
#endif
