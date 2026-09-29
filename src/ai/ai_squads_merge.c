// ai_squads_merge  (Ghidra: ai_squads_merge; named for this rewrite)
// address 0x433590, size 982 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN from objdump 0x433590..0x433965; hs ai_migrate worker)
// evidence: phase-4 summary ("merges/reassigns live squad members from one squad into
// another, matching by actor_variant, transferring leader status and starting-location
// ownership") matches the overall shape: builds a 64-entry remap table (source squad index
// -> best matching target squad index, via ai_squad_find_best_matching_member, 0x4333d0,
// this batch) for every squad of the source encounter, then walks three different actor
// chains (the global unassigned-actor list, every actor of the source encounter via
// actor_iterator_next filtered on encounter+0x44, and the platoon-designer member records)
// reassigning each actor/record whose squad maps to something other than itself onto the
// target encounter/squad. This function has ZERO callers in this build (out/functions.json),
// so it was given a lower-rigor pass than the rest of this batch: the two encounter/squad
// index parameters (EDX = source encounter, param_1 = target encounter, both packed the
// same way the sibling ai_reference_* functions decode) are kept, but this function's own
// implicit EAX (needed to prime ai_reference_squad_iterator_new/_next for the source
// encounter, called here with zero visible arguments) is assumed equal to the EDX value
// rather than independently confirmed, and the two post-remap walks are transliterated
// close to the raw offsets rather than fully re-derived, since the local Ghidra names
// (`local_94`, `iVar5`) visibly alias the same stack slots across unrelated loops.
// register convention: UNSURE overall; Ghidra resolved param_1/param_2/param_3 as ordinary
// parameters and left the source-encounter reference in EDX unresolved.
//   // blam-cc: EDX -> source_reference (also assumed to feed EAX for the squad iterator
//   calls), stack -> target_encounter_index, notify, is_platoon_merge (param_2/param_3 roles
//   guessed from their use in ai_communication_broadcast and the global_structure_bsp_index compare)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *encounter_data;  // 0x008802c8
extern Scenario *global_scenario;   // 0x00746f8c
extern tag_instance *tag_instances; // 0x0087bc14
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern data_array *actor_data;      // 0x00880360
extern int16_t global_structure_bsp_index;        // 0x0069e8d8, UNSURE: a designer-slot/leader sentinel value

extern void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator); // 0x4324f0, this batch
extern encounter_squad_state *ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator); // 0x4325b0, this batch
extern int32_t ai_squad_find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index,
    uint8_t *requested_actor_data, uint8_t *requested_actor_variant_data, char match_by_index); // 0x4333d0, this batch
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, EAX, ECX
extern void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only); // 0x436a30, this batch
extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70, this batch
extern void ai_actor_unlink_from_unassigned_list(datum_index actor_index); // 0x436990, this batch
extern void encounter_add_actor(int16_t squad_index, datum_index actor_index,
    datum_index encounter_index, uint8_t keep_team); // 0x436770, blam-cc: DX -> squad_index
    // UNSURE: the squad index arrives in DX and Ghidra did not attribute it to this call
    // site, so the actor's current squad_index is passed; encounter_add_actor writes it
    // straight back into the same field.
extern void encounters_recompute_dirty(void); // 0x435f00, this batch
extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index,
    int16_t squad_index); // 0x4290f0, EAX, EBX, stack
extern void ai_recompute_all_relationship_flags(void); // 0x42bbb0, outside this rewrite's range, UNSURE signature
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, already established elsewhere

