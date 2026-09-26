// actor_new_and_attach_to_unit  (Ghidra: actor_new_and_attach_to_unit, already named)
// address 0x426ac0, size 553 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: types/ai.h actor.swarm(0x06)/cluster_count(0x1e)/actor_variant_tag(0x5c)/
//   squad_index(0x3a)/next_in_encounter(0x2c)/active(0x08)/type(0x04)/awareness_level(0x6a)/
//   unknown_60/unknown_62/unknown_68/unknown_8e/unknown_90/unknown_92; actor_type_table_entry
//   (0x006853b8 table). Calls actor_new (0x426760), actor_attach_to_unit (0x427560),
//   actor_set_units_active (0x427860), actor_delete (0x427e60), object_try_and_get
//   (0x4f6ec0), all already established, plus actor_link_to_unit_cluster/encounter_add_actor/ai_actor_link_to_unassigned_list/
//   actor_lookup_small_table_entry/ai_reference_actor_iterator_init_cursor, none of which are rewritten yet (the last two already have
//   UNSURE externs elsewhere in this module).
// register convention: Ghidra already resolved all twelve parameters as ordinary
//   parameters; kept in that order rather than re-derived through disassembly, given the
//   scope of this rewrite pass. See per-parameter names below for the inferred role of each.
//   UNSURE: this function's true register/stack split was not independently re-verified with
//   objdump, unlike most of this rewrite; Ghidra's own arity here (12 concrete parameters)
//   is a much stronger signal than the "()"-truncated calls seen elsewhere in this module, so
//   it was trusted directly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *encounter_data;  // 0x008802c8
extern ai_globals *ai_globals_ptr;      // 0x00880354
extern void *actor_type_procs[16];  // 0x006853b8

extern datum_index actor_new(datum_index actor_variant_tag); // 0x426760
extern void actor_attach_to_unit(datum_index actor_index, datum_index unit_index); // 0x427560, UNSURE signature
extern void actor_set_units_active(datum_index actor_index, uint8_t activate); // 0x427860, blam-cc: EAX, BL
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60
extern void *object_try_and_get(int32_t kind); // 0x4f6ec0
extern int32_t actor_lookup_small_table_entry(int16_t index); // 0x40e790, UNSURE which index this call site passes
extern datum_index ai_reference_actor_iterator_init_cursor(void); // 0x4369f0
    // UNSURE: returns the head of the unassigned actor list per types/ai.h ai_globals+0x08;
    // ai_actor_link_to_unassigned_list at 0x436940 is its sibling.
extern void ai_actor_link_to_unassigned_list(void); // 0x436940, UNSURE signature, not in this rewrite range
extern void encounter_add_actor(int16_t squad_index, datum_index actor_index,
    datum_index encounter_index, uint8_t keep_team); // 0x436770, blam-cc: DX -> squad_index
    // UNSURE: the squad index arrives in DX and Ghidra did not attribute it to this call
    // site, so the actor's current squad_index is passed; encounter_add_actor writes it
    // straight back into the same field.
extern char actor_link_to_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x4279f0, in this rewrite range (actor_link_to_unit_cluster), not yet written when this file was authored

