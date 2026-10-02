// ai_clear_object_references  (Ghidra: ai_clear_object_references; named for this rewrite)
// address 0x42c140, size 351 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: phase-4 summary ("cleans up all AI-side references (recognized-object cache
// entries, squad references, vehicle-entry queue) to an object index that has just been
// deleted"). squad_clear_unit_references (0x430d30) is renamed here to
// ai_conversation_clear_object_references per out/phase4/ai_types_notes.md's misattribution
// table. actor_delete's real argument was recovered from disassembly (objdump -d -M intel
// --start-address=0x42c140 --stop-address=0x42c19d bin/halo.exe): Ghidra shows a literal
// `actor_delete(0)`, but the real call pushes 0 while the just-computed actor_index sits
// live in EBX, matching this module's register-passthrough convention.
// register convention: plain __cdecl, one stack argument.
// blam-cc: stack -> object_index
//
// UNSURE: actor_delete's stack argument (0 here) and actor_release_from_cluster_or_delete's role are not otherwise
// established in this repo.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *object_data;    // 0x008603b0
extern data_array *prop_data;      // 0x008802c0

extern void actor_release_from_cluster_or_delete(datum_index actor_index, datum_index unit_index); // 0x428e50, EAX, stack
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60, blam-cc: EBX -> actor_index, stack -> flag; not yet rewritten
extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference); // 0x428470, stack, ESI, EDI
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove); // 0x43ea20, EAX, EDI
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void ai_conversation_clear_object_references(datum_index object_index, uint8_t force_full_scan); // 0x430d30

// blam-cc: stack -> object_index
// Cleans up every AI-side reference to object_index once it has been deleted: deletes the
// controlling actor if the unit had one, or drops its swarm otherwise; releases or unlinks
// every recognized-object prop that referenced it (as tracked object or as relationship
// object); clears its AI-conversation references; and removes it from the pending
// vehicle-entry queue.
void ai_clear_object_references(datum_index object_index)
{
    object *obj;
    prop *p;
    data_iterator iterator;
    unit_data *unit;
    int16_t queue_count;
    int16_t i;

    if (!ai_globals_ptr->actors_valid) {
        return;
    }
    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    if (((1 << (obj->type & 0x1f)) & 3) == 0) {
        return; // only bipeds and vehicles carry a unit extension here
    }

    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor_delete(unit->actor_index, 0);
    } else if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        // FIXED (objdump 0x42c19d..0x42c1a9): EAX = the swarm actor (+0x1f8), stack = this unit
        actor_release_from_cluster_or_delete(unit->swarm_actor_index, object_index);
    }

    iterator.data = prop_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (prop *)data_iterator_next(&iterator);
    while (p != 0) {
        if (p->object_index == object_index) {
            // 0x42c1ec: stack the prop's actor, ESI -1, EDI the prop; then EAX actor, EDI prop
            actor_replace_object_reference(p->actor_index, 0xffffffff, iterator.index);
            actor_unlink_prop(p->actor_index, iterator.index);
            datum_delete(prop_data, iterator.index);
        } else if (p->relationship_object_index == (int32_t)object_index) {
            p->relationship_object_index = -1;
            p->is_vehicle_driver = 0;
            p->is_vehicle_gunner = 0;
        }
        p = (prop *)data_iterator_next(&iterator);
    }

    ai_conversation_clear_object_references(object_index, 1);

    queue_count = ai_globals_ptr->vehicle_entry_count;
    for (i = 0; i < queue_count; i++) {
        if (ai_globals_ptr->vehicle_entry_queue[i] == object_index) {
            queue_count = queue_count - 1;
            ai_globals_ptr->vehicle_entry_count = queue_count;
            if (0 < queue_count) {
                ai_globals_ptr->vehicle_entry_queue[i] = ai_globals_ptr->vehicle_entry_queue[queue_count];
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x42c140):

void FUN_0042c140(uint param_1)

{
  uint *puVar1;
  int iVar2;
  short sVar3;
  int iVar4;

  if ((*(char *)(DAT_00880354 + 1) != '\0') &&
     (iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc),
     (1 << (*(byte *)(iVar2 + 0xb4) & 0x1f) & 3U) != 0)) {
    if (*(int *)(iVar2 + 500) == -1) {
      if (*(int *)(iVar2 + 0x1f8) != -1) {
        FUN_00428e50(param_1);
      }
    }
    else {
      actor_delete(0);
    }
    iVar4 = DAT_00880354;
    iVar2 = data_iterator_next();
    while (iVar2 != 0) {
      if (*(uint *)(iVar2 + 0x18) == param_1) {
        actor_replace_object_reference(*(undefined4 *)(iVar2 + 4));
        FUN_0043ea20();
        datum_delete();
        iVar4 = DAT_00880354;
      }
      else if (*(uint *)(iVar2 + 0x110) == param_1) {
        *(undefined4 *)(iVar2 + 0x110) = 0xffffffff;
        *(undefined1 *)(iVar2 + 0x136) = 0;
        *(undefined1 *)(iVar2 + 0x135) = 0;
      }
      iVar2 = data_iterator_next();
    }
    squad_clear_unit_references(param_1,'\x01');
    sVar3 = 0;
    if (0 < *(short *)(iVar4 + 0x8b8)) {
      do {
        puVar1 = (uint *)(iVar4 + 0x8bc + sVar3 * 4);
        if (*puVar1 == param_1) {
          *(short *)(iVar4 + 0x8b8) = *(short *)(iVar4 + 0x8b8) + -1;
          if (0 < *(short *)(iVar4 + 0x8b8)) {
            *puVar1 = *(uint *)(iVar4 + 0x8bc + *(short *)(iVar4 + 0x8b8) * 4);
          }
        }
        sVar3 = sVar3 + 1;
      } while (sVar3 < *(short *)(iVar4 + 0x8b8));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
