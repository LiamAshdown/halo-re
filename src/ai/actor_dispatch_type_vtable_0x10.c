// actor_dispatch_type_vtable_0x10  (Ghidra: actor_dispatch_type_vtable_0x10, already named)
// address 0x426670, size 46 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop: objdump 0x426670..0x42669d, ECX actor pushed to the proc)
// evidence: types/ai.h actor_type_table_entry.proc_10 (0x10) and the global comment for
//   0x006853b8 actor_type_procs[16], indexed by actor.type (0x04). Phase-4 summary: "Invokes
//   the per-actor-type virtual callback at vtable slot 0x10, if one is registered for this
//   actor's type."
// register convention: ECX -> actor_index; the callee, whatever it is per actor type,
// receives whichever registers this dispatcher leaves untouched -- assumed to be the same
// actor_index in ECX, since nothing here sets up any other register.
//   // blam-cc: ECX -> actor_index
// UNSURE: the real per-type callee signature is unknown (it varies by ActorType and this
// module never defines any of them); modeled as a bare function pointer taking the actor
// index in ECX, matching the only register this dispatcher itself reads.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern void *actor_type_procs[16];  // 0x006853b8

// blam-cc: ECX -> actor_index
// Calls the actor-type-specific callback at vtable slot 0x10, if one is registered.
void actor_dispatch_type_vtable_0x10(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    if (entry->proc_10 != 0) {
        ((void (*)(datum_index))entry->proc_10)(actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x426670):

void actor_dispatch_type_vtable_0x10(void)

{
  uint in_ECX;

  if (*(code **)((&PTR_PTR_006853b8)
                 [*(short *)((in_ECX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 0x10)
      != (code *)0x0) {
    (**(code **)((&PTR_PTR_006853b8)
                 [*(short *)((in_ECX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 0x10)
    )();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
