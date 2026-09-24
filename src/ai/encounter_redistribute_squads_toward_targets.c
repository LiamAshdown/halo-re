// encounter_redistribute_squads_toward_targets  (Ghidra: encounter_redistribute_squads_toward_targets, renamed)
// address 0x4394a0, size 2235 bytes
// name confidence: 0.35  rewrite confidence: 0.15
// evidence: types/ai.h encounter (unknown_62 target mode, unknown_64 explicit target,
//   unknown_68 leash distance -- see UNSURE), encounter_squad_state, encounter_platoon_state,
//   actor (body_position +0x12c, next_in_encounter +0x2c, squad_index +0x3a, platoon_index
//   +0x3c, firing_position_index +0x3b8, active_movement.type +0x46c, active_movement.extra
//   +0x480, mode +0x6c); types/tags.h ScenarioEncounter.squads/platoons/firing_positions,
//   ScenarioSquad.flags (+0x28, confirmed by offsetof) and .platoon (+0x22, confirmed) and
//   .attacking/.attacking_search/.attacking_guard/.defending/.defending_search/
//   .defending_guard/.pursuing (+0x54..+0x6c, all confirmed by offsetof). Calls
//   object_get_position, object_try_and_get(3) (both established), the actor-iterator pair
//   at 0x432650/0x4326d0 (phase-4: "iterates the actors belonging to a squad/team
//   reference"), squad_remove_actor (0x436620, already named, not yet rewritten),
//   encounter_add_actor (0x436770, phase-4 "registers a newly spawned actor as a member of a
//   squad") and ai_actor_unlink_from_unassigned_list (0x436990, phase-4 "removes an actor from the global list of
//   squad-less actors").
//
// Given the size, the number of never-independently-confirmed ScenarioSquad/tag sub-fields
// this reaches into, and several register-argument gaps at its callees (documented per call
// site below), this rewrite is kept deliberately close to the Ghidra decompilation rather
// than fully re-derived, the same tradeoff src/ai/actor_refresh_combat_context.c documents
// for a function of comparable scope. Field accesses with an established types/ai.h name are
// used; everything else is left as raw offset arithmetic with an inline comment, exactly as
// Ghidra shows it.
//
// register convention: stack -> encounter_index (0xffffffff means "the global unassigned
//   actor list" in the two places this function branches on it, matching the same sentinel
//   in encounter_propagate_platoon_state_to_actors.c).
//   // blam-cc: stack -> encounter_index
//
// UNSURE, broadly (see also the inline comments below):
//  - encounter.unknown_68 is declared int16_t in types/ai.h but is read here as a float (the
//    "leash distance" the redistribution allows before it stops); another same-offset type
//    disagreement between functions, not resolved in the header.
//  - The three floats used as a scratch `real_point3d` (local_1dc/local_1d8/local_1d4 in the
//    original) are ALSO used, only in the sVar8==1 branch, as the first three fields of a
//    stack-allocated `data_iterator` passed to data_iterator_next() over player_data --
//    Ghidra shows this as three independent float writes because next_index is a 16-bit
//    field embedded in a stack slot Ghidra typed as float. The lifetimes do not overlap
//    (the iterator is only read by data_iterator_next before the position is ever used), so
//    this rewrite models them as two separate C variables rather than one aliased slot.
//  - The function-pointer table at 0x655278, indexed by actor.mode * 0x38, is one field
//    (offset +0x24, inside the 28-byte "unknown_1c" tail types/ai.h leaves unnamed) short of
//    lining up with actor_mode_definitions at 0x655254 (stride 0x38 there too) -- almost
//    certainly the same table, an as-yet-unnamed fourth per-mode callback, but declared here
//    as its own opaque table rather than asserting that without independent confirmation.
//  - object_try_and_get(3), squad_remove_actor(0), and the two actor-iterator calls are
//    called here with fewer/different arguments than a canonical signature would need
//    (register-only handoffs Ghidra did not attribute); each is declared locally with
//    exactly the arity this call site shows, per this module's established convention for
//    that situation (see e.g. encounter_squad_spawn_actor.c).
//  - The `auStackY_11c0[998]` stack array Ghidra shows in the original is never read or
//    written anywhere in the decompiled body and is omitted here as dead/unused stack space.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; also the missing index = -1 store (0x43959a)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern data_array *encounter_data;  // 0x008802c8
extern data_array *actor_data;      // 0x00880360
extern data_array *player_data;     // 0x0087a480, stride 0x200 (no types/players.h yet)
extern Scenario *global_scenario;   // 0x00746f8c
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern encounter_squad_state *encounter_squad_states;   // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4
extern void *ai_actor_mode_dispatch_table; // 0x00655278, see header UNSURE

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void object_get_position(datum_index object_index, real_point3d *out_position); // 0x4f6900, UNSURE signature (called here with no visible arguments; see call sites)
extern void *object_try_and_get(int32_t kind); // 0x4f6ec0, see header UNSURE
extern void ai_reference_actor_iterator_new(datum_index packed_reference); // 0x432650 = actor iterator init, see header
extern void *ai_reference_actor_iterator_next(void);                        // 0x4326d0 = actor iterator advance, see header
extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters); // 0x436620, blam-cc: EAX -> actor_index, stack -> skip_counters
extern void ai_actor_unlink_from_unassigned_list(void);                                              // 0x436990, called with no visible arguments
extern void encounter_add_actor(int16_t squad_index, datum_index actor_index,
    datum_index encounter_index, uint8_t keep_team); // 0x436770, blam-cc: DX -> squad_index
    // UNSURE: the squad index arrives in DX and Ghidra did not attribute it to this call
    // site, so the actor's current squad_index is passed; encounter_add_actor writes it
    // straight back into the same field.
