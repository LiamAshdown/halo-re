// ai_conversation_resolve_participant  (Ghidra: ai_conversation_resolve_participant; named for this rewrite)
// address 0x431680, size 1632 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/ai_types_notes.md's misattribution table places the 0x430830..0x431e70
// block on Scenario.ai_conversations, not squads; this function indexes
// global_scenario->ai_conversations (0x746f8c + 0x46c, stride 0x74) by
// ai_conversation.definition_index, then indexes that definition's participants
// (ScenarioAIConversation.participants at +0x50/+0x54, stride 0x54) by its own param_1, and
// fills in ai_conversation.participant_actor[param_1] plus the matching variant slot. Its
// only caller is ai_conversation_resolve_participants (0x430fc0). The phase-4 summary
// ("resolves what object/unit should occupy a given squad member slot") is right about the
// mechanism and wrong about the subsystem.
// register convention: EAX -> conversation_index (in_EAX; the caller loads it with
// `mov eax,[esp+0xc8]` at 0x431079 immediately before the call); param_1..param_5 are
// genuine stack arguments (`add esp,0x14`).
// blam-cc: EAX -> conversation_index, stack -> participant_index, out_resolved,
//          out_wants_alternate, out_blocked_by_player, inout_minimum_distance
//
// VERIFIED against disassembly 0x431680..0x431cdf (2026-09-30), after FIXED items: teams_are_enemies takes ECX = the player's
// team and EDX = the candidate's team (they were passed the other way round), and the squared-distance terms are summed in the
// x87 order dz, dy, dx. Notes confirmed by the disassembly:
//  - `goto LAB_00431850` in Ghidra's decompile is the loop's *advance* step (fetch the next candidate), not an early exit;
//    every `continue` below is one of those.
//  - The fixed-object path clears its own cursor to -1 after its one candidate so the next advance yields nothing.
//  - selection_type's case values (0/2/3/4/6/7) follow the jump table at 0x431ce8 (0 and 6 share the enemy check, 4 and 7 the
//    counts-toward-encounter bonus, 1 and 5 are the default).
//  - Ghidra's two phantom stack arrays are decompiler artifacts.
//  - actor+0x161 and the candidate's "already chosen by another participant" scan are transcribed literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include <stdint.h>

extern data_array *ai_conversation_data; // 0x008802d4
extern data_array *actor_data;           // 0x00880360
extern data_array *object_data;          // 0x008603b0
extern data_array *encounter_data;       // 0x008802c8
extern ai_globals *ai_globals_ptr;       // 0x00880354
extern Scenario *global_scenario;        // 0x00746f8c
extern datum_index *object_name_list; // 0x006b8cb8, 0x200 entries
extern uint32_t random_seed_global;      // 0x00719cd0

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern void ai_reference_actor_iterator_new(uint32_t reference,
                                            ai_reference_actor_iterator *iterator); // 0x432650
extern void *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0
extern float ai_communication_rate_player_proximity(uint8_t require_line_of_sight,
                                                    datum_index *out_player_object_index,
                                                    float *out_distance,
                                                    datum_index object_index); // 0x4303f0
extern int8_t teams_are_enemies(int16_t a, int16_t b); // 0x45bd50, blam-cc: ECX -> a, EDX -> b (table lookup [b][a])

