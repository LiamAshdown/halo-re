// actor_release_from_cluster_or_delete  (Ghidra: actor_release_from_cluster_or_delete, renamed)
// address 0x428e50, size 80 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/ai.h actor.cluster_count(0x1e)/encounter_index(0x34). Calls
//   actor_remove_from_unit_cluster (0x427c90) and actor_delete (0x427e60), both already
//   rewritten in this module, plus encounter_recompute_morale (outside this rewrite's range, UNSURE
//   signature -- likely the encounter morale/redistribution pass per ai_types_notes.md).
// register convention: EAX -> actor_index, stack -> unit_index.
//   // blam-cc: EAX -> actor_index, stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60
extern void encounter_recompute_morale(datum_index encounter_index); // 0x437940, UNSURE signature, not in this rewrite range

// blam-cc: EAX -> actor_index, stack -> unit_index
// Reduces an actor's cluster by one unit and, once the cluster is fully empty, notifies the
// actor's associated encounter via the shared release helpers.
void actor_release_from_cluster_or_delete(datum_index actor_index, datum_index unit_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    actor_remove_from_unit_cluster(actor_index, unit_index);
    if (self->cluster_count == 0) {
        datum_index encounter_index = self->encounter_index;
        actor_delete(actor_index, 1);
        if (encounter_index != (datum_index)k_datum_index_none) {
            encounter_recompute_morale(encounter_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x428e50):

void FUN_00428e50(undefined4 param_1)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  actor_remove_from_unit_cluster(param_1);
  if (*(short *)(iVar1 + 0x1e) == 0) {
    iVar1 = *(int *)(iVar1 + 0x34);
    actor_delete(1);
    if (iVar1 != -1) {
      FUN_00437940(iVar1);
    }
  }
  return;
}
#endif
