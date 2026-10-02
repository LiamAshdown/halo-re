// actor_get_target_prop_object_index  (Ghidra: actor_get_target_prop_object_index, renamed)
// address 0x4283d0, size 66 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump.)
// evidence: types/ai.h actor.target_unit_index (0x270); prop.object_index (0x18). As
//   actor_update_facing_change_timer @0x423670 already notes, this module reuses
//   actor.target_unit_index (a *unit* handle everywhere else) as a *prop* index at this
//   exact offset in more than one function; preserved literally here too.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

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
// Getter that returns the tracked object index of the prop referenced by actor.target_unit_index
// (read here as a prop index; see UNSURE above), or k_datum_index_none when it has none.
datum_index actor_get_target_prop_object_index(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
        return p->object_index;
    }
    return (datum_index)k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x4283d0):

undefined4 FUN_004283d0(void)

{
  uint uVar1;
  uint in_EAX;

  uVar1 = *(uint *)((in_EAX & 0xffff) * 0x724 + 0x270 + *(int *)(DAT_00880360 + 0x34));
  if (uVar1 != 0xffffffff) {
    return *(undefined4 *)((uVar1 & 0xffff) * 0x138 + 0x18 + *(int *)(DAT_008802c0 + 0x34));
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
