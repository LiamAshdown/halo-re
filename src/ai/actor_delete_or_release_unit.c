// actor_delete_or_release_unit  (Ghidra: actor_delete_or_release_unit, already named)
// address 0x4288e0, size 215 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (checked against objdump 0x4288e0..0x4289b2)
// evidence: types/ai.h actor.swarm(0x06)/unit_index(0x18)/cluster_unit_index(0x24). Calls
//   actor_remove_from_unit_cluster (0x427c90) and actor_attempt_grenade_throw (0x428ab0),
//   both already rewritten in this module, plus actor_delete (0x427e60, already rewritten)
//   and five object_delete* helpers, none established elsewhere in this repo.
//   UNSURE: `is_dead` is Ghidra's untraced "in_AL" -- present in both the lone-unit and the
//   swarm branch, with no register write in between in either branch, so it is modeled as a
//   genuine second parameter rather than actor_attempt_grenade_throw's return value, but this
//   was not independently confirmed with objdump. The five object_delete* callees' exact
//   signatures are guessed from their call-site argument counts alone.
// register convention: stack -> actor_index, EAX (AL) -> is_dead.
//   // blam-cc: stack -> actor_index, EAX -> is_dead (not independently re-verified)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60
extern uint8_t actor_attempt_grenade_throw(datum_index actor_index); // 0x428ab0, in this rewrite range, not yet written when this file was authored
extern void object_delete_recursive(datum_index object_index, uint32_t flag); // 0x4f59d0, UNSURE signature
extern void object_delete_unparented(datum_index object_index); // 0x4f5aa0, UNSURE signature
extern void object_delete(datum_index object_index); // 0x4f5bd0, UNSURE signature
extern void object_delete_4f9030(datum_index object_index, uint32_t flag); // 0x4f9030, UNSURE signature

// blam-cc: stack -> actor_index, EAX -> is_dead
// Deletes or detaches the game object(s) tied to an actor: for a lone unit, tries a grenade
// throw first, then either fully recursive-deletes it (when is_dead) or does a plain
// object_delete; for a swarm, walks every unit in the cluster, detaching each and deleting
// it (recursively and unconditionally when is_dead, otherwise via object_delete_unparented
// or object_delete_recursive depending on the unit's own network role), finally deleting the
// actor itself once the cluster is empty.
void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm == 0) {
        datum_index unit_index = self->unit_index;
        actor_attempt_grenade_throw(actor_index);
        if (is_dead != 0) {
            object_delete_recursive(unit_index, 0);
            object_delete_4f9030(unit_index, 0);
            return;
        }
        object_delete(unit_index);
        return;
    }

    for (;;) {
        datum_index unit_index = self->cluster_unit_index;
        if (unit_index == (datum_index)k_datum_index_none) {
            actor_delete(actor_index, 1);
            return;
        }
        actor_remove_from_unit_cluster(actor_index, unit_index);
        if (is_dead == 0) {
            object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
            int32_t network_role = unit_object->network_role;
            if (network_role == 0) {
                object_delete_unparented(unit_index);
                object_delete_recursive(unit_index, 0);
            } else if (network_role == 3) {
                object_delete_recursive(unit_index, 0);
            }
        } else {
            object_delete_recursive(unit_index, 0);
            object_delete_4f9030(unit_index, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4288e0):

void actor_delete_or_release_unit(uint param_1)

{
  uint uVar1;
  undefined4 uVar2;
  char in_AL;
  int iVar3;
  int iVar4;

  iVar3 = (param_1 & 0xffff) * 0x724;
  iVar4 = iVar3 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar3 + 6 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    uVar2 = *(undefined4 *)(iVar4 + 0x18);
    actor_attempt_grenade_throw(param_1);
    if (in_AL != '\0') {
      object_delete_recursive(uVar2,0);
      object_delete_4f9030(uVar2,0);
      return;
    }
    object_delete();
    return;
  }
  uVar1 = *(uint *)(iVar4 + 0x24);
  do {
    if (uVar1 == 0xffffffff) {
      actor_delete(1);
      return;
    }
    actor_remove_from_unit_cluster(uVar1);
    if (in_AL == '\0') {
      iVar3 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 4);
      if (iVar3 == 0) {
        object_delete_unparented();
      }
      else if (iVar3 != 3) goto LAB_0042896d;
      object_delete_recursive(uVar1,0);
    }
    else {
      object_delete_recursive(uVar1,0);
      object_delete_4f9030(uVar1,0);
    }
LAB_0042896d:
    uVar1 = *(uint *)(iVar4 + 0x24);
  } while( true );
}
#endif