// REWRITTEN from objdump. hs ai_migrate (0x47d826) passes (EDX = source ai reference, stack = target reference, 0, 0).
//   1. For every squad the source reference names that is populated (state +0x18 > 0) or whose encounter has
//      +0x1e set, remap[squad] = ai_squad_find_best_matching_member(EAX = the RAW target reference, squad, actor
//      tag data, actor variant data, source == target).
//   2. Every member of the source encounter (init_cursor, cursor[2], chained through +0x2c) whose remapped squad
//      is not -1 (and differs, when merging into itself) is moved with actor_reset_squad_link_for_type_change(
//      EAX = actor, EBX = target encounter, remapped squad). When notify is set and the actor has a unit, the
//      0x17 (or 0x16 for is_platoon_merge) communication is broadcast.
//   3. If the source encounter +0x1e is set, every actor whose +0x44/+0x48 (a secondary encounter/squad) points
//      at the source is remapped too, and +0x1e moves to the target unless merging into itself.
//   4. Unassigned actors (ai globals +0x08 list) whose +0x30/+0x38 encounter/squad is the source are
//      remapped. When not merging into itself and the target encounter's BSP (+0x7e) is the current one, they
//      are unlinked and encounter_add_actor(DX = the new +0x38 squad, actor, target, 1) is called.
//   5. A team change recomputes the relationship flags, then encounters_recompute_dirty.
//   The draft walked the unassigned list in step 2, dropped the actor/encounter arguments of the squad move,
//   masked the reference handed to find_best_matching_member, and used +0x3a as the squad in step 4.
// blam-cc: EDX -> source_reference, stack -> target_encounter_index, notify, is_platoon_merge
void ai_squads_merge(uint32_t source_reference, uint32_t target_encounter_index, char notify, char is_platoon_merge)
{
    uint32_t target_reference = target_encounter_index;
    uint32_t source_index;
    uint32_t target_index;
    encounter *target_enc;
    encounter *source_enc;
    ScenarioEncounter *source_definition;
    ScenarioEncounter *target_definition;
    int16_t remap[64];
    ai_reference_squad_iterator iterator;
    encounter_squad_state *state;
    datum_index cursor[3];
    datum_index actor_index;
    uint8_t merging_into_self;
    int32_t i;

    if (source_reference == (uint32_t)k_datum_index_none || target_reference == (uint32_t)k_datum_index_none) {
        return;
    }
    source_index = source_reference & 0xffff;
    target_index = target_reference & 0xffff;

    target_enc = &((encounter *)encounter_data->data)[target_index];
    source_enc = &((encounter *)encounter_data->data)[source_index];
    target_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[target_index];
    source_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[source_index];
    merging_into_self = (uint8_t)(source_index == target_index);

    for (i = 0; i < 64; i++) {
        remap[i] = -1;
    }

    // 1. squad remap table
    ai_reference_squad_iterator_new(source_reference, &iterator);
    for (state = ai_reference_squad_iterator_next(&iterator); state != 0;
         state = ai_reference_squad_iterator_next(&iterator)) {
        uint8_t *squad;
        int16_t palette_index;
        uint8_t *variant_data = 0;
        uint8_t *actor_tag_data = 0;

        if (!(state->weighted_actor_count > 0) && source_enc->unknown_1e[0] == 0) {
            continue;
        }
        squad = *(uint8_t **)&((struct ScenarioEncounter *)source_definition)->squads.pointer + iterator.cursor * 0xe8;
        palette_index = *(int16_t *)(squad + 0x20);
        if (palette_index >= 0 && (int32_t)palette_index < *(int32_t *)((uint8_t *)global_scenario + 0x420)) {
            uint8_t *entry = *(uint8_t **)((uint8_t *)global_scenario + 0x424) + palette_index * 0x10;
            datum_index variant_tag = *(datum_index *)(entry + 0xc);

            if (variant_tag != (datum_index)k_datum_index_none &&
                tag_instances[(int16_t)variant_tag].group_tag == 0x61637476 /* 'actv' */) {
                datum_index actor_tag;

                variant_data = (uint8_t *)tag_instances[variant_tag & 0xffff].data;
                actor_tag = *(datum_index *)(variant_data + 0x10);
                if (actor_tag != (datum_index)k_datum_index_none) {
                    actor_tag_data = (uint8_t *)tag_instances[actor_tag & 0xffff].data;
                }
            }
        }
        remap[iterator.cursor] = (int16_t)ai_squad_find_best_matching_member(target_reference, (int16_t)iterator.cursor,
            actor_tag_data, variant_data, (char)merging_into_self);
    }

    // 2. members of the source encounter
    ai_reference_actor_iterator_init_cursor((int32_t)source_index, cursor);
    actor_index = cursor[2];
    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
        datum_index current = actor_index;
        int16_t squad_index = a->squad_index;
        int16_t remapped = remap[squad_index];

        actor_index = a->next_in_encounter;
        if (remapped == -1 || (merging_into_self && remapped == squad_index)) {
            continue;
        }
        actor_reset_squad_link_for_type_change(current, (datum_index)target_index, remapped);
        if (notify != 0) {
            datum_index unit_index = ((actor *)actor_data->data)[current & 0xffff].unit_index;

            if (unit_index != (datum_index)k_datum_index_none) {
                ai_communication_broadcast(is_platoon_merge != 0 ? 0x16 : 0x17, unit_index,
                    (datum_index)k_datum_index_none, -1, (datum_index)k_datum_index_none,
                    (datum_index)k_datum_index_none, 0);
            }
        }
    }

    // 3. secondary encounter/squad references
    if (source_enc->unknown_1e[0] != 0) {
        actor_iterator_state all;
        actor *a;

        actor_iterator_new(&all, 0);
        while ((a = actor_iterator_next(&all)) != 0) {
            uint8_t *raw = (uint8_t *)a;
            int16_t squad_index;
            int16_t remapped;

            if ((*(uint32_t *)(raw + 0x44) & 0xffff) != source_index) {
                continue;
            }
            squad_index = *(int16_t *)(raw + 0x48);
            remapped = remap[squad_index];
            if (remapped == -1 || (merging_into_self && remapped == squad_index)) {
                continue;
            }
            *(uint32_t *)(raw + 0x44) = target_index;
            *(int16_t *)(raw + 0x48) = remapped;
        }
        if (!merging_into_self) {
            source_enc->unknown_1e[0] = 0;
            target_enc->unknown_1e[0] = 1;
        }
    }

    // 4. unassigned actors that name the source encounter
    actor_index = ai_globals_ptr->actors_valid != 0 ? ai_globals_ptr->unknown_08 : (datum_index)k_datum_index_none;
    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        datum_index current = actor_index;
        uint8_t *raw = (uint8_t *)&((actor *)actor_data->data)[current & 0xffff];
        int16_t squad_index;
        int16_t remapped;

        actor_index = ((struct actor *)raw)->next_in_encounter;
        if ((*(uint32_t *)&((struct actor *)raw)->unknown_30 & 0xffff) != source_index) {
            continue;
        }
        squad_index = ((struct actor *)raw)->unknown_38;
        remapped = remap[squad_index];
        if (remapped == -1 || (merging_into_self && remapped == squad_index)) {
            continue;
        }
        *(uint32_t *)&((struct actor *)raw)->unknown_30 = target_index;
        ((struct actor *)raw)->unknown_38 = remapped;
        if (merging_into_self || *(int16_t *)&((struct ScenarioEncounter *)target_definition)->precomputed_bsp_index != global_structure_bsp_index) {
            continue;
        }
        ai_actor_unlink_from_unassigned_list(current);
        encounter_add_actor(((struct actor *)raw)->unknown_38, current, ((struct actor *)raw)->unknown_30, 1);
    }

    // 5.
    if (source_enc->team != target_enc->team) {
        ai_recompute_all_relationship_flags();
    }
    encounters_recompute_dirty();
}

