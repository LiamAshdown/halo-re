// ai_squad_find_best_matching_member  (Ghidra: ai_squad_find_best_matching_member; named for this rewrite)
// address 0x4333d0, size 437 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump; return priority FIXED)
// evidence: objdump (bin/halo.exe 0x4333d0..0x43358f) confirms the loop reads
// ai_reference_squad_iterator_new/_next's (0x4324f0/0x4325b0, this batch) shared iterator
// struct directly off this function's own stack (the "local_c" Ghidra shows with no visible
// assignment is exactly iterator.cursor, written by the callee through the same memory).
// For each squad in the packed reference's encounter, resolves ScenarioSquad.actor_type
// (+0x20, matching ai_reference_spawn_starting_location_object's own use of that field)
// through Scenario.actor_palette (checking the tag group fourcc is 'actv' before trusting
// it) to the ActorVariant tag's data, and through ActorVariant.actor_definition (+0x10) to
// the underlying Actor tag's data. Picks the first squad, in priority order, that: matches
// requested_squad_index exactly (only when match_by_index is set); resolves to exactly
// requested_actor_variant_data; resolves to exactly requested_actor_data; or resolves to an
// Actor tag whose type (+0x14, ai.h's own actor.type source) matches requested_actor_data's
// type -- falling back to the very first squad in the range if nothing else matches.
// register convention: Ghidra fully resolved all four parameters (confirmed in order by
// objdump, reading esp+0x3c/0x40/0x44/0x48 in ascending order).
//   // blam-cc: EAX -> packed_reference, stack -> requested_squad_index,
//   requested_actor_data, requested_actor_variant_data, match_by_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario;   // 0x00746f8c
extern tag_instance *tag_instances; // 0x0087bc14

extern void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator); // 0x4324f0, this batch
extern encounter_squad_state *ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator); // 0x4325b0, this batch

