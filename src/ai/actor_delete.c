// actor_delete  (Ghidra: actor_delete, already named)
// address 0x427e60, size 216 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/ai.h actor.unknown_09/swarm(0x06)/cluster_unit_index(0x24); already
//   established elsewhere in this module: actor_delete's own signature
//   (EBX -> actor_index, stack -> flag, per src/ai/ai_clear_object_references.c),
//   actor_unlink_unit, actor_remove_from_unit_cluster, actor_delete_swarm and
//   actor_clear_perceived_props (all already rewritten in this module). data_iterator_next
//   walks prop_data the same way as ai_clear_object_references.c's own loop; prop.owner_actor_index
//   is 0x1c per types/ai.h.
//   UNSURE: encounter_remove_actor (0x436620, renamed from squad_remove_actor) and
//   ai_actor_unlink_from_unassigned_list/ai_conversation_clear_participant (outside this
//   rewrite's range) are called with only their "flag" stack argument visible in Ghidra's
//   decompile; the actor_index they also need is assumed to be the same EBX passthrough
//   every other call in this function relies on.
// register convention: EBX -> actor_index, stack -> flag.
//   // blam-cc: EBX -> actor_index, stack -> flag
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters); // 0x436620, blam-cc: EAX -> actor_index, stack -> skip_counters
extern void ai_actor_unlink_from_unassigned_list(datum_index actor_index); // 0x436990, EDI -> actor_index
extern void actor_unlink_unit(datum_index actor_index); // 0x427bc0
extern void actor_delete_swarm(datum_index actor_index); // 0x4280b0
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90
extern void actor_clear_perceived_props(datum_index actor_index); // 0x427e00
extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void ai_conversation_clear_participant(datum_index actor_index); // 0x430c70, UNSURE signature, not in this rewrite range
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle

// blam-cc: EBX -> actor_index, stack -> flag
// Fully tears down and frees an actor: removes it from its squad (or the alternate teardown
// path when unknown_09 is set), detaches it from its unit or, if it is a swarm, from every
// unit in its cluster and deletes the swarm; clears its perceived-prop list; scrubs any
// prop.owner_actor_index in the whole prop table that still points at it; runs one more
// unestablished cleanup step; and finally deletes the actor's own datum.
void actor_delete(datum_index actor_index, uint32_t flag)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    data_iterator iterator;
    prop *p;

    if (self->encounterless == 0) {
        encounter_remove_actor(actor_index, (uint8_t)flag);
    } else {
        ai_actor_unlink_from_unassigned_list(actor_index);
    }

    if (self->swarm == 0) {
        actor_unlink_unit(actor_index);
    } else {
        actor_delete_swarm(actor_index);
        while (self->cluster_unit_index != (datum_index)k_datum_index_none) {
            actor_remove_from_unit_cluster(actor_index, self->cluster_unit_index);
        }
    }

    actor_clear_perceived_props(actor_index);

    iterator.data = prop_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (prop *)data_iterator_next(&iterator);
    while (p != 0) {
        if (p->owner_actor_index == actor_index) {
            p->owner_actor_index = (datum_index)k_datum_index_none;
        }
        p = (prop *)data_iterator_next(&iterator);
    }

    ai_conversation_clear_participant(actor_index);
    datum_delete(actor_data, actor_index);
}

#if 0
Original Ghidra decompilation (0x427e60):

void actor_delete(undefined4 param_1)

{
  uint unaff_EBX;
  int iVar1;
  int iVar2;

  iVar1 = (unaff_EBX & 0xffff) * 0x724;
  iVar2 = iVar1 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar1 + 9 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    squad_remove_actor(param_1);
  }
  else {
    FUN_00436990();
  }
  if (*(char *)(iVar2 + 6) == '\0') {
    actor_unlink_unit();
  }
  else {
    actor_delete_swarm();
    iVar1 = *(int *)(iVar2 + 0x24);
    while (iVar1 != -1) {
      actor_remove_from_unit_cluster(iVar1);
      iVar1 = *(int *)(iVar2 + 0x24);
    }
  }
  FUN_00427e00();
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if (*(uint *)(iVar1 + 0x1c) == unaff_EBX) {
      *(undefined4 *)(iVar1 + 0x1c) = 0xffffffff;
    }
    iVar1 = data_iterator_next();
  }
  FUN_00430c70();
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
