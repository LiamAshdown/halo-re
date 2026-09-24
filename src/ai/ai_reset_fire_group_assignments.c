// ai_reset_fire_group_assignments  (Ghidra: still FUN_0042c940; named for this rewrite)
// address 0x42c940, size 1358 bytes, 0 callers in this build
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: phase-4 summary ("Bulk-resets fire-group/assignment bookkeeping for a table of
//   actor groups and clears stale recognized-object linkage fields across all actors; appears
//   unused (0 callers) in this build"); out/phase4/ai_types_notes.md's own note on this address
//   ("bulk-resets fire-group bookkeeping across all actors and swarms... used for offset
//   evidence only where another function agreed, since dead code can carry a stale layout").
//   types/ai.h encounter.units_active (0xd), .unknown_2a, .first_actor (0x14); actor.mode (0x6c),
//   .team (0x3e), .next_in_encounter (0x2c), .swarm (0x06), .swarm_index (0x28),
//   .encounter_index (0x34), .squad_index (0x3a), .actor_variant_tag (0x5c),
//   .target_combat_status (0x268), .firing_position_index (0x3b8), .active_movement (0x46c);
//   prop.kind (0x24), .pair_index (0x0c), .is_parented (0x12e), .distance (0x11c); swarm
//   .component_count (0x02), .unit_index[16] (0x18); ai_globals.actors_valid (0x01),
//   .unknown_08 (0x08); types/objects.h object.parent_object (0x11c),
//   object.location_cluster_index (0x9c); team_relationship_flags (0x006b0b84, established by
//   src/ai/actor_refresh_combat_context.c and others); use_absolute_team_check (0x006f1d20,
//   src/ai/ai_recompute_all_relationship_flags.c); local_player_globals (0x0087a478,
//   src/ai/encounters_update_activation.c, +0x18 raw per that file's own precedent).
// register convention: this function takes no arguments (Ghidra shows no recognized parameters
//   and no unresolved incoming register reads either -- confirmed against objdump -d -M intel,
//   which shows every register live at function entry being written before it is read).
//
// UNSURE (structural, not just individual fields -- see ai_types_notes.md's own caution that
// this dead function "can carry a stale layout"):
//  - actor+0x270 is used here purely as a prop_data index (indexed with the 0x138 prop stride,
//    and the record it reaches is tested with prop.kind/pair_index/is_parented/distance), not
//    as the unit handle types/ai.h's actor.target_unit_index documents for that same offset
//    (established by the live function actor_choose_best_target). Kept as a local variable
//    named for what THIS function does with it (candidate_prop_index) rather than reusing the
//    header's unit-shaped name, and not renamed in types/ai.h since that name is backed by a
//    live, still-called function.
//  - the "team * 10 + 1" index into team_relationship_flags (evidence: actor_refresh_combat_context.c
//    uses the same "team_a * 10 + team_b" indexing with two live teams) fixes the second team
//    to the literal 1 here; preserved exactly, reason unknown.
//  - the very first read of the "candidate actor" variable, when ai_globals->actors_valid is
//    already true at the very first encounter processed, would read this function's local
//    "current squad actor" variable before this function ever assigns it. This is preserved by
//    seeding it to k_datum_index_none, since Ghidra's own decompilation shows no initializer
//    either; the path is only reachable if encounter 0 has an activation delay set and
//    ai_globals->actors_valid is clear on the very first check but becomes true a few
//    instructions later, which does not happen in one straight-line execution, so this is very
//    likely unreachable in practice.
//  - actor_clear_target_state and encounter_deactivate/encounter_remove_actor are not in this rewrite's three target
//    addresses; their extern declarations below are re-derived from this function's own call
//    sites (objdump), not independently verified against their own prologues.
//  - the block starting at "if (*(char *)(DAT_00880354 + 1) != '\0') { ...; actor_movement_action_cancel(); }"
//    duplicates -- inline, not by calling it -- the body of the function this codebase's
//    src/ai/actor_movement_action_cancel.c already rewrites (which is actually 0x428650, not
//    0x417a30; see that rewrite's own NAME COLLISION note and
//    src/ai/actor_movement_actions_cancel.c, written in this same session for the true
//    0x417a30). The separate, real call near the end of this function is also to 0x428650, and
//    is declared here as actor_movement_action_cancel rather than reusing that file's (collided) symbol name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern void *global_scenario;           // 0x00746f8c, UNSURE type (Scenario tag data or
                                        //   scenario_globals; only .encounter_count at +0x42c
                                        //   is used here)
extern data_array *encounter_data;      // 0x008802c8
extern data_array *actor_data;          // 0x00880360
extern data_array *prop_data;           // 0x008802c0
extern data_array *swarm_data;          // 0x0088035c
extern data_array *object_data;         // 0x008603b0
extern ai_globals *ai_globals_ptr;      // 0x00880354
extern uint8_t team_relationship_flags; // 0x006b0b84, base of the 0x2d-dword team-relationship block
extern int32_t use_absolute_team_check; // 0x006f1d20
extern void *local_player_globals;      // 0x0087a478
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern datum_index actor_new_and_attach_to_unit(char reuse_existing, datum_index unit_index,
    datum_index actor_variant_tag, uint32_t encounter_or_none, int16_t squad_index,
    char ignore_squad, datum_index exclude_actor, char start_active, uint16_t unknown_60,
    int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68); // 0x426ac0
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90
extern void actor_movement_action_cancel(datum_index actor_index); // 0x428650, blam-cc: EDI -> actor_index;
                                        //   see file header NAME COLLISION note -- this is
                                        //   src/ai/actor_movement_action_cancel.c's function,
                                        //   declared here under its raw name to avoid the
                                        //   collision with the true 0x417a30 symbol
extern void encounter_remove_actor(datum_index actor_index, uint32_t flag); // 0x436620, blam-cc:
                                        //   EAX -> actor_index, stack -> flag; UNSURE, not yet
                                        //   rewritten, re-derived from this call site only
extern void encounter_deactivate(void);     // 0x437870, no visible argument; UNSURE, not yet rewritten
extern void actor_clear_target_state(datum_index actor_index); // 0x4286c0, blam-cc: stack -> actor_index;
                                        //   UNSURE, not yet rewritten, re-derived from this call
                                        //   site only
extern void object_delete_unparented(void); // 0x4f5aa0, no visible argument
extern void object_delete_recursive(datum_index object_index, uint32_t flag); // 0x4f59d0

// Walks every active, delayed encounter's member chain looking for a squad actor whose swarm
// still has a live, uncommitted (not-recently-recognized) prop within threat range; if the
// squad's swarm has any such members that are NOT already recognized, it spawns replacement
// actors for the recognized ones (deleting the originals when the replacement fails), rebuilds
// the squad's own recognized-object linkage, deactivates the squad, and re-links it onto the
// front of ai_globals' unassigned-actor list. Once every encounter has been swept, it also
// walks that same unassigned list end to end and clears three stale recognized-object fields
// on every prop belonging to each unassigned actor's swarm. Always returns nothing; has no
// callers in this build.
void ai_reset_fire_group_assignments(void)
{
    void *scenario = global_scenario;
    int16_t encounter_index = 0;

    if (0 < *(int32_t *)((uint8_t *)scenario + 0x42c)) {
        int32_t previous_encounter_index = 0;

        do {
            encounter *enc = &((encounter *)encounter_data->data)[(uint16_t)encounter_index];

            if (enc->units_active != 0 && enc->unknown_2a > 0) {
                datum_index actor_index = (datum_index)k_datum_index_none; // UNSURE seed, see header

                if (ai_globals_ptr->actors_valid != 0) {
                    if (previous_encounter_index == -1) {
                        actor_index = ai_globals_ptr->unknown_08;
                    } else {
                        actor_index = enc->first_actor;
                    }
                }

                for (;;) {
                    actor *self;
                    datum_index next_in_encounter;
                    datum_index candidate_prop_index; // UNSURE: see file header on actor+0x270

                    if (ai_globals_ptr->actors_valid == 0 || actor_index == (datum_index)k_datum_index_none) {
                        break;
                    }

                    self = &((actor *)actor_data->data)[actor_index & 0xffff];
                    candidate_prop_index = self->target_unit_index; // UNSURE name, see header
                    next_in_encounter = self->next_in_encounter;

                    if (candidate_prop_index == (datum_index)k_datum_index_none ||
                        self->target_combat_status < 5) {
                        // No (recent enough) engaged target: fall back to a team check, and if
                        // that passes, scan the actor's OWN prop list (not the target's) for any
                        // parented prop that is either a higher threat grade or already close.
                        int16_t team = self->team;
                        int32_t team_index = team * 10 + 1; // UNSURE: second team fixed to 1, see header
                        char team_matches;
                        char any_prop_matches;
                        datum_index prop_index;

                        if (use_absolute_team_check == 0) {
                            if (team < 0 || team >= 10) {
                                actor_index = next_in_encounter;
                                continue;
                            }
                            team_matches = ((1u << (team_index & 0x1f)) &
                                            *(uint32_t *)((uint8_t *)&team_relationship_flags + 0xa4 +
                                                          (team_index >> 5) * 4)) != 0;
                        } else {
                            team_matches = (team == 1);
                        }

                        if (!team_matches) {
                            actor_index = next_in_encounter;
                            continue;
                        }

                        any_prop_matches = 0;
                        prop_index = self->first_prop;
                        while (prop_index != (datum_index)k_datum_index_none) {
                            prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
                            prop_index = p->next_in_actor;
                            if (p->is_parented != 0 &&
                                (1 < p->unknown_32 || p->distance < 3.0f)) {
                                any_prop_matches = 1;
                            }
                        }
                        if (!any_prop_matches) {
                            actor_index = next_in_encounter;
                            continue;
                        }
                    } else {
                        // Has a recent enough engaged target: check that target's own prop
                        // (following its pair_index when it is a shared/vault kind) directly.
                        prop *candidate = &((prop *)prop_data->data)[candidate_prop_index & 0xffff];
                        if (3 < candidate->kind && candidate->kind < 6) {
                            candidate = &((prop *)prop_data->data)[candidate->pair_index & 0xffff];
                        }
                        if (!(candidate->is_parented != 0 && self->unknown_88 != -1 &&
                              self->unknown_88 < 0x5a && candidate->distance < 10.0f)) {
                            actor_index = next_in_encounter;
                            continue;
                        }
                    }

                    // Found a squad actor eligible for swarm replacement: scan its swarm
                    // members for any not already "recognized" (per local_player_globals+0x18's
                    // per-cluster bitmask against the member's root object's cluster).
                    {
                        if (self->swarm != 0) {
                            if (self->swarm_index == (datum_index)k_datum_index_none) {
                                // The original jumps straight back to the member-chain loop
                                // here (LAB_0042c9c6), so a swarm actor with no swarm record
                                // skips the whole re-link tail below, it does not fall into it.
                                actor_index = next_in_encounter;
                                continue;
                            }
                            {
                                swarm *sw = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
                                int16_t member_count = sw->component_count;

                                if (0 < member_count) {
                                    datum_index excess_units[16];
                                    int16_t excess_count = 0;
                                    int32_t i;

                                    for (i = 0; i < member_count; i++) {
                                        datum_index unit_index = sw->unit_index[i];
                                        datum_index root_index = (datum_index)k_datum_index_none;

                                        while (unit_index != (datum_index)k_datum_index_none) {
                                            root_index = unit_index;
                                            unit_index = ((object_header *)object_data->data)[unit_index & 0xffff]
                                                             .data->parent_object;
                                        }

                                        {
                                            object *root = ((object_header *)object_data->data)[root_index & 0xffff].data;
                                            int16_t cluster = root->location_cluster_index;

                                            if (cluster == -1 ||
                                                (*(uint32_t *)((uint8_t *)local_player_globals + 0x18 +
                                                               (cluster >> 5) * 4) &
                                                 (1u << (cluster & 0x1f))) == 0) {
                                                excess_units[excess_count] = sw->unit_index[i];
                                                excess_count++;
                                            }
                                        }
                                    }

                                    if (excess_count != 0 && excess_count != member_count) {
                                        for (i = 0; i < excess_count; i++) {
                                            datum_index unit_index = excess_units[i];
                                            datum_index new_actor;

                                            actor_remove_from_unit_cluster(actor_index, unit_index);
                                            new_actor = actor_new_and_attach_to_unit(
                                                1, unit_index, self->actor_variant_tag,
                                                (uint32_t)self->encounter_index, self->squad_index,
                                                0, actor_index, 0, 2, 0, 0xffffffff, 0);

                                            if (new_actor == (datum_index)k_datum_index_none) {
                                                object *unit_object =
                                                    ((object_header *)object_data->data)[unit_index & 0xffff].data;
                                                // The original calls object_delete_unparented()
                                                // AND THEN object_delete_recursive() when
                                                // network_role == 0 (no separate "goto" between
                                                // them for that case), but skips
                                                // object_delete_recursive entirely when
                                                // network_role is neither 0 nor 3.
                                                if (unit_object->network_role == 0) {
                                                    object_delete_unparented();
                                                }
                                                if (unit_object->network_role == 0 ||
                                                    unit_object->network_role == 3) {
                                                    object_delete_recursive(unit_index, 0);
                                                }
                                            }
                                        }
                                    } else if (excess_count == member_count) {
                                        actor_index = next_in_encounter;
                                        continue;
                                    }
                                    // excess_count == 0: nothing to replace, fall through.
                                }
                            }
                        }
                    }

                    self->unknown_30 = (datum_index)previous_encounter_index;
                    self->unknown_38 = self->squad_index;

                    // Inline duplicate of actor_movement_action_cancel's body (0x428650 -- see file header):
                    // cancel the active movement action and invoke the current mode's fifth
                    // (unnamed) per-mode procedure slot.
                    {
                        int16_t active_type = self->active_movement.type;
                        self->firing_position_index = -1;
                        if (active_type == 3 || active_type == 4) {
                            self->active_movement.type = 0;
                            self->active_movement.extra = 0xffffffff;
                        }
                        {
                            void (*mode_proc)(datum_index) =
                                *(void (**)(datum_index))((uint8_t *)&actor_mode_definitions[self->mode] + 0x24);
                            if (mode_proc != 0) {
                                mode_proc(actor_index);
                            }
                        }
                    }

                    encounter_remove_actor(actor_index, 0);

                    if (ai_globals_ptr->actors_valid != 0) {
                        uint8_t was_active = self->active;
                        self->next_in_encounter = ai_globals_ptr->unknown_08;
                        ai_globals_ptr->unknown_08 = actor_index;
                        self->unknown_09 = 1;
                        // UNSURE: actor+0x10 (unknown_10, a 2-byte field with no established
                        // name) reproduces the original's `-(active != 0) & 0x5a` idiom, i.e.
                        // 0x5a when the actor was already active, else 0. Read before
                        // unknown_09 above is set, matching the original's instruction order.
                        *(uint16_t *)((uint8_t *)self + 0x10) = (was_active != 0) ? 0x5a : 0;
                        actor_movement_action_cancel(actor_index); // blam-cc: EDI -> actor_index
                    }

                    actor_index = next_in_encounter;
                }
            }

            enc->activation_delay = 0;
            encounter_deactivate();
            encounter_index = encounter_index + 1;
            previous_encounter_index = (int32_t)(int16_t)encounter_index;
        } while (previous_encounter_index < *(int32_t *)((uint8_t *)scenario + 0x42c));
    }

    // Second pass: walk the unassigned-actor list end to end (ai_globals->unknown_08, chained
    // through actor.next_in_encounter) and, for every actor with a swarm, clear three stale
    // recognized-object fields (prop.unknown_100/0xfc/0xec) on every prop reachable from
    // actor.first_prop.
    {
        datum_index actor_index = ai_globals_ptr->unknown_08;

        while (actor_index != (datum_index)k_datum_index_none) {
            actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
            datum_index next_actor;

            actor_clear_target_state(actor_index);

            next_actor = self->next_in_encounter;
            {
                datum_index prop_index = self->first_prop;
                while (prop_index != (datum_index)k_datum_index_none) {
                    prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
                    prop_index = p->next_in_actor;
                    *(int16_t *)((uint8_t *)p + 0x100) = -1; // UNSURE offset, no header name past prop+0xfc
                    *(int32_t *)((uint8_t *)p + 0xfc) = -1;  // UNSURE offset
                    *(int32_t *)((uint8_t *)p + 0xec) = -1;  // UNSURE offset (prop.path_surface_index per
                                                             //   header, but written -1 here alongside two
                                                             //   otherwise-unnamed fields, so kept raw)
                }
            }
            actor_index = next_actor;
        }
    }
}

#if 0
Original Ghidra decompilation (0x42c940) -- full listing via `python tools/pack.py 0x42c940`:

void FUN_0042c940(void)

{
  short sVar1;
  ushort uVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  ushort uVar11;
  uint uVar12;
  uint *puVar13;
  ushort uVar14;
  int iVar15;
  char cVar16;
  uint local_68;
  uint local_60;
  uint local_44;
  uint local_40 [16];

  iVar6 = global_scenario;
  uVar14 = 0;
  if (0 < *(int *)(global_scenario + 0x42c)) {
    local_68 = 0;
    do {
      iVar9 = *(int *)(DAT_008802c8 + 0x34);
      if ((*(char *)((uint)uVar14 * 0x6c + 0xd + iVar9) != '\0') &&
         (0 < *(short *)((uint)uVar14 * 0x6c + iVar9 + 0x2a))) {
        uVar3 = local_44;
        if (*(char *)(DAT_00880354 + 1) != '\0') {
          if (local_68 == 0xffffffff) {
            uVar3 = *(uint *)(DAT_00880354 + 8);
          }
          else {
            uVar3 = *(uint *)((local_68 & 0xffff) * 0x6c + 0x14 + iVar9);
          }
        }
LAB_0042c9c6:
        while ((local_44 = uVar3, *(char *)(DAT_00880354 + 1) != '\0' && (local_44 != 0xffffffff)))
        {
          iVar9 = *(int *)(DAT_00880360 + 0x34);
          uVar12 = local_44 & 0xffff;
          iVar15 = uVar12 * 0x724;
          uVar10 = *(uint *)(iVar15 + 0x270 + iVar9);
          uVar3 = *(uint *)(iVar15 + 0x2c + iVar9);
          iVar15 = iVar15 + iVar9;
          bVar5 = false;
          if ((uVar10 == 0xffffffff) || (*(short *)(iVar15 + 0x268) < 5)) {
            sVar1 = *(short *)(iVar15 + 0x3e);
            if (DAT_006f1d20 == 0) goto LAB_0042cac2;
            cVar16 = sVar1 != 1;
            goto LAB_0042cb01;
          }
          iVar9 = *(int *)(DAT_008802c0 + 0x34);
          iVar8 = (uVar10 & 0xffff) * 0x138;
          sVar1 = *(short *)(iVar8 + 0x24 + iVar9);
          iVar8 = iVar8 + iVar9;
          if ((3 < sVar1) && (sVar1 < 6)) {
            iVar8 = (*(uint *)(iVar8 + 0xc) & 0xffff) * 0x138 + iVar9;
          }
          if ((((*(char *)(iVar8 + 0x12e) != '\0') && (*(int *)(iVar15 + 0x88) != -1)) &&
              (*(int *)(iVar15 + 0x88) < 0x5a)) && (*(float *)(iVar8 + 0x11c) < 10.0))
          goto LAB_0042cb6e;
        }
      }
      *(undefined2 *)((local_68 & 0xffff) * 0x6c + 0xe + *(int *)(DAT_008802c8 + 0x34)) = 0;
      squad_deactivate();
      uVar14 = uVar14 + 1;
      local_68 = (uint)(short)uVar14;
    } while ((int)local_68 < *(int *)(iVar6 + 0x42c));
  }
  uVar3 = *(uint *)(DAT_00880354 + 8);
  iVar6 = DAT_00880360;
  while (uVar3 != 0xffffffff) {
    iVar15 = (uVar3 & 0xffff) * 0x724;
    uVar10 = *(uint *)(iVar15 + 0x2c + *(int *)(iVar6 + 0x34));
    FUN_004286c0(uVar3);
    iVar6 = DAT_00880360;
    iVar9 = DAT_008802c0;
    uVar12 = *(uint *)(iVar15 + 0x50 + *(int *)(DAT_00880360 + 0x34));
    while (uVar3 = uVar10, uVar12 != 0xffffffff) {
      iVar15 = (uVar12 & 0xffff) * 0x138 + *(int *)(iVar9 + 0x34);
      uVar12 = *(uint *)(iVar15 + 8);
      *(undefined2 *)(iVar15 + 0x100) = 0xffff;
      *(undefined4 *)(iVar15 + 0xfc) = 0xffffffff;
      *(undefined4 *)(iVar15 + 0xec) = 0xffffffff;
    }
  }
  return;
LAB_0042cac2:
  if ((-1 < sVar1) && (sVar1 < 10)) {
    iVar8 = sVar1 * 10 + 1;
    cVar16 = '\x01' - ((1 << ((byte)iVar8 & 0x1f) &
                       *(uint *)(DAT_006b0b84 + 0xa4 + (iVar8 >> 5) * 4)) != 0);
LAB_0042cb01:
    if (cVar16 == '\0') {
      uVar10 = *(uint *)(uVar12 * 0x724 + 0x50 + iVar9);
      while (uVar10 != 0xffffffff) {
        iVar9 = *(int *)(DAT_008802c0 + 0x34);
        iVar8 = (uVar10 & 0xffff) * 0x138;
        uVar10 = *(uint *)(iVar8 + 8 + iVar9);
        if ((*(char *)(iVar8 + 0x12e + iVar9) != '\0') &&
           ((1 < *(short *)(iVar8 + iVar9 + 0x32) || (*(float *)(iVar8 + iVar9 + 0x11c) < 3.0)))) {
          bVar5 = true;
        }
      }
      if (bVar5) {
LAB_0042cb6e:
        if (*(char *)(iVar15 + 6) != '\0') {
          if (*(uint *)(iVar15 + 0x28) == 0xffffffff) goto LAB_0042c9c6;
          iVar9 = (*(uint *)(iVar15 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
          uVar2 = *(ushort *)(iVar9 + 2);
          uVar11 = 0;
          if (0 < (short)uVar2) {
            iVar8 = *(int *)(DAT_008603b0 + 0x34);
            puVar13 = (uint *)(iVar9 + 0x18);
            local_60 = (uint)uVar2;
            do {
              uVar7 = *puVar13;
              uVar10 = 0xffffffff;
              while (uVar4 = uVar7, uVar4 != 0xffffffff) {
                uVar10 = uVar4;
                uVar7 = *(uint *)(*(int *)(iVar8 + 8 + (uVar4 & 0xffff) * 0xc) + 0x11c);
              }
              iVar9 = *(int *)(iVar8 + 8 + (uVar10 & 0xffff) * 0xc);
              if ((*(short *)(iVar9 + 0x9c) == -1) ||
                 (sVar1 = *(short *)(iVar9 + 0x9c),
                 (*(uint *)(DAT_0087a478 + 0x18 + ((int)sVar1 >> 5) * 4) & 1 << ((byte)sVar1 & 0x1f)
                 ) == 0)) {
                iVar9 = (int)(short)uVar11;
                uVar11 = uVar11 + 1;
                local_40[iVar9] = *puVar13;
              }
              puVar13 = puVar13 + 1;
              local_60 = local_60 - 1;
            } while (local_60 != 0);
            if (uVar11 != 0) {
              if (uVar11 == uVar2) goto LAB_0042c9c6;
              if (0 < (short)uVar11) {
                local_60 = (uint)uVar11;
                puVar13 = local_40;
                do {
                  uVar10 = *puVar13;
                  actor_remove_from_unit_cluster(uVar10);
                  iVar9 = actor_new_and_attach_to_unit
                                    (1,uVar10,*(undefined4 *)(iVar15 + 0x5c),
                                     *(undefined4 *)(iVar15 + 0x34),*(undefined2 *)(iVar15 + 0x3a),0
                                     ,local_44,0,2,0,0xffffffff,0);
                  if (iVar9 == -1) {
                    iVar9 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                             (uVar10 & 0xffff) * 0xc) + 4);
                    if (iVar9 == 0) {
                      object_delete_unparented();
                    }
                    else if (iVar9 != 3) goto LAB_0042cce1;
                    object_delete_recursive(uVar10,0);
                  }
LAB_0042cce1:
                  puVar13 = puVar13 + 1;
                  local_60 = local_60 - 1;
                } while (local_60 != 0);
              }
            }
          }
        }
        *(uint *)(iVar15 + 0x30) = local_68;
        iVar8 = DAT_00880360;
        *(undefined2 *)(iVar15 + 0x38) = *(undefined2 *)(iVar15 + 0x3a);
        iVar9 = *(int *)(iVar8 + 0x34);
        iVar15 = uVar12 * 0x724;
        sVar1 = *(short *)(iVar9 + 0x46c + iVar15);
        iVar9 = iVar9 + iVar15;
        *(undefined2 *)(iVar9 + 0x3b8) = 0xffff;
        if ((sVar1 == 3) || (sVar1 == 4)) {
          *(undefined2 *)(iVar9 + 0x46c) = 0;
          *(undefined4 *)(iVar9 + 0x480) = 0xffffffff;
        }
        if (*(code **)(&DAT_00655278 + *(short *)(*(int *)(iVar8 + 0x34) + 0x6c + iVar15) * 0x38) !=
            (code *)0x0) {
          (**(code **)(&DAT_00655278 + *(short *)(*(int *)(iVar8 + 0x34) + 0x6c + iVar15) * 0x38))
                    (local_44);
        }
        squad_remove_actor(0);
        iVar9 = DAT_00880354;
        if (*(char *)(DAT_00880354 + 1) != '\0') {
          iVar15 = *(int *)(DAT_00880360 + 0x34) + iVar15;
          *(undefined4 *)(iVar15 + 0x2c) = *(undefined4 *)(DAT_00880354 + 8);
          *(uint *)(iVar9 + 8) = local_44;
          *(undefined1 *)(iVar15 + 9) = 1;
          *(ushort *)(iVar15 + 0x10) = -(ushort)(*(char *)(iVar15 + 8) != '\0') & 0x5a;
          FUN_00428650();
        }
      }
    }
  }
  goto LAB_0042c9c6;
}
#endif