// blam-cc: EAX -> conversation_index, stack -> participant_index, out_resolved,
//          out_wants_alternate, out_blocked_by_player, inout_minimum_distance
// Chooses which actor should play participant `participant_index` of a running conversation.
// Selection type 1 ("use this object") resolves immediately with no actor; otherwise the
// function walks a candidate set (a named scenario object, an encounter reference, or every
// live actor), rejects candidates already assigned to another participant slot or of the
// wrong actor type, scores the survivors by how visible a player is to them plus per-
// selection-type and per-variant bonuses, and keeps the best. Returns 1 when a participant
// was resolved.
int8_t ai_conversation_resolve_participant(int16_t participant_index, uint8_t *out_resolved,
                                           uint8_t *out_wants_alternate,
                                           uint8_t *out_blocked_by_player,
                                           float *inout_minimum_distance,
                                           datum_index conversation_index /* EAX */)
{
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    ScenarioAIConversationParticipant *participant;
    int16_t selection_type;
    datum_index best_actor;
    uint32_t best_variant;
    float best_score;
    float best_distance;
    int8_t resolved;
    uint8_t blocked;
    uint8_t use_named_object;   // bVar7
    uint8_t use_reference;      // bVar8
    uint8_t is_alternate_kind;  // bVar23 at entry: selection type 6 or 7
    datum_index named_object;
    ai_reference_actor_iterator reference_iterator;
    actor_iterator_state actor_iterator;
    actor *candidate;
    datum_index candidate_index;
    datum_index player_object_index;
    float player_distance;
    float player_score;
    float score;
    int16_t slot;
    int32_t i;
    float positions[24];        // up to 8 already-resolved participants, xyz each
    uint16_t resolved_count;
    uint16_t variant_candidates[8];
    int16_t variant_candidate_count;
    uint32_t zero_variant_slot;
    uint8_t have_zero_variant;
    int16_t candidate_variant;
    int16_t participant_variant;
    uint32_t chosen_variant;
    object *player_object;
    float dx, dy, dz, nearest;

    instance = (ai_conversation *)((uint8_t *)ai_conversation_data->data +
                                   (conversation_index & 0xffff) * k_ai_conversation_size);
    definition = (ScenarioAIConversation *)((uint8_t *)(uintptr_t)global_scenario->ai_conversations.pointer +
                                            (int32_t)instance->definition_index * 0x74);
    participant = (ScenarioAIConversationParticipant *)
        ((uint8_t *)(uintptr_t)definition->participants.pointer + (int32_t)participant_index * 0x54);
    selection_type = (int16_t)participant->selection_type;

    best_actor = (datum_index)k_datum_index_none;
    best_variant = (uint32_t)k_datum_index_none;
    best_distance = 3.4028235e+38f;
    blocked = 0;
    resolved = 0;

    if (selection_type == 1) {
        // "use this object": nothing to search for, and no actor is bound.
        best_variant = (best_variant & 0xffff0000u);
        resolved = 1;
        goto commit;
    }

    is_alternate_kind = (uint8_t)(selection_type == 6 || selection_type == 7);
    use_named_object = 0;
    use_reference = 0;
    named_object = (datum_index)k_datum_index_none;
    best_score = 0.0f;

    // Snapshot the body positions of every participant already resolved, so a candidate
    // standing right next to one of them scores higher.
    resolved_count = 0;
    for (i = 0; i < definition->participants.count; i++) {
        datum_index other = instance->participant_actor[i];
        if (other != (datum_index)k_datum_index_none) {
            actor *o = (actor *)((uint8_t *)actor_data->data + (other & 0xffff) * k_actor_size);
            positions[resolved_count * 3 + 0] = o->body_position.x;
            positions[resolved_count * 3 + 1] = o->body_position.y;
            positions[resolved_count * 3 + 2] = o->body_position.z;
            resolved_count = (uint16_t)(resolved_count + 1);
        }
    }

    if ((int16_t)participant->use_this_object == -1) {
        if (*(int32_t *)&((struct ScenarioAIConversationParticipant *)participant)->encounter_index == -1) {
            // No named object and no encounter: scan every live actor.
            if (ai_globals_ptr->actors_valid) {
                actor_iterator.filter_array = encounter_data;
                actor_iterator.next_index = 0;
                actor_iterator.cursor = -1;
                actor_iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
                actor_iterator.encounterless_done = 0;
                actor_iterator.active = 1;
                actor_iterator.actor_index = -1;
                actor_iterator.next_actor_index = -1;
            }
        } else {
            ai_reference_actor_iterator_new(
                ((struct ScenarioAIConversationParticipant *)participant)->encounter_index, &reference_iterator);
            use_reference = 1;
        }
    } else if ((int16_t)participant->use_this_object < 0 ||
               0x1ff < (int16_t)participant->use_this_object) {
        named_object = (datum_index)k_datum_index_none;
        use_named_object = 1;
    } else {
        named_object = object_name_list[(int16_t)participant->use_this_object];
        use_named_object = 1;
    }

    for (;;) {
        player_score = 0.0f;
        player_distance = 3.4028235e+38f;

        if (use_named_object) {
            // Single candidate: the named scenario object's controlling actor.
            object *obj = 0;
            void *element = 0;
            if (named_object != (datum_index)k_datum_index_none) {
                int16_t index = (int16_t)named_object;
                if (index >= 0 && index < object_data->maximum_count) {
                    int32_t byte_offset = (int32_t)object_data->size * (int32_t)index;
                    int16_t identifier = *(int16_t *)((uint8_t *)object_data->data + byte_offset);
                    int16_t salt = (int16_t)((uint32_t)named_object >> 16);
                    if (identifier != 0 && (salt == 0 || identifier == salt)) {
                        element = (uint8_t *)object_data->data + byte_offset;
                    }
                }
            }
            if (element != 0 && ((1 << (*((uint8_t *)element + 3) & 0x1f)) & 3u) != 0) {
                obj = *(object **)((uint8_t *)element + 8);
            }
            candidate = 0;
            candidate_index = (datum_index)k_datum_index_none;
            if (obj != 0) {
                datum_index a = *(datum_index *)((uint8_t *)obj + 0x1f4);
                if (a != (datum_index)k_datum_index_none) {
                    candidate = (actor *)((uint8_t *)actor_data->data + (a & 0xffff) * k_actor_size);
                    candidate_index = a;
                }
            }
            named_object = (datum_index)k_datum_index_none; // one shot only
        } else if (use_reference) {
            candidate = (actor *)ai_reference_actor_iterator_next(&reference_iterator);
            candidate_index = reference_iterator.actor_index;
        } else {
            candidate = actor_iterator_next(&actor_iterator);
            candidate_index = (datum_index)actor_iterator.actor_index;
        }

        if (candidate == 0) {
            break;
        }

        if (candidate->unit_index == (datum_index)k_datum_index_none ||
            candidate->type != (int16_t)participant->actor_type) {
            continue;
        }

        // Already assigned to an earlier participant slot?
        slot = 0;
        for (i = 0; i < definition->participants.count; i++) {
            if (candidate_index == instance->participant_actor[i]) {
                break;
            }
            slot = (int16_t)(slot + 1);
            if (slot >= definition->participants.count) {
                break;
            }
        }
        if (definition->participants.count <= slot) {
            player_score = ai_communication_rate_player_proximity(
                (uint8_t)(resolved_count == 0), &player_object_index, &player_distance,
                candidate->unit_index);
            if (player_object_index == (datum_index)k_datum_index_none) {
                if (!is_alternate_kind) {
                    blocked = 1;
                    continue;
                }
                player_object = 0;
                // Ghidra's `local_a4`: a local that is re-zeroed at the top of every
                // iteration and never assigned anywhere else, so the score restarts at 0.
                score = 0.0f;
            } else {
                player_object = ((object_header *)object_data->data)
                                    [player_object_index & 0xffff].data;
                score = player_score;
            }

            switch ((int16_t)participant->selection_type) {
            case 0:
            case 6:
                // Any friendly actor: reject when the player is an enemy of it.
                // FIXED (0x431a08..0x431a13): CX = the player's team (object +0xb8), DX = the candidate's team (actor +0x3e).
                if (player_object != 0 &&
                    teams_are_enemies(((struct object *)player_object)->owner_team,
                                      candidate->team) != 0) {
                    continue;
                }
                break;
            case 2:
                // Must share the player's vehicle.
                if (player_object == 0 ||
                    player_object->parent_object == (datum_index)k_datum_index_none ||
                    candidate->active_unit_index != player_object->parent_object) {
                    continue;
                }
                if (candidate->vehicle_gunner != 0) {
                    score = score + 1.0f;
                }
                break;
            case 3:
                // Must be on foot.
                if (candidate->active_unit_index != (datum_index)k_datum_index_none) {
                    continue;
                }
                break;
            case 4:
            case 7:
                if (candidate->counts_toward_encounter != 0) {
                    score = score + 1.5f;
                }
                break;
            default:
                break;
            }

            if (resolved_count == 0 && !is_alternate_kind && player_score < 2.0f &&
                definition->run_to_player_dist == 0.0f) {
                blocked = 1;
                continue;
            }

            if (0 < (int16_t)resolved_count) {
                nearest = 3.4028235e+38f;
                for (i = 0; i < (int32_t)resolved_count; i++) {
                    dx = positions[i * 3 + 0] - candidate->body_position.x;
                    dy = positions[i * 3 + 1] - candidate->body_position.y;
                    dz = positions[i * 3 + 2] - candidate->body_position.z;
                    dx = dz * dz + dy * dy + dx * dx; // x87 term order (0x431aea..0x431afa)
                    if (dx < nearest) {
                        nearest = dx;
                    }
                }
                if (nearest < 20.25f) {
                    score = (1.0f - ((float)sqrt((double)nearest) - 1.5f) * 0.33333334f) + score;
                }
            }

            // Prefer the variant slot matching this unit's current variant index
            // (object+0xbe); slot 0 is the wildcard and any other slot under 100 is a
            // random fallback.
            candidate_variant = *(int16_t *)((uint8_t *)((object_header *)object_data->data)
                                                 [candidate->unit_index & 0xffff].data + 0xbe);
            variant_candidate_count = 0;
            zero_variant_slot = (uint32_t)k_datum_index_none;
            have_zero_variant = 0;
            chosen_variant = 0;
            for (i = 0; i < 6; i++) {
                participant_variant = (int16_t)participant->variant_numbers[i];
                if (participant_variant != -1) {
                    if (participant_variant == candidate_variant) {
                        score = score + 0.7f;
                        chosen_variant = (uint32_t)i;
                        goto have_variant;
                    }
                    if (participant_variant == 0) {
                        have_zero_variant = 1;
                        zero_variant_slot = (uint32_t)i;
                    } else if (candidate_variant < 100 && participant_variant < 100) {
                        variant_candidates[variant_candidate_count] = (uint16_t)i;
                        variant_candidate_count = (int16_t)(variant_candidate_count + 1);
                    }
                }
            }
            if (have_zero_variant) {
                score = score + 0.7f;
                chosen_variant = zero_variant_slot;
            } else {
                if (variant_candidate_count < 1) {
                    continue;
                }
                // The original reads a full DWORD here (`mov ecx,[esp+0x78]` at 0x431bd5)
                // even though only int16 slots were written, so the high half of the
                // register is stale; only the low 16 bits are ever stored, so the stale
                // half is harmless and is not reproduced.
                chosen_variant = variant_candidates[0];
                if (variant_candidate_count != 1) {
                    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                    chosen_variant = (uint32_t)variant_candidates
                        [(int16_t)(((int32_t)variant_candidate_count *
                                    (int32_t)(random_seed_global >> 0x10)) >> 0x10)];
                }
            }
have_variant:
            if (best_score < score) {
                best_actor = candidate_index;
                best_score = score;
                best_distance = player_distance;
                resolved = 1;
                best_variant = chosen_variant;
            }
        }
    }

    if (resolved != 0) {
        goto commit;
    }
    // Nothing matched. When the participant carries the "has alternate" flag, tell the
    // caller so it runs the is_alternate pass; otherwise just report the blocked flag.
    if ((participant->flags & 2) == 0) {
        goto report;
    }
    if (out_wants_alternate != 0) {
        *out_wants_alternate = 1;
    }
    goto report;

commit:
    instance->participant_mask = instance->participant_mask | (1u << ((uint8_t)participant_index & 0x1f));
    instance->participant_actor[participant_index] = best_actor;
    *(int16_t *)((uint8_t *)instance + 0x18 + participant_index * 2) = (int16_t)best_variant;
    if (out_resolved != 0 && best_actor != (datum_index)k_datum_index_none) {
        *out_resolved = 1;
    }

report:
    if (blocked && out_blocked_by_player != 0) {
        *out_blocked_by_player = 1;
    }
    if (inout_minimum_distance != 0 && best_distance < *inout_minimum_distance) {
        *inout_minimum_distance = best_distance;
    }
    return resolved;
}

