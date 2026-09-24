// encounter_release_stale_props  (Ghidra: encounter_release_stale_props; named for this rewrite)
// address 0x4382b0, size 303 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: phase-4 summary ("detaches and frees stale shared-group references held by a
//   squad's actors"); it walks the encounter member list and, for every prop on
//   actor.first_prop (types/ai.h: "head of the prop list, chained through prop.next_in_actor
//   at +0x08") whose kind is 4 or 5 (the shared / vault kinds) and which is engaged but is
//   not the actor's current target, unlinks it (actor_unlink_prop @0x43ea20) and deletes the
//   datum. It also resets the encounter's retreat latch (+0x42 = 1, +0x4c = 0) and flushes
//   the recently-seen-object ring.
// register convention: EAX -> encounter_index; no stack arguments.
//   // blam-cc: EAX -> encounter_index
//
// The `props[p->pair_index].pair_index = none` write clears the partner prop's back pointer
// before this prop is deleted (types/ai.h prop.pair_index, "the paired prop allocated by
// 0x43e910 / 0x43e980").
// actor_replace_object_reference (0x428470) and actor_unlink_prop (0x43ea20) are both shown
// argument-less or short by Ghidra here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data; // 0x008802c8
extern ai_globals *ai_global_data; // 0x00880354
extern data_array *prop_data;      // 0x008802c0
extern data_array *actor_data;     // 0x00880360

extern void actor_replace_object_reference(datum_index actor_index); // 0x428470, not yet rewritten
extern void squad_recent_object_list_clear(datum_index encounter_index); // 0x436c10
extern void actor_unlink_prop(datum_index prop_index);               // 0x43ea20
extern void datum_delete(data_array *array, datum_index handle);     // 0x4d0510

// blam-cc: EAX -> encounter_index
void encounter_release_stale_props(datum_index encounter_index)
{
    encounter *enc;
    actor *a;
    prop *p;
    prop *props;
    datum_index actor_index;
    datum_index current;
    datum_index prop_index;
    datum_index next_prop;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    enc->unknown_42 = 1;
    enc->unknown_4c = 0;
    squad_recent_object_list_clear(encounter_index);

    actor_index = (datum_index)k_datum_index_none;
    if (ai_global_data->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = ai_global_data->unknown_08;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (ai_global_data->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        actor_index = a->next_in_encounter;

        next_prop = a->first_prop;
        while (next_prop != (datum_index)k_datum_index_none) {
            props = (prop *)prop_data->data;
            prop_index = next_prop;
            p = &props[prop_index & 0xffff];
            next_prop = p->next_in_actor;

            if (3 < p->kind && p->kind < 6 && p->is_unit != 0 &&
                prop_index != a->target_unit_index) {
                props[p->pair_index & 0xffff].pair_index = (datum_index)k_datum_index_none;
                actor_replace_object_reference(current);
                actor_unlink_prop(prop_index);
                datum_delete(prop_data, prop_index);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4382b0):

void FUN_004382b0(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint in_EAX;
  int iVar5;
  uint uVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  uint local_4;

  iVar2 = DAT_008802c8;
  iVar9 = (in_EAX & 0xffff) * 0x6c;
  iVar5 = *(int *)(DAT_008802c8 + 0x34) + iVar9;
  *(undefined1 *)(iVar5 + 0x42) = 1;
  *(undefined2 *)(iVar5 + 0x4c) = 0;
  squad_recent_object_list_clear();
  iVar5 = DAT_008802c0;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (in_EAX == 0xffffffff) {
      local_4 = *(uint *)(DAT_00880354 + 8);
    }
    else {
      local_4 = *(uint *)(*(int *)(iVar2 + 0x34) + 0x14 + iVar9);
    }
  }
  while ((uVar6 = local_4, *(char *)(DAT_00880354 + 1) != '\0' && (uVar6 != 0xffffffff))) {
    iVar2 = *(int *)(DAT_00880360 + 0x34);
    iVar9 = (uVar6 & 0xffff) * 0x724;
    local_4 = *(uint *)(iVar9 + 0x2c + iVar2);
    uVar4 = *(uint *)((uVar6 & 0xffff) * 0x724 + 0x50 + iVar2);
    while (uVar8 = uVar4, uVar8 != 0xffffffff) {
      iVar3 = *(int *)(iVar5 + 0x34);
      iVar7 = (uVar8 & 0xffff) * 0x138;
      sVar1 = *(short *)(iVar7 + 0x24 + iVar3);
      uVar4 = *(uint *)(iVar7 + 8 + iVar3);
      if ((((3 < sVar1) && (sVar1 < 6)) && (*(char *)(iVar7 + iVar3 + 0x60) != '\0')) &&
         (uVar8 != *(uint *)(iVar9 + iVar2 + 0x270))) {
        *(undefined4 *)((*(uint *)(iVar7 + iVar3 + 0xc) & 0xffff) * 0x138 + 0xc + iVar3) =
             0xffffffff;
        actor_replace_object_reference(uVar6);
        FUN_0043ea20();
        iVar5 = DAT_008802c0;
        datum_delete();
      }
    }
  }
  return;
}
#endif