// blam-cc: EAX -> packed_reference, stack -> requested_squad_index, requested_actor_data,
//   requested_actor_variant_data, match_by_index
int32_t ai_squad_find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index,
                                            uint8_t *requested_actor_data, uint8_t *requested_actor_variant_data,
                                            char match_by_index)
{
    ScenarioEncounter *encounter_definition;
    ai_reference_squad_iterator iterator;
    encounter_squad_state *state;
    int32_t best_by_index = -1;
    int32_t best_by_actor = -1;
    int32_t best_by_variant = -1;
    int32_t best_by_type = -1;
    int32_t first_any = -1;

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[packed_reference & 0xffff];

    ai_reference_squad_iterator_new(packed_reference, &iterator);
    state = ai_reference_squad_iterator_next(&iterator);
    if (state != 0) {
        do {
            ScenarioSquad *squad = &((ScenarioSquad *)encounter_definition->squads.pointer)[iterator.cursor];
            int16_t actor_palette_index = (int16_t)squad->actor_type;
            uint8_t *actor_variant_data = 0;
            uint8_t *actor_data = 0;

            if (actor_palette_index >= 0 && actor_palette_index < global_scenario->actor_palette.count) {
                TagDependency *entry =
                    &((TagDependency *)global_scenario->actor_palette.pointer)[actor_palette_index];
                datum_index actor_variant_tag = *(datum_index *)&entry->tag_id;
                if (actor_variant_tag != (datum_index)k_datum_index_none &&
                    tag_instances[actor_variant_tag & 0xffff].group_tag == 0x61637476 /* 'actv' */) {
                    actor_variant_data = (uint8_t *)tag_instances[actor_variant_tag & 0xffff].data;
                    if (*(uint32_t *)(actor_variant_data + 0x10) != (uint32_t)k_datum_index_none) {
                        datum_index actor_tag = *(datum_index *)(actor_variant_data + 0x10);
                        actor_data = (uint8_t *)tag_instances[actor_tag & 0xffff].data;
                    }
                }
            }

            if (best_by_index == -1 && match_by_index != 0 && requested_squad_index == iterator.cursor) {
                best_by_index = iterator.cursor;
            }
            if (best_by_actor == -1 && requested_actor_data != 0 && actor_data != 0 &&
                requested_actor_data == actor_data) {
                best_by_actor = iterator.cursor;
            }
            if (best_by_variant == -1 && requested_actor_variant_data != 0 && actor_variant_data != 0 &&
                requested_actor_variant_data == actor_variant_data) {
                best_by_variant = iterator.cursor;
            }
            if (best_by_type == -1 && requested_actor_data != 0 && actor_data != 0 &&
                *(int16_t *)(requested_actor_data + 0x14) == *(int16_t *)(actor_data + 0x14)) {
                best_by_type = iterator.cursor;
            }
            if (first_any == -1) {
                first_any = iterator.cursor;
            }

            state = ai_reference_squad_iterator_next(&iterator);
        } while (state != 0);

        if (best_by_index != -1) return best_by_index;
        // FIXED (objdump 0x433536..0x433568): the variant match ([esp+0xc]) is returned before the actor match
        if (best_by_variant != -1) return best_by_variant;
        if (best_by_actor != -1) return best_by_actor;
        if (best_by_type != -1) return best_by_type;
        if (first_any != -1) return first_any;
    }

    if (encounter_definition->squads.count > 0) {
        return 0;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4333d0):

int FUN_004333d0(short param_1,int param_2,int param_3,char param_4)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_c;

  local_18 = -1;
  local_1c = -1;
  local_20 = -1;
  local_24 = -1;
  local_28 = -1;
  iVar5 = (in_EAX & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  FUN_004324f0();
  iVar4 = FUN_004325b0();
  iVar3 = DAT_0087bc14;
  if (iVar4 != 0) {
    do {
      sVar1 = *(short *)(local_c * 0xe8 + *(int *)(iVar5 + 0x84) + 0x20);
      iVar4 = 0;
      iVar6 = 0;
      if ((((-1 < sVar1) && ((int)sVar1 < *(int *)(global_scenario + 0x420))) &&
          (uVar2 = *(uint *)(sVar1 * 0x10 + *(int *)(global_scenario + 0x424) + 0xc),
          uVar2 != 0xffffffff)) && (*(int *)((short)uVar2 * 0x20 + iVar3) == 0x61637476)) {
        iVar4 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + iVar3);
        if (*(uint *)(iVar4 + 0x10) != 0xffffffff) {
          iVar6 = *(int *)((*(uint *)(iVar4 + 0x10) & 0xffff) * 0x20 + 0x14 + iVar3);
        }
      }
      if ((((short)local_28 == -1) && (param_4 != '\0')) && (param_1 == local_c)) {
        local_28 = local_c;
      }
      if ((((short)local_24 == -1) && (param_3 != 0)) && ((iVar4 != 0 && (param_3 == iVar4)))) {
        local_24 = local_c;
      }
      if (((((short)local_20 == -1) && (param_2 != 0)) && (iVar6 != 0)) && (param_2 == iVar6)) {
        local_20 = local_c;
      }
      if ((((short)local_1c == -1) && (param_2 != 0)) &&
         ((iVar6 != 0 && (*(short *)(param_2 + 0x14) == *(short *)(iVar6 + 0x14))))) {
        local_1c = local_c;
      }
      if ((short)local_18 == -1) {
        local_18 = local_c;
      }
      iVar4 = FUN_004325b0();
    } while (iVar4 != 0);
    if ((short)local_28 != -1) {
      return local_28;
    }
    if ((short)local_24 != -1) {
      return local_24;
    }
    if ((short)local_20 != -1) {
      return local_20;
    }
    if ((short)local_1c != -1) {
      return local_1c;
    }
    if ((short)local_18 != -1) {
      return local_18;
    }
  }
  if (*(int *)(iVar5 + 0x80) < 1) {
    return -1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