#if 0
Original Ghidra decompilation (0x431680):

char FUN_00431680(short param_1,undefined1 *param_2,undefined1 *param_3,undefined1 *param_4,
                 float *param_5)

{
  float fVar1;
  short sVar2;
  float fVar3;
  float fVar4;
  bool bVar5;
  ushort uVar6;
  bool bVar7;
  bool bVar8;
  bool bVar9;
  char cVar10;
  uint in_EAX;
  int iVar11;
  int iVar12;
  int iVar13;
  short sVar14;
  float *pfVar15;
  short sVar16;
  uint uVar17;
  int iVar18;
  int iVar19;
  int iVar20;
  uint uVar21;
  ushort uVar22;
  bool bVar23;
  float10 fVar24;
  float10 extraout_ST0;
  float afStackY_60060 [81911];
  short asStackY_10084 [32702];
  char local_e9;
  uint local_e4;
  float local_dc;
  uint local_d4;
  int local_cc;
  float local_c4;
  float local_c0;
  float local_b4;
  float local_b0;
  uint local_ac;
  uint local_a8;
  float local_a4;
  uint local_a0;
  undefined2 local_9c;
  undefined4 local_98;
  uint local_94;
  undefined1 local_90;
  undefined1 local_8f;
  uint local_8c;
  undefined4 local_88;
  uint local_84 [7];
  uint local_68;
  float local_60 [24];

  iVar11 = (in_EAX & 0xffff) * 100 + *(int *)(DAT_008802d4 + 0x34);
  iVar18 = *(short *)(iVar11 + 2) * 0x74 + *(int *)(global_scenario + 0x46c);
  sVar14 = *(short *)(param_1 * 0x54 + 4 + *(int *)(iVar18 + 0x54));
  iVar19 = param_1 * 0x54 + *(int *)(iVar18 + 0x54);
  local_d4 = 0xffffffff;
  local_ac = 0xffffffff;
  local_c4 = 3.4028235e+38;
  bVar9 = false;
  local_e9 = '\0';
  if (sVar14 == 1) {
    local_d4._0_2_ = 0;
    local_e9 = '\x01';
LAB_00431c43:
    *(uint *)(iVar11 + 0x14) = *(uint *)(iVar11 + 0x14) | 1 << ((byte)param_1 & 0x1f);
    *(uint *)(iVar11 + 0x28 + param_1 * 4) = local_ac;
    *(undefined2 *)(iVar11 + 0x18 + param_1 * 2) = (undefined2)local_d4;
    if (param_2 == (undefined1 *)0x0) goto LAB_00431c98;
    bVar23 = local_ac == 0xffffffff;
  }
  else {
    bVar23 = false;
    bVar7 = false;
    bVar8 = false;
    local_cc = -1;
    local_c0 = 0.0;
    if ((sVar14 == 6) || (sVar14 == 7)) {
      bVar23 = true;
    }
    iVar20 = *(int *)(iVar18 + 0x50);
    uVar22 = 0;
    sVar14 = 0;
    uVar6 = 0;
    if (0 < iVar20) {
      iVar12 = 0;
      uVar22 = 0;
      do {
        uVar17 = *(uint *)(iVar11 + 0x28 + iVar12 * 4);
        if (uVar17 != 0xffffffff) {
          pfVar15 = (float *)((uVar17 & 0xffff) * 0x724 + 300 + *(int *)(DAT_00880360 + 0x34));
          iVar12 = (int)(short)uVar22;
          local_60[iVar12 * 3] = *pfVar15;
          local_60[iVar12 * 3 + 1] = pfVar15[1];
          local_60[iVar12 * 3 + 2] = pfVar15[2];
          uVar22 = uVar6 + 1;
          uVar6 = uVar22;
        }
        sVar14 = sVar14 + 1;
        iVar12 = (int)sVar14;
      } while (iVar12 < iVar20);
    }
    sVar14 = *(short *)(iVar19 + 8);
    if (sVar14 == -1) {
      if (*(int *)(iVar19 + 0x44) == -1) {
        if (*(char *)(DAT_00880354 + 1) != '\0') {
          local_a0 = DAT_008802c8;
          local_94 = DAT_008802c8 ^ 0x69746572;
          local_9c = 0;
          local_98 = 0xffffffff;
          local_90 = 0;
          local_88 = 0xffffffff;
          local_8c = 0xffffffff;
          local_8f = 1;
        }
      }
      else {
        FUN_00432650(*(int *)(iVar19 + 0x44));
        bVar8 = true;
      }
    }
    else if ((sVar14 < 0) || (0x1ff < sVar14)) {
      local_cc = -1;
      bVar7 = true;
    }
    else {
      local_cc = *(int *)(DAT_006b8cb8 + sVar14 * 4);
      bVar7 = true;
    }
LAB_00431850:
    local_a4 = 0.0;
    local_b4 = 3.4028235e+38;
    if (bVar7) {
      iVar20 = 0;
      if (((local_cc != -1) && (sVar14 = (short)local_cc, -1 < sVar14)) &&
         (sVar14 < *(short *)(DAT_008603b0 + 0x20))) {
        iVar12 = (int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar14;
        sVar14 = *(short *)(iVar12 + *(int *)(DAT_008603b0 + 0x34));
        if ((sVar14 != 0) &&
           ((sVar16 = (short)((uint)local_cc >> 0x10), sVar16 == 0 || (sVar14 == sVar16)))) {
          iVar20 = iVar12 + *(int *)(DAT_008603b0 + 0x34);
        }
      }
      iVar12 = 0;
      if ((iVar20 != 0) && ((1 << (*(byte *)(iVar20 + 3) & 0x1f) & 3U) != 0)) {
        iVar12 = *(int *)(iVar20 + 8);
      }
      iVar20 = 0;
      local_e4 = 0xffffffff;
      if ((iVar12 != 0) && (uVar17 = *(uint *)(iVar12 + 500), uVar17 != 0xffffffff)) {
        iVar20 = (uVar17 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
        local_e4 = uVar17;
      }
      local_cc = -1;
    }
    else if (bVar8) {
      iVar20 = FUN_004326d0();
      local_e4 = local_68;
    }
    else {
      iVar20 = actor_iterator_next();
      local_e4 = local_8c;
    }
    if (iVar20 != 0) {
      if ((*(int *)(iVar20 + 0x18) != -1) && (*(short *)(iVar20 + 4) == *(short *)(iVar19 + 6))) {
        iVar12 = *(int *)(iVar18 + 0x50);
        sVar14 = 0;
        if (0 < iVar12) {
          iVar13 = 0;
          do {
            if (local_e4 == *(uint *)(iVar11 + 0x28 + iVar13 * 4)) break;
            sVar14 = sVar14 + 1;
            iVar13 = (int)sVar14;
          } while (iVar13 < iVar12);
        }
        if (iVar12 <= sVar14) {
          fVar24 = (float10)FUN_004303f0(uVar22 == 0,&local_a8,&local_b4);
          local_b0 = (float)fVar24;
          if (local_a8 == 0xffffffff) {
            if (!bVar23) {
              bVar9 = true;
              goto LAB_00431850;
            }
            iVar12 = 0;
            fVar1 = local_a4;
          }
          else {
            iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_a8 & 0xffff) * 0xc);
            fVar1 = local_b0;
          }
          fVar24 = (float10)fVar1;
          switch(*(undefined2 *)(iVar19 + 4)) {
          case 0:
          case 6:
            if ((iVar12 != 0) && (cVar10 = FUN_0045bd50(), fVar24 = extraout_ST0, cVar10 != '\0'))
            goto LAB_00431850;
            break;
          case 2:
            if (((iVar12 == 0) || (*(int *)(iVar12 + 0x11c) == -1)) ||
               (*(int *)(iVar20 + 0x158) != *(int *)(iVar12 + 0x11c))) goto LAB_00431850;
            if (*(char *)(iVar20 + 0x161) != '\0') {
              fVar24 = fVar24 + (float10)1.0;
            }
            break;
          case 3:
            if (*(int *)(iVar20 + 0x158) != -1) goto LAB_00431850;
            break;
          case 4:
          case 7:
            if (*(char *)(iVar20 + 0x1c) != '\0') {
              fVar24 = fVar24 + (float10)1.5;
            }
          }
          if (((uVar22 != 0) || ((bVar23 || (2.0 <= local_b0)))) ||
             (*(float *)(iVar18 + 0x28) != 0.0)) {
            if (0 < (short)uVar6) {
              local_dc = 3.4028235e+38;
              pfVar15 = local_60 + 2;
              uVar17 = (uint)uVar6;
              do {
                fVar1 = pfVar15[-2] - *(float *)(iVar20 + 300);
                fVar4 = pfVar15[-1] - *(float *)(iVar20 + 0x130);
                fVar3 = *pfVar15 - *(float *)(iVar20 + 0x134);
                fVar1 = fVar1 * fVar1 + fVar4 * fVar4 + fVar3 * fVar3;
                if (fVar1 < local_dc) {
                  local_dc = fVar1;
                }
                pfVar15 = pfVar15 + 3;
                uVar17 = uVar17 - 1;
              } while (uVar17 != 0);
              if (local_dc < 20.25) {
                fVar24 = ((float10)1.0 -
                         (SQRT((float10)local_dc) - (float10)1.5) * (float10)0.33333334) + fVar24;
              }
            }
            sVar14 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (*(uint *)(iVar20 + 0x18) & 0xffff) * 0xc) + 0xbe);
            sVar16 = 0;
            uVar21 = 0xffffffff;
            bVar5 = false;
            uVar17 = 0;
            do {
              sVar2 = *(short *)(iVar19 + 0x18 + (short)uVar17 * 2);
              if (sVar2 != -1) {
                if (sVar2 == sVar14) {
                  fVar24 = fVar24 + (float10)0.7;
                  goto LAB_00431c06;
                }
                if (sVar2 == 0) {
                  bVar5 = true;
                  uVar21 = uVar17;
                }
                else if ((sVar14 < 100) && (sVar2 < 100)) {
                  *(short *)((int)local_84 + sVar16 * 2) = (short)uVar17;
                  sVar16 = sVar16 + 1;
                }
              }
              uVar17 = uVar17 + 1;
            } while ((short)uVar17 < 6);
            if (bVar5) {
              fVar24 = fVar24 + (float10)0.7;
              uVar17 = uVar21;
            }
            else {
              if (sVar16 < 1) goto LAB_00431850;
              uVar17 = local_84[0];
              if (sVar16 != 1) {
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                uVar17 = (uint)*(ushort *)
                                ((int)local_84 +
                                (short)((int)sVar16 * (random_seed_global >> 0x10) >> 0x10) * 2);
              }
            }
LAB_00431c06:
            if ((float10)local_c0 < fVar24) {
              local_ac = local_e4;
              local_c0 = (float)fVar24;
              local_c4 = local_b4;
              local_e9 = '\x01';
              local_d4 = uVar17;
            }
            goto LAB_00431850;
          }
          bVar9 = true;
        }
      }
      goto LAB_00431850;
    }
    if (local_e9 != '\0') goto LAB_00431c43;
    if ((*(byte *)(iVar19 + 2) & 2) == 0) goto LAB_00431c98;
    bVar23 = param_3 == (undefined1 *)0x0;
    param_2 = param_3;
  }
  if (!bVar23) {
    *param_2 = 1;
  }
LAB_00431c98:
  if ((bVar9) && (param_4 != (undefined1 *)0x0)) {
    *param_4 = 1;
  }
  if ((param_5 != (float *)0x0) && (local_c4 < *param_5)) {
    *param_5 = local_c4;
    return local_e9;
  }
  return local_e9;
}
#endif