#if 0
Original Ghidra decompilation (0x433590):

void FUN_00433590(uint param_1,char param_2,char param_3)

{
  short sVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  uint in_EDX;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  short *psVar10;
  undefined4 uVar11;
  uint uVar12;
  bool bVar13;
  uint local_94;
  short local_80 [64];

  if ((in_EDX != 0xffffffff) && (param_1 != 0xffffffff)) {
    uVar9 = in_EDX & 0xffff;
    param_1 = param_1 & 0xffff;
    if ((uVar9 != 0xffffffff) && (param_1 != 0xffffffff)) {
      iVar7 = param_1 * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      iVar6 = uVar9 * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      iVar2 = *(int *)(global_scenario + 0x430);
      bVar13 = uVar9 == param_1;
      psVar10 = local_80;
      for (iVar5 = 0x20; iVar5 != 0; iVar5 = iVar5 + -1) {
        psVar10[0] = -1;
        psVar10[1] = -1;
        psVar10 = psVar10 + 2;
      }
      FUN_004324f0();
      iVar5 = FUN_004325b0();
      while (iVar5 != 0) {
        if ((0 < *(short *)(iVar5 + 0x18)) || (*(char *)(iVar6 + 0x1e) != '\0')) {
          sVar3 = *(short *)(local_94 * 0xe8 + *(int *)(uVar9 * 0xb0 + iVar2 + 0x84) + 0x20);
          iVar5 = 0;
          uVar11 = 0;
          if ((-1 < sVar3) &&
             ((((int)sVar3 < *(int *)(global_scenario + 0x420) &&
               (uVar12 = *(uint *)(sVar3 * 0x10 + *(int *)(global_scenario + 0x424) + 0xc),
               uVar12 != 0xffffffff)) &&
              (*(int *)(DAT_0087bc14 + (short)uVar12 * 0x20) == 0x61637476)))) {
            iVar5 = *(int *)((uVar12 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(uint *)(iVar5 + 0x10) != 0xffffffff) {
              uVar11 = *(undefined4 *)
                        ((*(uint *)(iVar5 + 0x10) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            }
          }
          sVar3 = FUN_004333d0(local_94,uVar11,iVar5,bVar13);
          local_80[local_94] = sVar3;
        }
        iVar5 = FUN_004325b0();
      }
      FUN_004369f0();
      uVar12 = local_94;
      while ((iVar5 = DAT_00880354, *(char *)(DAT_00880354 + 1) != '\0' && (uVar12 != 0xffffffff)))
      {
        uVar8 = uVar12 & 0xffff;
        iVar5 = uVar8 * 0x724;
        uVar12 = *(uint *)(iVar5 + 0x2c + *(int *)(DAT_00880360 + 0x34));
        sVar3 = *(short *)(iVar5 + *(int *)(DAT_00880360 + 0x34) + 0x3a);
        sVar1 = local_80[sVar3];
        if ((((sVar1 != -1) && ((!bVar13 || (sVar1 != sVar3)))) &&
            (FUN_004290f0((int)sVar1), param_2 != '\0')) &&
           (iVar5 = *(int *)(uVar8 * 0x724 + 0x18 + *(int *)(DAT_00880360 + 0x34)), iVar5 != -1)) {
          ai_communication_broadcast
                    (0x17 - (uint)(param_3 != '\0'),iVar5,0xffffffff,0xffffffff,0xffffffff,
                     0xffffffff,0);
        }
      }
      if (*(char *)(iVar6 + 0x1e) != '\0') {
        FUN_00436a30(0);
        iVar4 = actor_iterator_next();
        while (iVar4 != 0) {
          if ((*(uint *)(iVar4 + 0x44) & 0xffff) == uVar9) {
            sVar3 = local_80[*(short *)(iVar4 + 0x48)];
            if ((sVar3 != -1) && ((!bVar13 || (sVar3 != *(short *)(iVar4 + 0x48))))) {
              *(uint *)(iVar4 + 0x44) = param_1;
              *(short *)(iVar4 + 0x48) = sVar3;
            }
          }
          iVar4 = actor_iterator_next();
          iVar5 = DAT_00880354;
        }
        if (!bVar13) {
          *(undefined1 *)(iVar6 + 0x1e) = 0;
          *(undefined1 *)(iVar7 + 0x1e) = 1;
        }
      }
      if (*(char *)(iVar5 + 1) != '\0') {
        local_94 = *(uint *)(iVar5 + 8);
      }
      while ((uVar12 = local_94, *(char *)(DAT_00880354 + 1) != '\0' && (uVar12 != 0xffffffff))) {
        iVar5 = (uVar12 & 0xffff) * 0x724;
        local_94 = *(uint *)(iVar5 + 0x2c + *(int *)(DAT_00880360 + 0x34));
        iVar5 = iVar5 + *(int *)(DAT_00880360 + 0x34);
        if ((*(uint *)(iVar5 + 0x30) & 0xffff) == uVar9) {
          sVar3 = local_80[*(short *)(iVar5 + 0x38)];
          if ((sVar3 != -1) && ((!bVar13 || (sVar3 != *(short *)(iVar5 + 0x38))))) {
            *(uint *)(iVar5 + 0x30) = param_1;
            *(short *)(iVar5 + 0x38) = sVar3;
            if ((!bVar13) && (*(short *)(param_1 * 0xb0 + iVar2 + 0x7e) == DAT_0069e8d8)) {
              FUN_00436990();
              FUN_00436770(uVar12,*(undefined4 *)(iVar5 + 0x30),1);
            }
          }
        }
      }
      if (*(short *)(iVar6 + 2) != *(short *)(iVar7 + 2)) {
        FUN_0042bbb0();
      }
      FUN_00435f00();
    }
  }
  return;
}
#endif