extern double sqrt(double x); // FSQRT

// blam-cc: stack -> encounter_index
void encounter_redistribute_squads_toward_targets(datum_index encounter_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    int16_t target_mode;
    datum_index targets[8];
    uint16_t target_count;
    real_point3d target_positions[8];
    float best_distance_to_target[8]; // local_120
    real_point3d chosen_position;     // local_1dc/1d8/1d4 once repurposed from the iterator
    datum_index chosen_object;        // puVar23
    uint32_t squad_considered_mask[2];   // local_1c0[0..1]
    uint32_t squad_active_mask[2];       // local_1c0[2..3]
    uint32_t squad_occupied_mask[2];     // local_1c0[4..5]
    uint32_t combined_trigger_mask;      // local_208
    uint32_t squad_trigger_mask[64];     // local_100
    int16_t total_occupancy;             // sVar24 (first use)
    int16_t squad_count;
    int16_t squad_index;
    int16_t i;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];
    target_mode = self->unknown_62;
    target_count = 0;

    if (target_mode == 1) {
        data_iterator player_iter;
        void *player_record;

        player_iter.data = player_data;
        player_iter.next_index = 0;
        player_iter.index = (datum_index)k_datum_index_none; // 0x43959a mov [esp+0x48],ebp (-1)
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_record = data_iterator_next(&player_iter);
        if (player_record == 0) {
            return;
        }
        do {
            if ((*(int32_t *)((uint8_t *)player_record + 0x34) != -1) && ((int16_t)target_count < 8)) {
                targets[(int16_t)target_count] = *(datum_index *)((uint8_t *)player_record + 0x34);
                target_count = target_count + 1;
            }
            player_record = data_iterator_next(&player_iter);
        } while (player_record != 0);
    } else if (target_mode == 2) {
        datum_index cached_target = self->unknown_64;
        if (object_try_and_get(3) == 0) {
            self->unknown_64 = (datum_index)0xffffffff;
            return;
        }
        targets[0] = cached_target;
        target_count = 1;
        goto have_targets;
    } else if (target_mode == 3) {
        void *member;
        if (self->unknown_64 == (datum_index)0xffffffff) {
            return;
        }
        ai_reference_actor_iterator_new(self->unknown_64);
        member = ai_reference_actor_iterator_next();
        if (member == 0) {
            return;
        }
        do {
            if (7 < (int16_t)target_count) break;
            targets[(int16_t)target_count] = ((actor *)member)->unit_index;
            target_count = target_count + 1;
            member = ai_reference_actor_iterator_next();
        } while (member != 0);
    } else {
        return;
    }

    if ((int16_t)target_count < 1) {
        return;
    }