// Creates a brand-new actor (or, when reuse_existing is set, reuses a compatible existing
// squad member instead) and binds it to a newly placed unit, or deletes it again on failure.
// UNSURE: parameter roles below are inferred from usage, not independently confirmed.
datum_index actor_new_and_attach_to_unit(
    char reuse_existing,           // param_1: 0 = always create a new actor; else scan for a reusable one
    datum_index unit_index,        // param_2: the unit object to attach to
    datum_index actor_variant_tag, // param_3: tag passed to actor_new / matched against actor.actor_variant_tag
    uint32_t encounter_or_none,    // param_4: an encounter datum_index, or a raw next_in_encounter value already carrying its salt
    int16_t squad_index,           // param_5: required actor.squad_index unless ignore_squad is set
    char ignore_squad,             // param_6: skip the squad_index match in the reuse scan
    datum_index exclude_actor,     // param_7: an actor index the reuse scan must not pick
    char start_active,             // param_8: 0 = leave inactive (awareness 2); else awareness 0 and activate if already active
    uint16_t unknown_60,           // param_9
    int16_t unknown_62,            // param_10: falls back to actor_lookup_small_table_entry() when -1 or 0
    uint16_t unknown_90,           // param_11
    uint8_t unknown_68)            // param_12
{
    datum_index actor_index;
    actor *self;

    if (unit_index == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }
    if (actor_variant_tag == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    if (reuse_existing == 0) {
    create_new:
        if (object_try_and_get(1) == 0) {
            return (datum_index)k_datum_index_none;
        }
        // UNSURE: the vitality-flags-bit-4 gate above is read from the local player object
        // (object_try_and_get(1)) but its result pointer is otherwise unused here; kept
        // exactly as the original, which discards the pointer after the flag test.
        goto make_actor;

    make_actor:
        actor_index = actor_new(actor_variant_tag);
        if (actor_index == (datum_index)k_datum_index_none) {
            return (datum_index)k_datum_index_none;
        }
        self = &((actor *)actor_data->data)[actor_index & 0xffff];

        if (encounter_or_none == (uint32_t)k_datum_index_none) {
            ai_actor_link_to_unassigned_list();
        } else {
            if ((encounter_or_none & 0xffff0000) == 0) {
                encounter *enc = &((encounter *)encounter_data->data)[encounter_or_none & 0xffff];
                encounter_or_none = ((uint32_t)enc->identifier << 0x10) | (encounter_or_none & 0xffff);
            }
            encounter_add_actor(self->squad_index, actor_index, encounter_or_none, 0);
        }

        if (start_active == 0) {
            self->awareness_level = 2;
        } else {
            self->awareness_level = 0;
            if (self->active != 0) {
                actor_set_units_active(actor_index, 0);
            }
        }

        self->unknown_60 = unknown_60;
        self->unknown_62 = unknown_62;
        if (unknown_62 == -1 || unknown_62 == 0) {
            self->unknown_62 = (int16_t)actor_lookup_small_table_entry(self->type); // UNSURE index
        }
        self->unknown_68 = unknown_68;
        self->unknown_8e = 0;
        self->unknown_92 = 2;
        self->unknown_90 = unknown_90;

        {
            actor_type_table_entry *type_entry = (actor_type_table_entry *)actor_type_procs[self->type];
            if (self->swarm != ((uint8_t *)type_entry)[0xd]) { // UNSURE: type_entry+0xd, not individually named
                goto delete_and_fail;
            }
        }
    } else {
        datum_index candidate = ai_reference_actor_iterator_init_cursor();

        for (;;) {
            actor_index = candidate;
            if (ai_globals_ptr->actors_valid == 0 || actor_index == (datum_index)k_datum_index_none) {
                goto make_actor;
            }
            self = &((actor *)actor_data->data)[actor_index & 0xffff];
            candidate = self->next_in_encounter;

            if (self->swarm != 0 &&
                actor_index != exclude_actor &&
                self->cluster_count <= 0xf &&
                self->actor_variant_tag == actor_variant_tag &&
                (ignore_squad != 0 || self->squad_index == squad_index)) {
                break;
            }
        }
        if (actor_index == (datum_index)k_datum_index_none) {
            goto make_actor;
        }
    }

    if (reuse_existing == 0) {
        actor_attach_to_unit(actor_index, unit_index);
        return actor_index;
    }

    if (actor_link_to_unit_cluster(actor_index, unit_index) != 0) {
        return actor_index;
    }
    self = &((actor *)actor_data->data)[actor_index & 0xffff];
    if (self->cluster_count != 0) {
        return (datum_index)k_datum_index_none;
    }

delete_and_fail:
    actor_delete(actor_index, 0);
    return (datum_index)k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x426ac0):

uint actor_new_and_attach_to_unit
               (char param_1,int param_2,int param_3,uint param_4,short param_5,char param_6,
               uint param_7,char param_8,undefined2 param_9,short param_10,undefined2 param_11,
               undefined1 param_12)

{
  char cVar1;
  undefined2 uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint local_4;

  if (param_2 == -1) {
    return 0xffffffff;
  }
  if (param_3 == -1) {
    return 0xffffffff;
  }
  if (param_1 == '\0') {
    iVar3 = object_try_and_get(1);
    if (iVar3 == 0) {
      return 0xffffffff;
    }
    if ((*(byte *)(iVar3 + 0x106) & 4) != 0) {
      return 0xffffffff;
    }
LAB_00426b6a:
    uVar5 = actor_new(param_3);
    if (uVar5 == 0xffffffff) {
      return 0xffffffff;
    }
    iVar3 = (uVar5 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    if (param_4 == 0xffffffff) {
      FUN_00436940();
    }
    else {
      if ((param_4 & 0xffff0000) == 0) {
        param_4 = (int)*(short *)((param_4 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34)) << 0x10
                  | param_4 & 0xffff;
      }
      FUN_00436770(uVar5,param_4,0);
    }
    if (param_8 == '\0') {
      *(undefined2 *)(iVar3 + 0x6a) = 2;
    }
    else {
      *(undefined2 *)(iVar3 + 0x6a) = 0;
      if (*(char *)(iVar3 + 8) != '\0') {
        actor_set_units_active();
      }
    }
    *(undefined2 *)(iVar3 + 0x60) = param_9;
    *(short *)(iVar3 + 0x62) = param_10;
    if ((param_10 == -1) || (param_10 == 0)) {
      uVar2 = FUN_0040e790();
      *(undefined2 *)(iVar3 + 0x62) = uVar2;
    }
    *(undefined1 *)(iVar3 + 0x68) = param_12;
    *(undefined1 *)(iVar3 + 0x8e) = 0;
    *(undefined2 *)(iVar3 + 0x92) = 2;
    *(undefined2 *)(iVar3 + 0x90) = param_11;
    if (*(char *)(iVar3 + 6) != (&PTR_PTR_006853b8)[*(short *)(iVar3 + 4)][0xd]) goto LAB_00426cb8;
  }
  else {
    FUN_004369f0();
    do {
      uVar5 = local_4;
      if ((*(char *)(DAT_00880354 + 1) == '\0') || (uVar5 == 0xffffffff)) goto LAB_00426b6a;
      iVar3 = (uVar5 & 0xffff) * 0x724;
      iVar4 = iVar3 + *(int *)(DAT_00880360 + 0x34);
      local_4 = *(uint *)(iVar4 + 0x2c);
    } while ((*(char *)(iVar3 + 6 + *(int *)(DAT_00880360 + 0x34)) == '\0') ||
            ((((uVar5 == param_7 || (0xf < *(short *)(iVar4 + 0x1e))) ||
              (*(int *)(iVar4 + 0x5c) != param_3)) ||
             ((param_6 == '\0' && (*(short *)(iVar4 + 0x3a) != param_5))))));
    if (uVar5 == 0xffffffff) goto LAB_00426b6a;
  }
  if (param_1 == '\0') {
    actor_attach_to_unit(uVar5,param_2);
    return uVar5;
  }
  cVar1 = FUN_004279f0(uVar5,param_2);
  if (cVar1 != '\0') {
    return uVar5;
  }
  if (*(short *)((uVar5 & 0xffff) * 0x724 + 0x1e + *(int *)(DAT_00880360 + 0x34)) != 0) {
    return 0xffffffff;
  }
LAB_00426cb8:
  actor_delete(0);
  return 0xffffffff;
}
#endif
