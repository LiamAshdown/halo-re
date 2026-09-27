// actor_dispatch_type_vtable_0x1c  (Ghidra: actor_dispatch_type_vtable_0x1c, already named)
// address 0x4266d0, size 63 bytes
// name confidence: 0.55   rewrite confidence: 1.0
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

// blam-cc: ECX -> actor_index, stack -> a, b, c
// FIXED (objdump 0x4266f9..0x42670b): forwards its three stack arguments -- proc(actor, a, b, c), cdecl; the draft
//   passed only the actor. actor_get_requested_velocity (a swarm actor's velocity) passes (param_1, speed_limit,
//   out_velocity).
void actor_dispatch_type_vtable_0x1c(datum_index actor_index, uint32_t a, uint32_t b, uint32_t c)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    if (entry->proc_1c != 0) {
        ((void (*)(datum_index, uint32_t, uint32_t, uint32_t))entry->proc_1c)(actor_index, a, b, c);
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