have_targets:
    squad_considered_mask[0] = 0;
    squad_considered_mask[1] = 0;
    squad_active_mask[0] = 0;
    squad_active_mask[1] = 0;
    combined_trigger_mask = 0;
    total_occupancy = 0;
    squad_occupied_mask[0] = 0;
    squad_occupied_mask[1] = 0;
    squad_index = 0;

    if (0 >= self->squad_count) {
        return;
    }
    squad_count = self->squad_count;
    {
        int16_t bit_index = 0;
        ScenarioSquad *squad_definition = (ScenarioSquad *)encounter_definition->squads.pointer;

        do {
            if ((squad_definition->flags & 0x20) != 0) { // automatic_migration
                encounter_squad_state *squad_state = &encounter_squad_states[(int16_t)(self->first_squad + squad_index)];
                uint32_t bit = 1u << (bit_index & 0x1f);
                int16_t word = bit_index >> 5;

                squad_considered_mask[word] |= bit;
                total_occupancy = total_occupancy + squad_state->unknown_18;

                if (squad_state->unknown_10 != 0) { // "has valid platoon" gate (encounter_squad_state+0x10)
                    encounter_platoon_state *platoon_state;
                    int16_t platoon_index = squad_definition->platoon;
                    uint32_t trigger;

                    squad_active_mask[word] |= bit;

                    if ((platoon_index < 0) || (encounter_definition->platoons.count <= (uint32_t)platoon_index) ||
                        (((uint8_t *)&encounter_platoon_states[(int16_t)(self->first_platoon + platoon_index)])[0] == 0)) {
                        // not actively attacking: watch the "start attacking" trigger group
                        trigger = squad_definition->attacking | squad_definition->attacking_search | squad_definition->attacking_guard;
                    } else {
                        // already attacking: watch the "switch to defending" trigger group
                        trigger = squad_definition->defending | squad_definition->defending_search | squad_definition->defending_guard;
                    }
                    (void)platoon_state; // silence unused-variable warning when the branch above is taken

                    squad_trigger_mask[bit_index] = trigger;
                    combined_trigger_mask |= trigger;

                    if (0 < squad_state->unknown_18) {
                        squad_occupied_mask[word] |= bit;
                    }
                }
            }

            squad_index = squad_index + 1;
            bit_index = bit_index + 1;
            squad_definition = squad_definition + 1;
        } while (squad_index < squad_count);
    }

    if ((total_occupancy <= 0) || (combined_trigger_mask == 0)) {
        return;
    }

    if (target_count == 1) {
        object_get_position(targets[0], &target_positions[0]);
    } else {
        int16_t t;
        datum_index next_actor;
        uint8_t have_target;

        for (t = 0; t < (int16_t)target_count; t = t + 1) {
            best_distance_to_target[t] = 3.4028235e+38f;
        }
        for (t = 0; t < (int16_t)target_count; t = t + 1) {
            object_get_position(targets[t], &target_positions[t]);
        }

        have_target = 0;
        next_actor = (datum_index)0xffffffff;
        if (ai_globals_ptr->actors_valid != 0) {
            if (encounter_index == (datum_index)0xffffffff) {
                next_actor = ai_globals_ptr->unknown_08;
            } else {
                next_actor = self->first_actor;
            }
            have_target = (uint8_t)(next_actor != (datum_index)0xffffffff);
        }

        while (have_target && (next_actor != (datum_index)0xffffffff)) {
            actor *member = (actor *)((uint8_t *)actor_data->data + (next_actor & 0xffff) * sizeof(actor));
            int16_t member_squad = member->squad_index;
            next_actor = member->next_in_encounter;

            if (((squad_considered_mask[member_squad >> 5] & (1u << (member_squad & 0x1f))) != 0) &&
                (0 < (int16_t)target_count)) {
                for (t = 0; t < (int16_t)target_count; t = t + 1) {
                    float dx = member->body_position.x - target_positions[t].x;
                    float dy = member->body_position.y - target_positions[t].y;
                    float dz = member->body_position.z - target_positions[t].z;
                    float d2 = dx * dx + dy * dy + dz * dz;
                    if (best_distance_to_target[t] <= d2) {
                        d2 = best_distance_to_target[t];
                    }
                    best_distance_to_target[t] = d2;
                }
            }
        }

        if ((int16_t)target_count < 1) {
            return;
        }
        chosen_object = (datum_index)0xffffffff;
        for (t = 0; t < (int16_t)target_count; t = t + 1) {
            if (best_distance_to_target[t] < 3.4028235e+38f) {
                chosen_object = targets[t];
                chosen_position = target_positions[t];
            }
        }
    }

    if (chosen_object == (datum_index)0xffffffff) {
        return;
    }

    {
        float group_distance[26]; // local_1a8, one slot per ScenarioSquadAttacking bit ('a'..'z')
        ScenarioFiringPosition *firing_positions;
        int32_t firing_count;
        int32_t fp;
        int16_t g;
        int16_t best_squad;
        int16_t best_occupied_squad;
        float best_squad_distance;
        float best_occupied_distance;

        for (g = 0; g < 26; g = g + 1) {
            group_distance[g] = 3.4028235e+38f;
        }

        firing_count = (int32_t)encounter_definition->firing_positions.count;
        firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;
        for (fp = 0; fp < firing_count; fp = fp + 1) {
            int16_t group = firing_positions[fp].group_index;
            if ((combined_trigger_mask & (1u << (group & 0x1f))) != 0) {
                float dx = chosen_position.x - firing_positions[fp].position.x;
                float dy = chosen_position.y - firing_positions[fp].position.y;
                float dz = chosen_position.z - firing_positions[fp].position.z;
                float d2 = dx * dx + dy * dy + dz * dz;
                if (group_distance[group] <= d2) {
                    d2 = group_distance[group];
                }
                group_distance[group] = d2;
            }
        }

        best_squad_distance = 3.4028235e+38f;
        best_occupied_distance = -1.0f; // matches the original's (float)0xff7fffff bit pattern used as "smallest so far, start below everything"
        best_squad = -1;
        best_occupied_squad = -1;

        if (0 < squad_count) {
            for (squad_index = 0; squad_index < squad_count; squad_index = squad_index + 1) {
                uint32_t bit = 1u << (squad_index & 0x1f);
                if ((squad_active_mask[squad_index >> 5] & bit) != 0) {
                    float best_for_squad = 3.4028235e+38f;
                    int16_t bit_pos;
                    for (bit_pos = 0; bit_pos < 26; bit_pos = bit_pos + 1) {
                        if (((squad_trigger_mask[squad_index] & (1u << (bit_pos & 0x1f))) != 0) &&
                            (group_distance[bit_pos] < best_for_squad)) {
                            best_for_squad = group_distance[bit_pos];
                        }
                    }
                    if (best_for_squad < best_squad_distance) {
                        best_squad_distance = best_for_squad;
                        best_squad = squad_index;
                    }
                    if (((squad_occupied_mask[squad_index >> 5] & bit) != 0) &&
                        (best_occupied_distance < best_for_squad)) {
                        best_occupied_distance = best_for_squad;
                        best_occupied_squad = squad_index;
                    }
                }
            }

            if (best_squad != -1) {
                if (best_occupied_squad != -1) {
                    float leash = (self->unknown_68 <= 0) ? 2.0f : *(float *)&self->unknown_68; // UNSURE, see header
                    if ((float)sqrt(best_occupied_distance) - leash <= (float)sqrt(best_squad_distance)) {
                        return;
                    }
                }

                {
                    datum_index next_actor;
                    uint8_t have_target;

                    have_target = 0;
                    next_actor = (datum_index)0xffffffff;
                    if (ai_globals_ptr->actors_valid != 0) {
                        if (encounter_index == (datum_index)0xffffffff) {
                            next_actor = ai_globals_ptr->unknown_08;
                        } else {
                            next_actor = self->first_actor;
                        }
                        have_target = (uint8_t)(next_actor != (datum_index)0xffffffff);
                    }

                    while (have_target && (next_actor != (datum_index)0xffffffff)) {
                        actor *member = (actor *)((uint8_t *)actor_data->data + (next_actor & 0xffff) * sizeof(actor));
                        int16_t member_squad = member->squad_index;

                        if (((squad_considered_mask[member_squad >> 5] & (1u << (member_squad & 0x1f))) != 0) &&
                            (member_squad != best_squad)) {
                            member->firing_position_index = (int16_t)0xffff;
                            if ((member->active_movement.type == 3) || (member->active_movement.type == 4)) {
                                member->active_movement.type = 0;
                                member->active_movement.extra = 0xffffffff;
                            }
                            {
                                void (*dispatch)(datum_index) = *(void (**)(datum_index))
                                    ((uint8_t *)&ai_actor_mode_dispatch_table + member->mode * 0x38);
                                if (dispatch != 0) {
                                    dispatch(next_actor);
                                }
                            }

                            if (member->unknown_09 == 0) {
                                if (member->encounter_index != (datum_index)0xffffffff) {
                                    encounter_remove_actor(next_actor, 0);
                                }
                            } else {
                                ai_actor_unlink_from_unassigned_list();
                            }

                            if (encounter_index == (datum_index)0xffffffff) {
                                if (ai_globals_ptr->actors_valid != 0) {
                                    member->next_in_encounter = ai_globals_ptr->unknown_08;
                                    ai_globals_ptr->unknown_08 = next_actor;
                                    member->unknown_09 = 1;
                                    *(uint16_t *)&member->unknown_10 = -(uint16_t)(member->active != 0) & 0x5a;
                                    {
                                        void (*dispatch)(datum_index) = *(void (**)(datum_index))
                                            ((uint8_t *)&ai_actor_mode_dispatch_table + member->mode * 0x38);
                                        if (dispatch != 0) {
                                            dispatch(next_actor);
                                        }
                                    }
                                }
                            } else {
                                encounter_add_actor(member->squad_index, next_actor, encounter_index, 1);
                            }
                        }

                        next_actor = member->next_in_encounter;
                    }
                }
            }
        }
    }
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_004394a0 @ 0x4394a0) ----
void FUN_004394a0(uint param_1)

