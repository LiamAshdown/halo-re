// actor_new_and_attach_to_unit  (Ghidra: actor_new_and_attach_to_unit, already named)
// address 0x426ac0, size 553 bytes
// name confidence: 0.55   rewrite confidence: 0.85
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

// REWRITTEN from objdump 0x426ac0..0x426ce8. The draft called object_try_and_get(1) -- the literal 1 as the
//   object handle -- so every non-swarm placement failed and actor_place_new_unit deleted the new unit (the
//   a10 crewmen placed by ai_place vanished on spawn). The binary checks the unit itself (ECX unit, mask 1)
//   and refuses one whose +0x106 has bit 2. Also: encounter_add_actor takes the squad argument (DX), the
//   small-table lookup takes unknown_60 (CX), the reuse scan starts at the iterator cursor's third dword.
// blam-cc: stack -> the twelve arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *encounter_data;  // 0x008802c8
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern void *actor_type_procs[16];  // 0x006853b8

extern datum_index actor_new(datum_index actor_variant_tag); // 0x426760, stack
extern void actor_attach_to_unit(datum_index actor_index, datum_index unit_index); // 0x427560, stack
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, EAX, BL
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60, EBX, stack
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern int32_t actor_lookup_small_table_entry(int16_t index); // 0x40e790, CX
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, EAX, ECX
extern void ai_actor_link_to_unassigned_list(datum_index actor_index); // 0x436940, EAX
extern void encounter_add_actor(int16_t squad_index, datum_index actor_index, datum_index encounter_index,
    uint8_t keep_team); // 0x436770, DX, stack
extern uint8_t actor_link_to_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x4279f0

#define ACTOR_AT(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)

datum_index actor_new_and_attach_to_unit(
    char reuse_existing,           // swarm actors: join a compatible existing actor instead of a new one
    datum_index unit_index,        // the unit object to attach to
    datum_index actor_variant_tag,
    uint32_t encounter_or_none,    // an encounter index (salt added here when missing) or -1
    int16_t squad_index,
    char ignore_squad,             // skip the squad match in the reuse scan
    datum_index exclude_actor,     // an actor the reuse scan must not pick
    char start_active,
    uint16_t unknown_60,
    int16_t unknown_62,
    uint16_t unknown_90,
    uint8_t unknown_68)
{
    datum_index actor_index = k_datum_index_none;
    uint8_t *self;

    if (unit_index == k_datum_index_none || actor_variant_tag == k_datum_index_none) {
        return k_datum_index_none;
    }

    if (reuse_existing != 0) {
        datum_index cursor[4];
        datum_index candidate;

        ai_reference_actor_iterator_init_cursor((int32_t)encounter_or_none, cursor);
        candidate = cursor[2];
        while (ai_globals_ptr->actors_valid != 0 && candidate != k_datum_index_none) {
            uint8_t *actor = ACTOR_AT(candidate);

            actor_index = candidate;
            candidate = ((struct actor *)actor)->next_in_encounter;
            if (actor[6] == 0 || actor_index == exclude_actor || ((struct actor *)actor)->cluster_count >= 0x10 ||
                ((struct actor *)actor)->actor_variant_tag != actor_variant_tag ||
                (ignore_squad == 0 && ((struct actor *)actor)->squad_index != squad_index)) {
                continue;
            }
            goto attach;
        }
    } else {
        uint8_t *unit = (uint8_t *)object_try_and_get(unit_index, 1);

        if (unit == 0 || (unit[0x106] & 4) != 0) {
            return k_datum_index_none;
        }
    }

    actor_index = actor_new(actor_variant_tag);
    if (actor_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    self = ACTOR_AT(actor_index);
    if (encounter_or_none == (uint32_t)k_datum_index_none) {
        ai_actor_link_to_unassigned_list(actor_index);
    } else {
        if ((encounter_or_none & 0xffff0000) == 0) {
            uint8_t *enc = (uint8_t *)encounter_data->data + (encounter_or_none & 0xffff) * 0x6c;

            encounter_or_none = ((uint32_t)(int32_t)*(int16_t *)enc << 0x10) | (encounter_or_none & 0xffff);
        }
        encounter_add_actor(squad_index, actor_index, encounter_or_none, 0);
    }
    if (start_active == 0) {
        *(int16_t *)(self + 0x6a) = 2;
    } else {
        *(int16_t *)(self + 0x6a) = 0;
        if (self[8] != 0) {
            actor_set_units_active(actor_index, 0);
        }
    }
    *(uint16_t *)(self + 0x60) = unknown_60;
    *(int16_t *)(self + 0x62) = unknown_62;
    if (unknown_62 == -1 || unknown_62 == 0) {
        *(int16_t *)(self + 0x62) = (int16_t)actor_lookup_small_table_entry((int16_t)unknown_60);
    }
    self[0x68] = unknown_68;
    self[0x8e] = 0;
    *(int16_t *)(self + 0x92) = 2;
    *(uint16_t *)(self + 0x90) = unknown_90;
    if (self[6] != ((uint8_t *)actor_type_procs[*(int16_t *)(self + 4)])[0xd]) {
        actor_delete(actor_index, 0);
        return k_datum_index_none;
    }

attach:
    if (reuse_existing == 0) {
        actor_attach_to_unit(actor_index, unit_index);
        return actor_index;
    }
    if (actor_link_to_unit_cluster(actor_index, unit_index) != 0) {
        return actor_index;
    }
    if (*(int16_t *)(ACTOR_AT(actor_index) + 0x1e) == 0) {
        actor_delete(actor_index, 0);
    }
    return k_datum_index_none;
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