{
  float fVar1;
  char cVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  float fVar6;
  float fVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  uint uVar12;
  byte bVar13;
  int iVar14;
  int iVar15;
  float *pfVar16;
  uint uVar17;
  float *pfVar18;
  int iVar19;
  short *psVar20;
  float fVar21;
  ushort uVar22;
  uint *puVar23;
  short sVar24;
  uint auStackY_11c0 [998];
  uint local_208;
  int local_200;
  float local_1fc;
  uint *local_1f8;
  uint *local_1f0;
  int local_1ec;
  float local_1dc;
  float local_1d8;
  float local_1d4;
  uint local_1c0 [6];
  float local_1a8 [26];
  int local_140 [8];
  float local_120 [8];
  uint local_100 [64];

  iVar14 = (param_1 & 0xffff) * 0x6c;
  iVar19 = *(int *)(DAT_008802c8 + 0x34) + iVar14;
  iVar9 = (param_1 & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  sVar8 = *(short *)(iVar19 + 0x62);
  uVar22 = 0;
  local_1f0 = (uint *)0xffffffff;
  if (sVar8 == 1) {
    local_1dc = DAT_0087a480;
    local_1d8 = (float)((uint)local_1d8 & 0xffff0000);
    local_1d4 = -NAN;
    iVar10 = data_iterator_next();
    if (iVar10 == 0) {
      return;
    }
    do {
      if ((*(int *)(iVar10 + 0x34) != -1) && ((short)uVar22 < 8)) {
        local_140[(short)uVar22] = *(int *)(iVar10 + 0x34);
        uVar22 = uVar22 + 1;
      }
      iVar10 = data_iterator_next();
    } while (iVar10 != 0);
  }
  else {
    if (sVar8 == 2) {
      iVar10 = *(int *)(iVar19 + 100);
      iVar11 = object_try_and_get(3);
      if (iVar11 == 0) {
        *(undefined4 *)(iVar19 + 100) = 0xffffffff;
        return;
      }
      local_140[0] = iVar10;
      uVar22 = 1;
      goto LAB_004395e0;
    }
    if (sVar8 != 3) {
      return;
    }
    if (*(int *)(iVar19 + 100) == -1) {
      return;
    }
    FUN_00432650(*(int *)(iVar19 + 100));
    iVar10 = FUN_004326d0();
    if (iVar10 == 0) {
      return;
    }
    do {
      if (7 < (short)uVar22) break;
      local_140[(short)uVar22] = *(int *)(iVar10 + 0x18);
      uVar22 = uVar22 + 1;
      iVar10 = FUN_004326d0();
    } while (iVar10 != 0);
  }
  if ((short)uVar22 < 1) {
    return;
  }
LAB_004395e0:
  iVar10 = DAT_00880354;
  local_1c0[0] = 0;
  local_1c0[1] = 0;
  local_1c0[2] = 0;
  local_1c0[4] = 0;
  local_208 = 0;
  sVar24 = 0;
  local_1c0[3] = 0;
  local_1c0[5] = 0;
  sVar8 = 0;
  if (0 < *(short *)(iVar19 + 6)) {
    sVar3 = *(short *)(iVar19 + 6);
    local_1ec = 0;
    local_1f8 = local_100;
    psVar20 = (short *)(*(int *)(iVar9 + 0x84) + 0x22);
    do {
      if ((*(byte *)(psVar20 + 3) & 0x20) != 0) {
        iVar11 = (short)(*(short *)(iVar19 + 4) + sVar8) * 0x20 + DAT_008802cc;
        uVar17 = 1 << ((byte)local_1ec & 0x1f);
        iVar15 = local_1ec >> 5;
        local_1c0[iVar15] = local_1c0[iVar15] | uVar17;
        sVar4 = *(short *)(iVar11 + 0x18);
        sVar24 = sVar24 + sVar4;
        if (*(char *)(iVar11 + 0x10) != '\0') {
          local_1c0[iVar15 + 2] = local_1c0[iVar15 + 2] | uVar17;
          sVar5 = *psVar20;
          uVar12 = 0;
          if (((sVar5 < 0) || (*(int *)(iVar9 + 0x8c) <= (int)sVar5)) ||
             (*(char *)((short)(*(short *)(iVar19 + 8) + sVar5) * 0x10 + DAT_008802c4) == '\0')) {
            puVar23 = (uint *)(psVar20 + 0x19);
            iVar11 = 3;
            do {
              uVar12 = uVar12 | *puVar23;
              puVar23 = puVar23 + 1;
              iVar11 = iVar11 + -1;
            } while (iVar11 != 0);
          }
          else {
            puVar23 = (uint *)(psVar20 + 0x1f);
            iVar11 = 3;
            do {
              uVar12 = uVar12 | *puVar23;
              puVar23 = puVar23 + 1;
              iVar11 = iVar11 + -1;
            } while (iVar11 != 0);
          }
          *local_1f8 = uVar12;
          local_208 = local_208 | uVar12;
          if (0 < sVar4) {
            local_1c0[iVar15 + 4] = local_1c0[iVar15 + 4] | uVar17;
          }
        }
      }
      puVar23 = (uint *)local_140[0];
      sVar8 = sVar8 + 1;
      local_1ec = local_1ec + 1;
      psVar20 = psVar20 + 0x74;
      local_1f8 = local_1f8 + 1;
    } while (sVar8 < sVar3);
    if ((0 < sVar24) && (local_208 != 0)) {
      if (uVar22 == 1) {
        object_get_position();
      }
      else {
        if (0 < (short)uVar22) {
          uVar12 = (uint)uVar22;
          pfVar18 = local_120;
          for (uVar17 = uVar12; uVar17 != 0; uVar17 = uVar17 - 1) {
            *pfVar18 = 3.4028235e+38;
            pfVar18 = pfVar18 + 1;
          }
          do {
            object_get_position();
            uVar12 = uVar12 - 1;
          } while (uVar12 != 0);
        }
        cVar2 = *(char *)(iVar10 + 1);
        fVar21 = local_1d4;
        if (cVar2 != '\0') {
          if (param_1 == 0xffffffff) {
            fVar21 = *(float *)(iVar10 + 8);
          }
          else {
            fVar21 = *(float *)(iVar14 + 0x14 + *(int *)(DAT_008802c8 + 0x34));
          }
        }
        while ((cVar2 != '\0' && (fVar21 != -NAN))) {
          iVar10 = *(int *)(DAT_00880360 + 0x34);
          iVar11 = ((uint)fVar21 & 0xffff) * 0x724;
          sVar8 = *(short *)(iVar11 + 0x3a + iVar10);
          fVar21 = *(float *)(iVar11 + 0x2c + iVar10);
          iVar11 = iVar11 + iVar10;
          if (((local_1c0[(int)sVar8 >> 5] & 1 << ((byte)sVar8 & 0x1f)) != 0) && (0 < (short)uVar22)
             ) {
            uVar17 = (uint)uVar22;
            pfVar16 = local_120;
            pfVar18 = local_1a8 + 2;
            do {
              fVar1 = pfVar18[-2] - *(float *)(iVar11 + 300);
              fVar7 = pfVar18[-1] - *(float *)(iVar11 + 0x130);
              fVar6 = *pfVar18 - *(float *)(iVar11 + 0x134);
              fVar1 = fVar1 * fVar1 + fVar7 * fVar7 + fVar6 * fVar6;
              if (*pfVar16 <= fVar1) {
                fVar1 = *pfVar16;
              }
              *pfVar16 = fVar1;
              pfVar18 = pfVar18 + 3;
              pfVar16 = pfVar16 + 1;
              uVar17 = uVar17 - 1;
            } while (uVar17 != 0);
          }
        }
        if ((short)uVar22 < 1) {
          return;
        }
        uVar17 = (uint)uVar22;
        pfVar18 = local_1a8;
        iVar10 = 0;
        puVar23 = local_1f0;
        do {
          if (*(float *)((int)local_120 + iVar10) < 3.4028235e+38) {
            puVar23 = *(uint **)((int)local_140 + iVar10);
            local_1dc = *pfVar18;
            local_1d8 = pfVar18[1];
            local_1d4 = pfVar18[2];
          }
          iVar10 = iVar10 + 4;
          pfVar18 = pfVar18 + 3;
          uVar17 = uVar17 - 1;
        } while (uVar17 != 0);
      }
      if (puVar23 != (uint *)0xffffffff) {
        local_1a8[0] = 3.4028235e+38;
        local_1a8[1] = 3.4028235e+38;
        local_1a8[2] = 3.4028235e+38;
        local_1a8[3] = 3.4028235e+38;
        local_1a8[4] = 3.4028235e+38;
        local_1a8[5] = 3.4028235e+38;
        local_1a8[6] = 3.4028235e+38;
        local_1a8[7] = 3.4028235e+38;
        local_1a8[8] = 3.4028235e+38;
        local_1a8[9] = 3.4028235e+38;
        local_1a8[10] = 3.4028235e+38;
        local_1a8[0xb] = 3.4028235e+38;
        local_1a8[0xc] = 3.4028235e+38;
        local_1a8[0xd] = 3.4028235e+38;
        local_1a8[0xe] = 3.4028235e+38;
        local_1a8[0xf] = 3.4028235e+38;
        local_1a8[0x10] = 3.4028235e+38;
        local_1a8[0x11] = 3.4028235e+38;
        local_1a8[0x12] = 3.4028235e+38;
        local_1a8[0x13] = 3.4028235e+38;
        local_1a8[0x14] = 3.4028235e+38;
        local_1a8[0x15] = 3.4028235e+38;
        local_1a8[0x16] = 3.4028235e+38;
        local_1a8[0x17] = 3.4028235e+38;
        local_1a8[0x18] = 3.4028235e+38;
        local_1a8[0x19] = 3.4028235e+38;
        iVar10 = *(int *)(iVar9 + 0x98);
        sVar24 = 0;
        sVar8 = -1;
        if (0 < iVar10) {
          iVar9 = *(int *)(iVar9 + 0x9c);
          iVar11 = 0;
          do {
            sVar3 = *(short *)(iVar9 + 0xc + iVar11 * 0x18);
            pfVar18 = (float *)(iVar9 + iVar11 * 0x18);
            if ((local_208 & 1 << ((byte)sVar3 & 0x1f)) != 0) {
              pfVar16 = local_1a8 + sVar3;
              fVar21 = (local_1d8 - pfVar18[1]) * (local_1d8 - pfVar18[1]) +
                       (local_1d4 - pfVar18[2]) * (local_1d4 - pfVar18[2]) +
                       (local_1dc - *pfVar18) * (local_1dc - *pfVar18);
              if (*pfVar16 <= fVar21) {
                fVar21 = *pfVar16;
              }
              *pfVar16 = fVar21;
            }
            sVar24 = sVar24 + 1;
            iVar11 = (int)sVar24;
          } while (iVar11 < iVar10);
        }
        local_1fc = 3.4028235e+38;
        local_1f8 = (uint *)0xff7fffff;
        sVar3 = -1;
        sVar24 = 0;
        if (0 < *(short *)(iVar19 + 6)) {
          local_1f0 = local_100;
          local_200 = 0;
          do {
            uVar17 = 1 << ((byte)local_200 & 0x1f);
            if ((local_1c0[(local_200 >> 5) + 2] & uVar17) != 0) {
              fVar21 = 3.4028235e+38;
              bVar13 = 0;
              pfVar18 = local_1a8;
              iVar9 = 0x1a;
              do {
                if (((*local_1f0 & 1 << (bVar13 & 0x1f)) != 0) && (*pfVar18 < fVar21)) {
                  fVar21 = *pfVar18;
                }
                bVar13 = bVar13 + 1;
                pfVar18 = pfVar18 + 1;
                iVar9 = iVar9 + -1;
              } while (iVar9 != 0);
              if (fVar21 < local_1fc) {
                local_1fc = fVar21;
                sVar8 = sVar24;
              }
              if (((local_1c0[(local_200 >> 5) + 4] & uVar17) != 0) && ((float)local_1f8 < fVar21))
              {
                local_1f8 = (uint *)fVar21;
                sVar3 = sVar24;
              }
            }
            sVar24 = sVar24 + 1;
            local_200 = local_200 + 1;
            local_1f0 = local_1f0 + 1;
          } while (sVar24 < *(short *)(iVar19 + 6));
          if (sVar8 != -1) {
            if (sVar3 != -1) {
              if (*(float *)(iVar19 + 0x68) <= 0.0) {
                fVar21 = 2.0;
              }
              else {
                fVar21 = *(float *)(iVar19 + 0x68);
              }
              if (SQRT((float)local_1f8) - fVar21 <= SQRT(local_1fc)) {
                return;
              }
            }
            fVar21 = local_1d4;
            if (*(char *)(DAT_00880354 + 1) != '\0') {
              if (param_1 == 0xffffffff) {
                fVar21 = *(float *)(DAT_00880354 + 8);
              }
              else {
                fVar21 = *(float *)(iVar14 + 0x14 + *(int *)(DAT_008802c8 + 0x34));
              }
            }
            while ((local_1d4 = fVar21, iVar9 = DAT_00880360, *(char *)(DAT_00880354 + 1) != '\0' &&
                   (local_1d4 != -NAN))) {
              iVar14 = ((uint)local_1d4 & 0xffff) * 0x724;
              fVar21 = *(float *)(iVar14 + 0x2c + *(int *)(DAT_00880360 + 0x34));
              iVar14 = iVar14 + *(int *)(DAT_00880360 + 0x34);
              sVar24 = *(short *)(iVar14 + 0x3a);
              if (((local_1c0[(int)sVar24 >> 5] & 1 << ((byte)sVar24 & 0x1f)) != 0) &&
                 (*(short *)(iVar14 + 0x3a) != sVar8)) {
                iVar19 = ((uint)local_1d4 & 0xffff) * 0x724;
                sVar24 = *(short *)(iVar19 + 0x46c + *(int *)(DAT_00880360 + 0x34));
                iVar14 = iVar19 + *(int *)(DAT_00880360 + 0x34);
                *(undefined2 *)(iVar14 + 0x3b8) = 0xffff;
                if ((sVar24 == 3) || (sVar24 == 4)) {
                  *(undefined2 *)(iVar14 + 0x46c) = 0;
                  *(undefined4 *)(iVar14 + 0x480) = 0xffffffff;
                }
                if (*(code **)(&DAT_00655278 +
                              *(short *)(*(int *)(iVar9 + 0x34) + iVar19 + 0x6c) * 0x38) !=
                    (code *)0x0) {
                  (**(code **)(&DAT_00655278 +
                              *(short *)(*(int *)(iVar9 + 0x34) + iVar19 + 0x6c) * 0x38))(local_1d4)
                  ;
                }
                if (*(char *)(iVar14 + 9) == '\0') {
                  if (*(int *)(iVar14 + 0x34) != -1) {
                    squad_remove_actor(0);
                  }
                }
                else {
                  FUN_00436990();
                }
                iVar14 = DAT_00880360;
                iVar9 = DAT_00880354;
                if (param_1 == 0xffffffff) {
                  if (*(char *)(DAT_00880354 + 1) != '\0') {
                    iVar10 = *(int *)(DAT_00880360 + 0x34) + iVar19;
                    *(undefined4 *)(iVar10 + 0x2c) = *(undefined4 *)(DAT_00880354 + 8);
                    *(float *)(iVar9 + 8) = local_1d4;
                    *(undefined1 *)(iVar10 + 9) = 1;
                    *(ushort *)(iVar10 + 0x10) = -(ushort)(*(char *)(iVar10 + 8) != '\0') & 0x5a;
                    iVar9 = *(int *)(iVar14 + 0x34);
                    sVar24 = *(short *)(iVar9 + 0x46c + iVar19);
                    iVar9 = iVar9 + iVar19;
                    *(undefined2 *)(iVar9 + 0x3b8) = 0xffff;
                    if ((sVar24 == 3) || (sVar24 == 4)) {
                      *(undefined2 *)(iVar9 + 0x46c) = 0;
                      *(undefined4 *)(iVar9 + 0x480) = 0xffffffff;
                    }
                    if (*(code **)(&DAT_00655278 +
                                  *(short *)(*(int *)(iVar14 + 0x34) + 0x6c + iVar19) * 0x38) !=
                        (code *)0x0) {
                      (**(code **)(&DAT_00655278 +
                                  *(short *)(*(int *)(iVar14 + 0x34) + 0x6c + iVar19) * 0x38))
                                (local_1d4);
                    }
                  }
                }
                else {
                  FUN_00436770(local_1d4,param_1,1);
                }
              }
            }
          }
        }
      }
    }
  }
  return;
}
#endif
