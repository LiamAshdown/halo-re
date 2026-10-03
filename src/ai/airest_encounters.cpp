#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/ai/airest_encounters.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/ai_constants.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern double fabs(double x);
extern game_engine_definition *current_game_engine;
extern game_time_globals *game_time;
extern uint8_t *actor_type_procs[];
extern int16_t ai_vocalization_line_table[4];
extern float ticks_per_second;
extern int32_t __ftol(double value);
extern team_pair_globals *team_pair_data;
extern data_array *player_data;
extern void *ai_actor_mode_dispatch_table;
extern double sqrt(double x);
extern float k_random_scale_65536;
extern player_globals *local_player_globals;
}

namespace halo::ai {

/**
 * Nudges one encounter squad's bias field (unknown_08) up or down by a random +-1 step, clamped so it
 * never drifts the global rate (ai_globals.unknown_0c) below a floor derived from the squad's own current
 * bias, and keeps the two in sync.
 *
 * @address 0x42a9d0
 */
uint8_t EncounterView::drift_zone_bias(int16_t squad_offset, float bias)
{
    datum_index encounter_index = handle;
    uint8_t hit;
    encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    encounter_squad_state *squad = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + squad_offset)];
    float floor = halo::ai::globals().state->major_upgrade_error * -0.33333334f;
    float step;

    if ((float)fabs((double)floor) <= (float)fabs((double)(-squad->major_upgrade_error))) {
        floor = -squad->major_upgrade_error;
    }

    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    hit = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f < floor + bias;
    step = (float)hit - bias;

    squad->major_upgrade_error = step + squad->major_upgrade_error;
    halo::ai::globals().state->major_upgrade_error = step + halo::ai::globals().state->major_upgrade_error;
    return hit;
}

/**
 * Behaviour of ai encounter record recent zone, moved unchanged from the original free function.
 *
 * @address 0x437820
 */
int32_t EncounterView::record_recent_zone(int16_t zone_id)
{
    datum_index encounter_index = handle;
    encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    int16_t *count = &enc->activation_link_count;
    int16_t *entries = enc->activation_link;
    int16_t i;

    for (i = 0; i < *count; i++) {
        if (entries[i] == zone_id) {
            return 1;
        }
    }

    if (*count > 2) {
        return 0;
    }

    entries[*count] = zone_id;
    *count = *count + 1;
    return 1;
}

/**
 * Behaviour of ai encounter stamp team from unit, moved unchanged from the original free function.
 *
 * @address 0x436710
 */
void EncounterView::stamp_team_from_unit(datum_index unit_index)
{
    datum_index encounter_index = handle;
    encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];

    if (halo::game::globals().current_engine == 0 && enc->team == 0) {
        object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
        enc->team = *(int16_t *)((uint8_t *)header->data + 0xb8);
        if (enc->activation_tick != (datum_index)k_datum_index_none) {
            halo::ai::ai_recompute_all_relationship_flags();
        }
    }
}

/**
 * Behaviour of ai release actors and swarms, moved unchanged from the original free function.
 *
 * @address 0x428ea0
 */
void Encounters::release_actors_and_swarms()
{
    actor_iterator_state iterator;
    actor *a;
    uint8_t is_dead = 0;

    halo::ai::globals().state->stagger_threshold = halo::ai::globals().state->stagger_highest;
    halo::ai::globals().state->stagger_highest = 0;
    halo::ai::globals().state->stagger_claimed = 0;


    iterator.filter_array = halo::ai::globals().encounter_data;
    iterator.next_index = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
    iterator.encounterless_done = 0;
    iterator.active = 1;
    iterator.actor_index = -1;
    iterator.next_actor_index = -1;

    a = halo::ai::actor_iterator_next(&iterator);
    while (a != 0) {
        datum_index actor_index = iterator.actor_index /* the full handle, salt included */;

        if (a->swarm_pending == 0) {
            if (a->awareness_level > 0) {
                halo::ai::actor_update_activation_state(actor_index);
            }
        } else {
            halo::ai::actor_delete_or_release_unit(actor_index, is_dead);
        }
        a = halo::ai::actor_iterator_next(&iterator);
    }
}

/**
 * Each match goes to actor_delete_or_release_unit(actor, BL).
 *
 * @address 0x42ab00
 */
void EncounterView::release_actors_filtered(int32_t platoon_index, int32_t squad_index, uint8_t is_dead)
{
    datum_index encounter_index = handle;
    if (halo::ai::globals().state->actors_valid == 0) {
        return;
    }

    if (encounter_index == (datum_index)k_datum_index_none) {
        actor_iterator_state iterator;

        halo::ai::actor_iterator_new(&iterator, 0);
        while (halo::ai::actor_iterator_next(&iterator) != 0) {
            halo::ai::actor_delete_or_release_unit(iterator.actor_index, is_dead);
        }
    } else {
        datum_index cursor[3];
        datum_index actor_index;

        halo::ai::ai_reference_actor_iterator_init_cursor((int32_t)encounter_index, cursor);
        actor_index = cursor[2];
        while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
            actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
            datum_index next = a->next_in_encounter;

            if ((platoon_index == -1 || (int32_t)a->platoon_index == platoon_index) &&
                (squad_index == -1 || (int32_t)a->squad_index == squad_index)) {
                halo::ai::actor_delete_or_release_unit(actor_index, is_dead);
            }
            actor_index = next;
        }
    }
}

/**
 * Formats a status string describing what is about to be released, releases it, and reports through
 * *has_more whether entries remain.
 *
 * @address 0x42ae50
 */
int32_t Encounters::release_inactive_encounters(char *buffer, uint8_t *has_more, int16_t *state)
{
    int16_t *entry;
    uint32_t index;
    ScenarioEncounter *scenario_encounter;
    encounter *runtime_encounter;
    actor *self;
    char *path;
    char *file_name;
    int32_t result;

    result = 0;
    if (state[1] < state[0]) {
        entry = state + state[1] * 6 + 2;
        index = *(uint32_t *)(entry + 2);
        if ((char)*entry == '\0') {
            scenario_encounter = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[index & halo::k_slot_mask];
            runtime_encounter = &((encounter *)halo::ai::globals().encounter_data->data)[index & halo::k_slot_mask];
            sprintf(buffer, "encounter %s (%d units)", scenario_encounter->name.string,
                    runtime_encounter->living_count);
            halo::ai::ai_release_actors_filtered((datum_index)index, -1, -1, 1);
        } else {
            self = &((actor *)halo::ai::globals().actor_data->data)[index & halo::k_slot_mask];
            path = halo::cache::globals().tag_instances[(int16_t)self->actor_variant_tag].path;
            file_name = strrchr(path, '\\');
            if (file_name != 0) {
                file_name = file_name + 1;
            } else {
                file_name = path;
            }
            sprintf(buffer, "encounterless-actor %s", file_name);
            halo::ai::actor_delete_or_release_unit(index, 1);
        }
        state[1] = state[1] + 1;
        result = (result & 0xffffff00) | 1;
    }
    *has_more = (uint8_t)(state[1] < state[0]);
    return result;
}

/**
 * Behaviour of ai release inactive swarms, moved unchanged from the original free function.
 *
 * @address 0x42abd0
 */
int Encounters::release_inactive_swarms(char *buffer, uint8_t *has_more)
{
    actor_iterator_state iterator;
    actor *a;
    uint8_t is_dead = 0;
    int16_t total = 0;

    is_dead = 1;

    iterator.filter_array = halo::ai::globals().encounter_data;
    iterator.next_index = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
    iterator.encounterless_done = 0;
    iterator.active = 0;
    iterator.actor_index = -1;
    iterator.next_actor_index = -1;

    a = halo::ai::actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->swarm != 0 && a->active == 0 && a->deactivation_time != (datum_index)k_datum_index_none) {
            datum_index actor_index = iterator.actor_index /* the full handle, salt included */;
            total = total + a->cluster_count;
            halo::ai::actor_delete_or_release_unit(actor_index, is_dead);
        }
        a = halo::ai::actor_iterator_next(&iterator);
    }

    sprintf(buffer, "%d swarm units", (int)total);
    *has_more = 0;
    return total > 0;
}

/**
 * Behaviour of ai squad find best matching member, moved unchanged from the original free function.
 *
 * @address 0x4333d0
 */
int32_t Encounters::find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index, const Actor *requested_actor_data, const ActorVariant *requested_actor_variant_data, char match_by_index)
{
    ScenarioEncounter *encounter_definition;
    ai_reference_squad_iterator iterator;
    encounter_squad_state *state;
    int32_t best_by_index = -1;
    int32_t best_by_actor = -1;
    int32_t best_by_variant = -1;
    int32_t best_by_type = -1;
    int32_t first_any = -1;

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[packed_reference & halo::k_slot_mask];

    halo::ai::ai_reference_squad_iterator_new(packed_reference, &iterator);
    state = halo::ai::ai_reference_squad_iterator_next(&iterator);
    if (state != 0) {
        do {
            ScenarioSquad *squad = &((ScenarioSquad *)encounter_definition->squads.pointer)[iterator.cursor];
            int16_t actor_palette_index = (int16_t)squad->actor_type;
            ActorVariant *actor_variant_data = 0;
            Actor *squad_actor_data = 0;

            if (actor_palette_index >= 0 && actor_palette_index < halo::scenario::globals().scenario->actor_palette.count) {
                TagDependency *entry =
                    &((TagDependency *)halo::scenario::globals().scenario->actor_palette.pointer)[actor_palette_index];
                datum_index actor_variant_tag = *(datum_index *)&entry->tag_id;
                if (actor_variant_tag != (datum_index)k_datum_index_none &&
                    halo::cache::globals().tag_instances[actor_variant_tag & halo::k_slot_mask].group_tag == 0x61637476 /* 'actv' */) {
                    actor_variant_data = halo::ai::tag_data<ActorVariant>(actor_variant_tag);
                    if (static_cast<uint32_t>(halo::ai::tag_handle(actor_variant_data->actor_definition)) != (uint32_t)k_datum_index_none) {
                        datum_index actor_tag = halo::ai::tag_handle(actor_variant_data->actor_definition);
                        squad_actor_data = halo::ai::tag_data<Actor>(actor_tag);
                    }
                }
            }

            if (best_by_index == -1 && match_by_index != 0 && requested_squad_index == iterator.cursor) {
                best_by_index = iterator.cursor;
            }
            if (best_by_actor == -1 && requested_actor_data != 0 && squad_actor_data != 0 &&
                requested_actor_data == squad_actor_data) {
                best_by_actor = iterator.cursor;
            }
            if (best_by_variant == -1 && requested_actor_variant_data != 0 && actor_variant_data != 0 &&
                requested_actor_variant_data == actor_variant_data) {
                best_by_variant = iterator.cursor;
            }
            if (best_by_type == -1 && requested_actor_data != 0 && squad_actor_data != 0 &&
                *(int16_t *)(requested_actor_data + 0x14) == squad_actor_data->type) {
                best_by_type = iterator.cursor;
            }
            if (first_any == -1) {
                first_any = iterator.cursor;
            }

            state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        } while (state != 0);

        if (best_by_index != -1) return best_by_index;
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

/**
 * Behaviour of ai squad priority compare, moved unchanged from the original free function.
 *
 * @address 0x42ac90
 */
int __cdecl Encounters::priority_compare(const ai_priority_target_record *record_a, const ai_priority_target_record *record_b)
{
    if (record_b->priority < record_a->priority) {
        return 1;
    }
    if (record_a->priority < record_b->priority) {
        return -1;
    }
    if (record_b->tiebreak < record_a->tiebreak) {
        return -1;
    }
    return record_a->tiebreak < record_b->tiebreak;
}

/**
 * Behaviour of ai squad resolve actor type, moved unchanged from the original free function.
 *
 * @address 0x4374a0
 */
int16_t Encounters::resolve_actor_type(ScenarioSquad *squad)
{
    int16_t actor_palette_index = (int16_t)squad->actor_type;

    if (actor_palette_index >= 0 && actor_palette_index < halo::scenario::globals().scenario->actor_palette.count) {
        TagDependency *entry = &((TagDependency *)halo::scenario::globals().scenario->actor_palette.pointer)[actor_palette_index];
        datum_index actor_variant_tag = *(datum_index *)&entry->tag_id;
        if (actor_variant_tag != (datum_index)k_datum_index_none) {
            ActorVariant *actor_variant_data = halo::ai::tag_data<ActorVariant>(actor_variant_tag);
            datum_index actor_definition_tag = halo::ai::tag_handle(actor_variant_data->actor_definition);
            if (actor_definition_tag != (datum_index)k_datum_index_none) {
                Actor *actor_tag_data = halo::ai::tag_data<Actor>(actor_definition_tag);
                return actor_tag_data->type;
            }
        }
    }
    return 0xe;
}

/**
 * A team change recomputes the relationship flags, then encounters_recompute_dirty.
 *
 * @address 0x433590
 */
void Encounters::merge(uint32_t source_reference, uint32_t target_encounter_index, char notify, char is_platoon_merge)
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
    source_index = source_reference & halo::k_slot_mask;
    target_index = target_reference & halo::k_slot_mask;

    target_enc = &((encounter *)halo::ai::globals().encounter_data->data)[target_index];
    source_enc = &((encounter *)halo::ai::globals().encounter_data->data)[source_index];
    target_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[target_index];
    source_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[source_index];
    merging_into_self = (uint8_t)(source_index == target_index);

    for (i = 0; i < 64; i++) {
        remap[i] = -1;
    }

    halo::ai::ai_reference_squad_iterator_new(source_reference, &iterator);
    for (state = halo::ai::ai_reference_squad_iterator_next(&iterator); state != 0;
         state = halo::ai::ai_reference_squad_iterator_next(&iterator)) {
        ScenarioSquad *squad;
        int16_t palette_index;
        ActorVariant *variant_data = 0;
        Actor *actor_tag_data = 0;

        if (!(state->living_count > 0) && source_enc->squads_carried_over == 0) {
            continue;
        }
        squad = (ScenarioSquad *)(*(uint8_t **)&source_definition->squads.pointer + iterator.cursor * 0xe8);
        palette_index = static_cast<int16_t>(squad->actor_type);
        if (palette_index >= 0 && (int32_t)palette_index < *(int32_t *)((uint8_t *)halo::scenario::globals().scenario + 0x420)) {
            uint8_t *entry = *(uint8_t **)((uint8_t *)halo::scenario::globals().scenario + 0x424) + palette_index * 0x10;
            datum_index variant_tag = *(datum_index *)(entry + 0xc);

            if (variant_tag != (datum_index)k_datum_index_none &&
                halo::cache::globals().tag_instances[(int16_t)variant_tag].group_tag == 0x61637476 /* 'actv' */) {
                datum_index actor_tag;

                variant_data = halo::ai::tag_data<ActorVariant>(variant_tag);
                actor_tag = halo::ai::tag_handle(variant_data->actor_definition);
                if (actor_tag != (datum_index)k_datum_index_none) {
                    actor_tag_data = halo::ai::tag_data<Actor>(actor_tag);
                }
            }
        }
        remap[iterator.cursor] = (int16_t)halo::ai::ai_squad_find_best_matching_member(target_reference, (int16_t)iterator.cursor,
            actor_tag_data, variant_data, (char)merging_into_self);
    }

    halo::ai::ai_reference_actor_iterator_init_cursor((int32_t)source_index, cursor);
    actor_index = cursor[2];
    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
        datum_index current = actor_index;
        int16_t squad_index = a->squad_index;
        int16_t remapped = remap[squad_index];

        actor_index = a->next_in_encounter;
        if (remapped == -1 || (merging_into_self && remapped == squad_index)) {
            continue;
        }
        halo::ai::actor_reset_squad_link_for_type_change(current, (datum_index)target_index, remapped);
        if (notify != 0) {
            datum_index unit_index = ((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask].unit_index;

            if (unit_index != (datum_index)k_datum_index_none) {
                halo::ai::ai_communication_broadcast(is_platoon_merge != 0 ? 0x16 : 0x17, unit_index,
                    (datum_index)k_datum_index_none, -1, (datum_index)k_datum_index_none,
                    (datum_index)k_datum_index_none, 0);
            }
        }
    }

    if (source_enc->squads_carried_over != 0) {
        actor_iterator_state all;
        actor *a;

        halo::ai::actor_iterator_new(&all, 0);
        while ((a = halo::ai::actor_iterator_next(&all)) != 0) {
            uint8_t *raw = (uint8_t *)a;
            int16_t squad_index;
            int16_t remapped;

            if ((*(uint32_t *)(raw + 0x44) & halo::k_slot_mask) != source_index) {
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
            source_enc->squads_carried_over = 0;
            target_enc->squads_carried_over = 1;
        }
    }

    actor_index = halo::ai::globals().state->actors_valid != 0 ? halo::ai::globals().state->first_encounterless_actor : (datum_index)k_datum_index_none;
    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        datum_index current = actor_index;
        uint8_t *raw = (uint8_t *)&((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        int16_t squad_index;
        int16_t remapped;

        actor_index = ((struct actor *)raw)->next_in_encounter;
        if ((((struct actor *)raw)->original_encounter_index & halo::k_slot_mask) != source_index) {
            continue;
        }
        squad_index = ((struct actor *)raw)->original_squad_index;
        remapped = remap[squad_index];
        if (remapped == -1 || (merging_into_self && remapped == squad_index)) {
            continue;
        }
        ((struct actor *)raw)->original_encounter_index = target_index;
        ((struct actor *)raw)->original_squad_index = remapped;
        if (merging_into_self || static_cast<int16_t>(target_definition->precomputed_bsp_index) != halo::scenario::globals().structure_bsp_index) {
            continue;
        }
        halo::ai::ai_actor_unlink_from_unassigned_list(current);
        halo::ai::encounter_add_actor(((struct actor *)raw)->original_squad_index, current, ((struct actor *)raw)->original_encounter_index, 1);
    }

    if (source_enc->team != target_enc->team) {
        halo::ai::ai_recompute_all_relationship_flags();
    }
    halo::ai::encounters_recompute_dirty();
}

/**
 * Behaviour of ai starting location derive placement flags, moved unchanged from the original free
 * function.
 *
 * @address 0x436d40
 */
void EncounterView::starting_location_derive_placement_flags(int16_t starting_location_index, uint8_t *out_a, int16_t *out_b, uint8_t *out_c, int16_t *out_d, int16_t *out_edx, int16_t *out_esi)
{
    datum_index encounter_index = handle;
    ScenarioEncounter *encounter_definition =
        &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];
    ScenarioSquad *squads = (ScenarioSquad *)encounter_definition->squads.pointer;
    int16_t category = encounter_definition->search_behavior;

    if ((*((uint8_t *)squads + starting_location_index * 0xe8 + 0x28) & 2) != 0) {
        category = 1;
    }

    if (category == 1) {
        *out_b = 1;
        *out_esi = 2;
        *out_edx = 2;
    } else if (category == 2) {
        *out_a = 1;
        *out_d = 0;
        *out_esi = 0;
        *out_edx = 0;
        *out_c = 0;
    }
}

/**
 * Returns the encounter's units_active flag.
 *
 * @address 0x437710
 */
uint8_t EncounterView::activate()
{
    datum_index encounter_index = handle;
    encounter *enc;
    ScenarioEncounter *definition;
    actor *a;
    datum_index actor_index;
    datum_index current;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];

    if ((int16_t)definition->precomputed_bsp_index != -1 &&
        (int16_t)definition->precomputed_bsp_index != halo::scenario::globals().structure_bsp_index) {
        return enc->units_active;
    }

    if (enc->units_active == 0) {
        actor_index = (datum_index)k_datum_index_none;
        if (halo::ai::globals().state->actors_valid != 0) {
            if (encounter_index == (datum_index)k_datum_index_none) {
                actor_index = halo::ai::globals().state->first_encounterless_actor;
            } else {
                actor_index = enc->first_actor;
            }
        }
        while (halo::ai::globals().state->actors_valid != 0 &&
               actor_index != (datum_index)k_datum_index_none) {
            current = actor_index;
            a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
            actor_index = a->next_in_encounter;
            if (a->active != 1) {
                if (a->swarm == 0 || (halo::ai::actor_create_swarm(current),
                                      a->swarm_index != (datum_index)k_datum_index_none)) {
                    a->active = 1;
                    if (a->awareness_level == 0) {
                        halo::ai::actor_set_units_active(current, 0);
                    }
                } else {
                    a->swarm_pending = 1;
                }
            }
        }
    }

    enc->activation_tick = halo::game::globals().game_time->game_time;
    enc->units_active = 1;
    return enc->units_active;
}

/**
 * Behaviour of encounter add actor, moved unchanged from the original free function.
 *
 * @address 0x436770
 */
void Encounters::add_actor(int16_t squad_index, datum_index actor_index, datum_index encounter_index, uint8_t keep_team)
{
    actor *a;
    encounter *enc;
    ScenarioEncounter *definition;
    encounter_squad_state *squad_state;
    encounter_platoon_state *platoon_state;
    int16_t platoon_index;
    uint8_t activated;

    if (halo::ai::globals().state->actors_valid == 0) {
        return;
    }

    a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];
    squad_state = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + squad_index)];

    platoon_index = (int16_t)((ScenarioSquad *)definition->squads.pointer)[squad_index].platoon;

    a->original_encounter_index = (datum_index)k_datum_index_none;
    a->original_squad_index = -1;
    a->next_in_encounter = enc->first_actor;
    enc->first_actor = actor_index;

    if (platoon_index < 0 || definition->platoons.count <= (int32_t)platoon_index) {
        platoon_index = -1;
    }
    a->squad_index = squad_index;
    a->platoon_index = platoon_index;
    a->encounter_index = encounter_index;

    activated = 0;
    if (a->active != 0 && a->keep_unit_alive == 0) {
        enc->activation_delay = 0x96;
        activated = halo::ai::encounter_activate(encounter_index);
    }
    if (activated == 0) {
        halo::ai::actor_toggle_active_state(enc->units_active, actor_index);
        if (enc->units_active != 0) {
            halo::ai::actor_set_units_active(actor_index, 0);
        }
    }

    if (a->unit_index != (datum_index)k_datum_index_none) {
        halo::ai::ai_encounter_stamp_team_from_unit(encounter_index, a->unit_index);
    }

    if (a->team != enc->team) {
        if (keep_team == 0 || enc->living_count != 0) {
            halo::ai::actor_propagate_unit_field(actor_index, enc->team); // 0x4368d1: ESI = the encounter's team
        } else {
            enc->team = a->team;
            halo::ai::ai_recompute_all_relationship_flags();
        }
    }

    enc->member_count = enc->member_count + 1;
    squad_state->member_count = squad_state->member_count + 1;
    if (a->counts_toward_encounter != 0) {
        enc->live_count = enc->live_count + 1;
    }

    if (platoon_index != -1) {
        platoon_state = &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + platoon_index)];
        a->platoon_defending = platoon_state->defending;
        a->defending = platoon_state->defending;
        platoon_state->member_count = platoon_state->member_count + 1;
    }
    enc->dirty = 1;
}

/**
 * Behaviour of encounter advance grenade timers, moved unchanged from the original free function.
 *
 * @address 0x438db0
 */
void EncounterView::advance_grenade_timers()
{
    datum_index encounter_index = handle;
    encounter *self;

    self = halo::ai::encounter_at(encounter_index);

    if (self->engaged == 0) {
        if (self->ticks_since_engaged != (datum_index)halo::k_dword_none) {
            self->ticks_since_engaged = self->ticks_since_engaged + 0xf;
        }
    } else {
        self->ticks_since_engaged = 0;
    }

    if (self->has_live_target == 0) {
        if (self->ticks_since_live_target != (datum_index)halo::k_dword_none) {
            self->ticks_since_live_target = self->ticks_since_live_target + 0xf;
        }
    } else {
        self->ticks_since_live_target = 0;
    }

    if ((self->post_combat != 0) && (self->post_combat_quiet != 0)) {
        if (0xf < self->post_combat_timer) {
            self->post_combat_timer = self->post_combat_timer - 0xf;
            return;
        }
        self->post_combat_timer = 0;
    }
    return;
}

/**
 * Behaviour of encounter build firing position claims, moved unchanged from the original free function.
 *
 * @address 0x4360d0
 */
void EncounterView::build_firing_position_claims(datum_index *out_claims)
{
    datum_index encounter_index = handle;
    ScenarioEncounter *definition;
    encounter *enc;
    actor *a;
    datum_index *cursor;
    datum_index actor_index;
    datum_index current;
    uint32_t remaining;
    int16_t position_index;

    definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];

    cursor = out_claims;
    for (remaining = (uint32_t)definition->firing_positions.count & 0x3fffffff;
         remaining != 0; remaining = remaining - 1) {
        *cursor = (datum_index)k_datum_index_none;
        cursor = cursor + 1;
    }

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
            actor_index = enc->first_actor;
        }
    }

    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        actor_index = a->next_in_encounter;
        position_index = a->firing_position_index;
        if (position_index != -1) {
            out_claims[position_index] = current;
        }
    }
}

/**
 * Behaviour of encounter choose vocalizations, moved unchanged from the original free function.
 *
 * @address 0x438580
 */
void EncounterView::choose_vocalizations()
{
    datum_index encounter_index = handle;
    encounter *enc;
    actor *a;
    actor *chosen;
    prop *p;
    object *obj;
    uint8_t *actor_definition;
    datum_index actor_index;
    datum_index current;
    datum_index prop_index;
    datum_index next_prop;
    datum_index chosen_actor;
    datum_index nearest_actor;
    datum_index morale_actor;
    datum_index nearest_prop;
    ai_scored_candidate buckets[8];
    ai_scored_candidate picked[2];
    int16_t picked_bucket[2];
    int16_t morale_line;
    float proximity;
    float range;
    float scale;
    float boost;
    float freshness;
    float score;
    float bias;
    float best_distance;
    float dx;
    float dy;
    float dz;
    float distance;
    int16_t bucket;
    int16_t i;
    int16_t head_count;
    int32_t margin;
    uint8_t has_unit_prop;
    uint8_t any_candidate;
    uint8_t still_any;
    uint8_t inserted;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];

    for (i = 0; i < 8; i = i + 1) {
        buckets[i].handle = (datum_index)k_datum_index_none;
        buckets[i].score = 0.0f;
        buckets[i].payload = (datum_index)k_datum_index_none;
        buckets[i].key = (datum_index)k_datum_index_none;
    }
    picked_bucket[0] = -1;
    picked_bucket[1] = -1;
    morale_line = -1;
    nearest_actor = (datum_index)k_datum_index_none;
    morale_actor = (datum_index)k_datum_index_none;
    nearest_prop = (datum_index)k_datum_index_none;
    any_candidate = 0;

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        actor_definition = (uint8_t *)halo::cache::globals().tag_instances[a->actor_definition_tag & halo::k_slot_mask].data;
        actor_index = a->next_in_encounter;
        has_unit_prop = 0;

        if (a->unit_index != (datum_index)k_datum_index_none) {
            proximity = halo::ai::ai_communication_rate_player_proximity(1, 0, 0, a->unit_index);

            next_prop = a->first_prop;
            while (next_prop != (datum_index)k_datum_index_none) {
                prop_index = next_prop;
                p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
                next_prop = p->next_in_actor;

                if (p->dead == 0) {
                    continue;
                }

                if (p->enemy == 0) {
                    range = 9.0f;
                    bucket = 2;
                    bias = 0.4f;
                } else if ((actor_definition[4] & 0x40) == 0 && p->dead_ticks < 0xd2) {
                    range = 10.0f;
                    bucket = 0;
                    bias = 0.7f;
                } else {
                    range = 5.0f;
                    bucket = 1;
                    bias = 0.0f;
                }

                if (p->distance < range) {
                    scale = range / p->distance;
                    if (2.0f <= scale) {
                        scale = 2.0f;
                    }
                    boost = proximity;
                    if (proximity <= 1.5f) {
                        boost = 1.5f;
                    }
                    freshness = (float)(int32_t)p->dead_ticks * 0.004166667f;
                    if (1.0f <= freshness) {
                        freshness = 1.0f;
                    }
                    score = (2.0f - freshness) * boost * scale + bias;
                    if (p->is_parented != 0) {
                        score = score + 2.0f;
                    }
                    if (p->enemy != 0) {
                        has_unit_prop = 1;
                    }
                    inserted = halo::ai::ai_insert_scored_candidate_pair(&buckets[bucket * 2], current,
                        score, prop_index, p->object_index);
                    if (inserted != 0) {
                        any_candidate = 1;
                    }
                }
            }

            if (has_unit_prop != 0) {
                obj = halo::ai::object_at(a->unit_index);
                head_count = *(int16_t *)((uint8_t *)obj + 0x42a);
                inserted = halo::ai::ai_insert_scored_candidate_pair(&buckets[3 * 2], current,
                    (float)(int32_t)head_count * 0.7f + proximity,
                    (datum_index)k_datum_index_none, (datum_index)k_datum_index_none);
                if (inserted != 0) {
                    any_candidate = 1;
                }
            }
        }
    }

    if (any_candidate != 0) {
        bucket = halo::ai::ai_pick_weighted_candidate(buckets, &picked[0]);
        picked_bucket[0] = bucket;

        if (enc->team != 2 || 7 < enc->enemy_death_count) {
            still_any = 0;
            for (i = 0; i < 4; i = i + 1) {
                ai_scored_candidate *pair = &buckets[i * 2];
                if (i == bucket) {
                    pair[0].handle = (datum_index)k_datum_index_none;
                    pair[0].score = 0.0f;
                    pair[0].payload = (datum_index)k_datum_index_none;
                    pair[0].key = (datum_index)k_datum_index_none;
                    pair[1] = pair[0];
                } else if (pair[0].handle == picked[0].handle ||
                           pair[0].key == picked[0].key) {
                    pair[0] = pair[1];
                    pair[1].handle = (datum_index)k_datum_index_none;
                    pair[1].score = 0.0f;
                    pair[1].payload = (datum_index)k_datum_index_none;
                    pair[1].key = (datum_index)k_datum_index_none;
                }
                if (pair[0].handle != (datum_index)k_datum_index_none) {
                    still_any = 1;
                }
            }
            if (still_any != 0) {
                picked_bucket[1] = halo::ai::ai_pick_weighted_candidate(buckets, &picked[1]);
            }
        }
    }

    if (enc->team != 2 || 3 < enc->enemy_death_count) {
        chosen_actor = (datum_index)k_datum_index_none;
        best_distance = 0.0f;

        actor_index = (datum_index)k_datum_index_none;
        if (halo::ai::globals().state->actors_valid != 0) {
            if (encounter_index == (datum_index)k_datum_index_none) {
                actor_index = halo::ai::globals().state->first_encounterless_actor;
            } else {
                actor_index = enc->first_actor;
            }
        }
        while (halo::ai::globals().state->actors_valid != 0 &&
               actor_index != (datum_index)k_datum_index_none) {
            current = actor_index;
            a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
            actor_index = a->next_in_encounter;
            if (a->unit_index != (datum_index)k_datum_index_none) {
                proximity = halo::ai::ai_communication_rate_player_proximity(1, 0, 0, a->unit_index);
                if ((actor_type_procs[a->type][4] & 2) != 0 /* 0x438a6c: byte +0x4 of the type definition */ && 2.0f < proximity &&
                    best_distance < proximity) {
                    best_distance = proximity;
                    chosen_actor = current;
                }
            }
        }

        morale_actor = chosen_actor;
        if (chosen_actor != (datum_index)k_datum_index_none) {
            chosen = &((actor *)halo::ai::globals().actor_data->data)[chosen_actor & halo::k_slot_mask];

            if (0.5f <= chosen->body_vitality ||
                chosen->stood_down_body_vitality - chosen->body_vitality <= 0.3f) {
                head_count = enc->living_count;
                if (head_count == 1 && 1 < enc->pre_combat_living_count) {
                    morale_line = 1;
                } else {
                    if (1 < head_count) {
                        margin = 2;
                        if (head_count < 3) {
                            margin = (int32_t)head_count;
                        }
                        if ((int32_t)head_count + margin <= (int32_t)enc->pre_combat_living_count) {
                            morale_line = 4;
                            goto stamp;
                        }
                        if (1 < head_count && (int32_t)enc->pre_combat_living_count - 1 <= (int32_t)head_count) {
                            morale_line = 5;
                            goto stamp;
                        }
                    }
                    if (0.8f < chosen->body_vitality) {
                        morale_line = 2;
                    }
                }
            } else {
                best_distance = 3.4028235e+38f;
                nearest_actor = (datum_index)k_datum_index_none;
                morale_line = 3;
                nearest_prop = (datum_index)k_datum_index_none;

                actor_index = (datum_index)k_datum_index_none;
                if (halo::ai::globals().state->actors_valid != 0) {
                    if (encounter_index == (datum_index)k_datum_index_none) {
                        actor_index = halo::ai::globals().state->first_encounterless_actor;
                    } else {
                        actor_index = enc->first_actor;
                    }
                }
                while (halo::ai::globals().state->actors_valid != 0 &&
                       actor_index != (datum_index)k_datum_index_none) {
                    datum_index found;
                    current = actor_index;
                    a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
                    actor_index = a->next_in_encounter;
                    if (a->unit_index == (datum_index)k_datum_index_none ||
                        current == chosen_actor) {
                        continue;
                    }
                    dx = a->aim_origin.x - chosen->aim_origin.x;
                    dy = a->aim_origin.y - chosen->aim_origin.y;
                    dz = a->aim_origin.z - chosen->aim_origin.z;
                    distance = dx * dx + dy * dy + dz * dz;
                    if (best_distance != 3.4028235e+38f &&
                        distance >= best_distance * best_distance) {
                        continue;
                    }
                    found = halo::ai::actor_find_prop_for_object(chosen->unit_index, current);
                    if (found == (datum_index)k_datum_index_none) {
                        continue;
                    }
                    best_distance = distance;
                    nearest_actor = current;
                    nearest_prop = found;
                }
                if (nearest_actor == (datum_index)k_datum_index_none) {
                    nearest_prop = (datum_index)k_datum_index_none;
                }
            }
        }
    }

stamp:
    for (i = 0; i < 2; i = i + 1) {
        if (picked_bucket[i] != -1 && picked[i].handle != (datum_index)k_datum_index_none) {
            a = &((actor *)halo::ai::globals().actor_data->data)[picked[i].handle & halo::k_slot_mask];
            a->post_combat_action = ai_vocalization_line_table[picked_bucket[i]];
            a->post_combat_prop_index = picked[i].payload;
        }
    }

    if (morale_line != -1 && morale_actor != (datum_index)k_datum_index_none) {
        a = &((actor *)halo::ai::globals().actor_data->data)[morale_actor & halo::k_slot_mask];
        a->post_combat_action = morale_line;
        a->post_combat_prop_index = (datum_index)k_datum_index_none;
        if (nearest_actor != (datum_index)k_datum_index_none) {
            a = &((actor *)halo::ai::globals().actor_data->data)[nearest_actor & halo::k_slot_mask];
            a->post_combat_action = 6;
            a->post_combat_prop_index = nearest_prop;
        }
    }

    enc->post_combat = 1;
    enc->post_combat_quiet = 0;
    enc->post_combat_timer = 0x78;
    enc->enemy_death_count = 0;
}

/**
 * Behaviour of encounter deactivate, moved unchanged from the original free function.
 *
 * @address 0x437870
 */
void EncounterView::deactivate()
{
    datum_index encounter_index = handle;
    encounter *enc;
    actor *a;
    datum_index actor_index;
    datum_index current;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    enc->units_active = 0;
    halo::ai::squad_recent_object_list_clear(encounter_index);

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        actor_index = a->next_in_encounter;
        if (a->active != 0) {
            halo::ai::actor_clear_perceived_props(current);
            halo::ai::actor_delete_swarm(current);
            halo::ai::actor_set_units_active(current, 1);
            a->active = 0;
            a->deactivation_time = (datum_index)halo::game::globals().game_time->game_time;
        }
    }
}

/**
 * Behaviour of encounter decay squad spawn delays, moved unchanged from the original free function.
 *
 * @address 0x4392f0
 */
void EncounterView::decay_squad_spawn_delays()
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    encounter_squad_state *squad_state;
    int16_t cooldown;
    uint32_t squad_flags;
    int16_t squad_index;

    self = halo::ai::encounter_at(encounter_index);
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];

    squad_index = 0;
    if (0 < self->squad_count) {
        do {
            squad_state = &halo::ai::globals().squad_states[(int16_t)(self->first_squad + squad_index)];
            cooldown = squad_state->squad_delay_ticks;

            squad_flags = ((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index].flags;
            if ((0 < cooldown) && ((squad_flags & 8) == 0)) {
                if (squad_state->timer_started == 0) {
                    if (((squad_flags & 4) == 0) && (self->combat_count < 1)) {
                        squad_state->timer_started = 0;
                    } else {
                        squad_state->timer_started = 1;
                    }
                } else if (cooldown < 0x10) {
                    halo::ai::encounter_squad_clear_spawn_delay(encounter_index, squad_index);
                } else {
                    squad_state->squad_delay_ticks = cooldown - 0xf;
                }
            }

            squad_index = squad_index + 1;
        } while (squad_index < self->squad_count);
    }
    return;
}

/**
 * Linear-searches one ScenarioEncounter's platoons for one whose name matches (case- insensitively, up to
 * 32 characters), returning its index or -1 if there are none or none match.
 *
 * @address 0x4322c0
 */
int32_t Encounters::definition_find_platoon_index_by_name(ScenarioEncounter *encounter_definition, char *name)
{
    int32_t index;
    uint8_t *cursor;

    if (encounter_definition->platoons.count <= 0) {
        return -1;
    }

    index = 0;
    cursor = (uint8_t *)encounter_definition->platoons.pointer;
    while (_strnicmp((char *)cursor, name, 0x20) != 0) {
        index = index + 1;
        cursor += sizeof(ScenarioPlatoon);
        if (encounter_definition->platoons.count <= index) {
            return -1;
        }
    }
    return index;
}

/**
 * Linear-searches one ScenarioEncounter's squads for one whose name matches (case- insensitively, up to 32
 * characters), returning its index or -1 if there are none or none match.
 *
 * @address 0x432260
 */
int32_t Encounters::definition_find_squad_index_by_name(ScenarioEncounter *encounter_definition, char *name)
{
    int32_t index;
    uint8_t *cursor;

    if (encounter_definition->squads.count <= 0) {
        return -1;
    }

    index = 0;
    cursor = (uint8_t *)encounter_definition->squads.pointer;
    while (_strnicmp((char *)cursor, name, 0x20) != 0) {
        index = index + 1;
        cursor += sizeof(ScenarioSquad);
        if (encounter_definition->squads.count <= index) {
            return -1;
        }
    }
    return index;
}

namespace {

class VitalityBelow75 final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override { return vitality < 0.75f; }
};

class VitalityBelow50 final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override { return vitality < 0.5f; }
};

class VitalityBelow25 final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override { return vitality < 0.25f; }
};

class AnyCasualty final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override { return living < members; }
};

class AtMostThreeQuartersAlive final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override
    {
        return (((int32_t)living << 2) / 3) <= members;
    }
};

class AtMostHalfAlive final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override
    {
        return (living * 2 - (int32_t)members == 0) || (living * 2 < (int32_t)members);
    }
};

class AtMostQuarterAlive final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override
    {
        return living * 4 <= (int32_t)members;
    }
};

class FewerThanTwoAlive final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override { return living < 2; }
};

class NoneAlive final : public PlatoonCondition {
public:
    bool test(float vitality, int16_t living, int16_t members) const override { return living == 0; }
};

constexpr VitalityBelow75 k_vitality_below_75;
constexpr VitalityBelow50 k_vitality_below_50;
constexpr VitalityBelow25 k_vitality_below_25;
constexpr AnyCasualty k_any_casualty;
constexpr AtMostThreeQuartersAlive k_at_most_three_quarters_alive;
constexpr AtMostHalfAlive k_at_most_half_alive;
constexpr AtMostQuarterAlive k_at_most_quarter_alive;
constexpr FewerThanTwoAlive k_fewer_than_two_alive;
constexpr NoneAlive k_none_alive;

constexpr const PlatoonCondition *k_platoon_conditions[] = {
    &k_vitality_below_75,
    &k_vitality_below_50,
    &k_vitality_below_25,
    &k_any_casualty,
    &k_at_most_three_quarters_alive,
    &k_at_most_half_alive,
    &k_at_most_quarter_alive,
    &k_fewer_than_two_alive,
    &k_none_alive,
};

}

/**
 * Returns the platoon condition rule for a scenario condition code (1 to 9), or null for any other code.
 */
const PlatoonCondition *platoon_condition(int32_t code)
{
    if (code < 1 || code > 9) {
        return nullptr;
    }
    return k_platoon_conditions[code - 1];
}

/**
 * Behaviour of encounter evaluate platoon condition, moved unchanged from the original free function.
 *
 * @address 0x439f20
 */
uint8_t EncounterView::evaluate_platoon_condition(const ai_platoon_condition *condition)
{
    datum_index encounter_index = handle;
    encounter *self;
    encounter_platoon_state *platoon_state;
    int16_t platoon_index;
    int16_t count_a;
    int16_t count_b;
    float threshold;

    self = halo::ai::encounter_at(encounter_index);

    platoon_index = condition->platoon_index;
    if ((platoon_index < 0) || (self->platoon_count <= platoon_index)) {
        count_b = self->member_count;
        count_a = self->living_count;
        threshold = self->average_vitality;
    } else {
        platoon_state = &halo::ai::globals().platoon_states[(int16_t)(self->first_platoon + platoon_index)];
        count_b = platoon_state->member_count;
        count_a = platoon_state->living_count;
        threshold = platoon_state->average_vitality;
    }

    if (count_b <= 0) {
        return 0;
    }

    const PlatoonCondition *rule = platoon_condition(condition->code);
    if (rule == nullptr) {
        return 0;
    }
    return rule->test(threshold, count_a, count_b);
}

/**
 * Behaviour of encounter evaluate support needs, moved unchanged from the original free function.
 *
 * @address 0x436dc0
 */
void EncounterView::evaluate_support_needs(datum_index self_actor_index, int16_t mode, uint8_t phase, uint8_t *out_crowded, uint8_t *out_flanked, uint8_t *out_a, uint8_t *out_b, uint8_t *out_reachable_a, uint8_t *out_reachable_b, uint8_t *out_any)
{
    datum_index encounter_index = handle;
    encounter *enc;
    actor *a;
    actor *self;
    datum_index actor_index;
    datum_index current;
    int16_t engaged_count;
    int16_t mode7_count;
    int16_t alert_count;
    int16_t mode5_count;
    int32_t threshold;
    int32_t reachable;
    uint8_t any;
    uint8_t not_self;

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
            actor_index = enc->first_actor;
        }
    }

    engaged_count = 0;
    mode7_count = 0;
    alert_count = 0;
    mode5_count = 0;

    for (;;) {
        if (halo::ai::globals().state->actors_valid == 0 || actor_index == (datum_index)k_datum_index_none) {
            break;
        }
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        not_self = (uint8_t)(current != self_actor_index);
        actor_index = a->next_in_encounter;

        if (not_self != 0 && a->grenade_ally_phase_flag == phase) {
            if (a->mode == 5) {
                if (*(int16_t *)(a->mode_data.raw + 8) == 0) {
                    if (a->combat_status < 3) {
                        mode5_count = mode5_count + 1;
                    }
                } else {
                    engaged_count = engaged_count + 1;
                }
            } else if (a->mode == 7) {
                if (*(int16_t *)(a->mode_data.raw + 8) != 0) {
                    engaged_count = engaged_count + 1;
                } else {
                    mode7_count = mode7_count + 1;
                }
            }
        }
        if (a->awareness_level == 3) {
            alert_count = alert_count + 1;
        }
    }

    if (mode == 1) {
        threshold = 0;
    } else if (mode == 2) {
        threshold = 999;
    } else {
        threshold = (int32_t)alert_count / 3;
        if (threshold < 3) {
            threshold = 3;
        }
    }

    self = &((actor *)halo::ai::globals().actor_data->data)[self_actor_index & halo::k_slot_mask];
    if (phase == 0) {
        halo::ai::actor_find_nearest_grenade_ally(self_actor_index, 0);
        *out_reachable_b = (uint8_t)(self->nearby_friend_prop_index != (datum_index)k_datum_index_none);
        self->grenade_ally_phase_flag = 0;
    } else {
        reachable = halo::ai::actor_find_nearest_grenade_ally(self_actor_index, 1);
        *out_reachable_a = (uint8_t)(1 < reachable);
        self->grenade_ally_phase_flag = (uint8_t)(1 < reachable);
    }

    any = (uint8_t)(out_reachable_a != 0 || out_reachable_b != 0);
    *out_any = any;

    *out_crowded = (uint8_t)(mode5_count < 6);
    *out_flanked = (uint8_t)(mode7_count < 4);
    *out_b = (uint8_t)(engaged_count < (int16_t)threshold);
    *out_a = (uint8_t)(engaged_count < (int16_t)threshold);
}

/**
 * Behaviour of encounter gather occupied clusters, moved unchanged from the original free function.
 *
 * @address 0x436190
 */
void EncounterView::gather_occupied_clusters(uint32_t *out_clusters, uint8_t record_per_actor, uint32_t *other_clusters)
{
    datum_index encounter_index = handle;
    ScenarioEncounter *definition;
    encounter *enc;
    actor *a;
    object *obj;
    uint32_t *fill;
    uint32_t dword_count;
    uint32_t squad_mask;
    uint32_t zone_mask;
    uint32_t extra;
    datum_index actor_index;
    datum_index current;
    datum_index object_index;
    datum_index root_index;
    datum_index child_index;
    int16_t cluster;
    uint32_t bit;
    uint8_t visible;
    int16_t i;
    int32_t j;
    ScenarioSquad *squad;
    ScenarioFiringPosition *positions;
    int16_t position_cluster;

    definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];
    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];

    zone_mask = 0;
    squad_mask = 0;

    fill = out_clusters;
    for (dword_count = (uint32_t)(((*(int32_t *)((uint8_t *)halo::scenario::globals().structure_bsp + 0x134)) + 0x1f) >> 5);
         dword_count != 0; dword_count = dword_count - 1) {
        *fill = 0;
        fill = fill + 1;
    }

    actor_index = enc->first_actor;
    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }

    do {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        visible = 1;

        if (a->swarm == 0) {
            root_index = (datum_index)k_datum_index_none;
            if (a->unit_index != (datum_index)k_datum_index_none) {
                object_index = a->unit_index;
                do {
                    root_index = object_index;
                    obj = halo::ai::object_at(root_index);
                    object_index = obj->parent_object;
                } while (object_index != (datum_index)k_datum_index_none);
            }
            obj = halo::ai::object_at(root_index);
            cluster = obj->location_cluster_index;
            if (cluster != -1) {
                bit = 1 << (cluster & 0x1f);
                out_clusters[(int32_t)cluster >> 5] =
                    out_clusters[(int32_t)cluster >> 5] | bit;
                if (other_clusters != 0 &&
                    (other_clusters[(int32_t)cluster >> 5] & bit) != 0) {
                    visible = 0;
                }
            }

            if (enc->units_active != 0) {
                if (a->awareness_level == 3) {
                    if (1 < a->combat_status) {
                        extra = 0;
                        if (a->encounter_index != (datum_index)k_datum_index_none) {
                            squad = &((ScenarioSquad *)
                                ((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                                    [a->encounter_index & halo::k_slot_mask].squads.pointer)[a->squad_index];
                            extra = squad->pursuing;
                        }
                        zone_mask = zone_mask | extra;
                    }
                    if (a->mode == 6 || a->mode == 4) {
                        extra = 0;
                        if (a->encounter_index != (datum_index)k_datum_index_none) {
                            squad = &((ScenarioSquad *)
                                ((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                                    [a->encounter_index & halo::k_slot_mask].squads.pointer)[a->squad_index];
                            extra = *(uint32_t *)((uint8_t *)squad + 0x54 +
                                (int16_t)((-(uint16_t)(a->defending != 0) & 3) + 2) * 4);
                        }
                        zone_mask = zone_mask | extra;
                    } else if (a->mode == 3 || a->mode == 5) {
                        zone_mask = zone_mask | halo::ai::actor_get_firing_position_group_mask(current, 0, 0);
                    }
                } else if (a->awareness_level == 2 && *(int16_t *)(a->mode_data.raw + 0) != 0) {
                    squad_mask = squad_mask | (1 << (a->squad_index & 0x1f));
                }

                if (a->target_unit_index != (datum_index)k_datum_index_none) {
                    cluster = halo::ai::prop_at(a->target_unit_index)->location.cluster_index;
                    if (cluster != -1) {
                        out_clusters[(int32_t)cluster >> 5] =
                            out_clusters[(int32_t)cluster >> 5] | (1 << (cluster & 0x1f));
                    }
                }
            }
        } else {
            child_index = a->cluster_unit_index;
            while (child_index != (datum_index)k_datum_index_none) {
                object *child = halo::ai::object_at(child_index);
                root_index = (datum_index)k_datum_index_none;
                object_index = child_index;
                for (; object_index != (datum_index)k_datum_index_none;
                     object_index = *(datum_index *)((uint8_t *)((object_header *)
                         halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data + 0x11c)) {
                    root_index = object_index;
                }
                obj = halo::ai::object_at(root_index);
                cluster = obj->location_cluster_index;
                if (cluster != -1) {
                    bit = 1 << (cluster & 0x1f);
                    out_clusters[(int32_t)cluster >> 5] =
                        out_clusters[(int32_t)cluster >> 5] | bit;
                    if (other_clusters != 0 &&
                        (other_clusters[(int32_t)cluster >> 5] & bit) != 0) {
                        visible = 0;
                    }
                }
                child_index = *(datum_index *)((uint8_t *)child + 0x1fc);
            }
        }

        if (record_per_actor != 0) {
            if (halo::ai::globals().squad_states[(int16_t)(a->squad_index + enc->first_squad)].dormancy_disabled
                != 0) {
                visible = 0;
            }
            a->can_go_dormant = visible;
        }
        actor_index = a->next_in_encounter;
    } while (actor_index != (datum_index)k_datum_index_none);

    if (zone_mask != 0 && 0 < definition->firing_positions.count) {
        positions = (ScenarioFiringPosition *)definition->firing_positions.pointer;
        i = 0;
        j = 0;
        do {
            if ((int16_t)positions[j].cluster_index != -1 &&
                (zone_mask & (1 << (positions[j].group_index & 0x1f))) != 0) {
                position_cluster = (int16_t)positions[j].cluster_index;
                out_clusters[(int32_t)position_cluster >> 5] =
                    out_clusters[(int32_t)position_cluster >> 5] |
                    (1 << (position_cluster & 0x1f));
            }
            i = i + 1;
            j = (int32_t)i;
        } while (j < definition->firing_positions.count);
    }

    if (squad_mask != 0 && 0 < definition->squads.count) {
        i = 0;
        j = 0;
        do {
            if ((squad_mask & (1 << (j & 0x1f))) != 0) {
                squad = &((ScenarioSquad *)definition->squads.pointer)[j];
                if (0 < squad->move_positions.count) {
                    int16_t k = 0;
                    int32_t m = 0;
                    do {
                        position_cluster = *(int16_t *)((uint8_t *)squad->move_positions.pointer +
                            m * 0x50 + 0x28);
                        if (position_cluster != -1) {
                            out_clusters[(int32_t)position_cluster >> 5] =
                                out_clusters[(int32_t)position_cluster >> 5] |
                                (1 << (position_cluster & 0x1f));
                        }
                        k = k + 1;
                        m = (int32_t)k;
                    } while (m < squad->move_positions.count);
                }
            }
            i = i + 1;
            j = (int32_t)i;
        } while (j < definition->squads.count);
    }
}

/**
 * Behaviour of encounter new, moved unchanged from the original free function.
 *
 * @address 0x437060
 */
void Encounters::create(int16_t *squad_cursor, ScenarioEncounter *definition, int16_t *platoon_cursor)
{
    datum_index encounter_index;
    encounter *enc;
    encounter_squad_state *squad_state;
    ScenarioSquad *squad_definition;
    int16_t squad_index;
    int16_t platoon_index;
    int16_t respawn_budget;
    int16_t count;

    encounter_index = halo::memory::datum_new(halo::ai::globals().encounter_data);
    if (encounter_index == (datum_index)k_datum_index_none) {
        return;
    }
    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];

    enc->team = definition->team_index;
    enc->first_actor = (datum_index)k_datum_index_none;
    enc->first_pursuit = (datum_index)k_datum_index_none;
    enc->blind = (uint8_t)((definition->flags >> 2) & 1);
    enc->deaf = (uint8_t)((definition->flags >> 3) & 1);
    enc->respawn_enabled = (uint8_t)((definition->flags >> 1) & 1);
    enc->respawn_delay_ticks = 0;
    enc->unknown_46 = 0;
    enc->engaged = 0;
    enc->ticks_since_engaged = (datum_index)k_datum_index_none;
    enc->has_live_target = 0;
    enc->ticks_since_live_target = (datum_index)k_datum_index_none;
    enc->last_idle_time = -1;
    enc->stood_down = 1;
    enc->last_grenade_time = (datum_index)k_datum_index_none;
    enc->activation_link_count = 0;
    enc->activation_tick = -1;

    count = (int16_t)definition->squads.count;
    enc->squad_count = count;
    enc->first_squad = *squad_cursor;
    *squad_cursor = *squad_cursor + count;

    squad_index = 0;
    if (0 < enc->squad_count) {
        do {
            squad_state = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + squad_index)];
            squad_definition = &((ScenarioSquad *)definition->squads.pointer)[squad_index];

            squad_state->timer_started = 0;
            if (!halo::ai::flag_set(squad_definition->flags, halo::tags::scenario_squad_tag_flag::no_timer_delay_forever)) {
                squad_state->squad_delay_ticks = (int16_t)__ftol(
                    (double)(squad_definition->squad_delay_time * ticks_per_second));
            } else {
                squad_state->squad_delay_ticks = 999;
            }
            squad_state->automatic_migration = (uint8_t)((squad_definition->flags >> 5) & 1);

            halo::ai::encounter_squad_reset_starting_location_mask(encounter_index, squad_index);

            if (0 < squad_definition->respawn_max_actors ||
                0 < squad_definition->respawn_min_actors) {
                respawn_budget = 999;
                if (squad_definition->respawn_total != 0) {
                    respawn_budget = squad_definition->respawn_total;
                }
                squad_state->respawn_budget = respawn_budget;
            }
            squad_index = squad_index + 1;
        } while (squad_index < enc->squad_count);
    }

    count = (int16_t)definition->platoons.count;
    enc->platoon_count = count;
    enc->first_platoon = *platoon_cursor;
    *platoon_cursor = *platoon_cursor + count;

    platoon_index = 0;
    if (0 < enc->platoon_count) {
        do {
            halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + platoon_index)].defending =
                (uint8_t)((((ScenarioPlatoon *)definition->platoons.pointer)[platoon_index].flags
                    >> 2) & 1);
            platoon_index = platoon_index + 1;
        } while (platoon_index < enc->platoon_count);
    }
}

/**
 * Behaviour of encounter process squad reinforcements, moved unchanged from the original free function.
 *
 * @address 0x4390a0
 */
void EncounterView::process_squad_reinforcements()
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    encounter_squad_state *squad_state;
    uint32_t ready_mask[2];
    int16_t ready_count;
    int16_t squad_index;
    uint32_t roll;

    self = halo::ai::encounter_at(encounter_index);

    if (self->respawn_enabled == 0) {
        return;
    }
    if (0xf < self->respawn_delay_ticks) {
        self->respawn_delay_ticks = self->respawn_delay_ticks - 0xf;
        return;
    }

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];
    ready_mask[0] = 0;
    self->respawn_delay_ticks = 0;
    ready_mask[1] = 0;
    ready_count = 0;
    squad_index = 0;

    if (0 < (int32_t)encounter_definition->squads.count) {
        do {
            squad_state = &halo::ai::globals().squad_states[(int16_t)(self->first_squad + squad_index)];
            squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];

            if (0 < squad_state->respawn_budget) {
                do {
                    if ((squad_definition->respawn_min_actors <= squad_state->living_count) ||
                        ((int8_t)halo::ai::encounter_squad_spawn_reinforcement(encounter_index, squad_index) == 0)) {
                        break;
                    }
                } while (0 < squad_state->respawn_budget);

                if ((0 < squad_state->respawn_budget) &&
                    (squad_state->living_count < squad_definition->respawn_max_actors)) {
                    if (squad_state->respawn_delay_ticks < 0x10) {
                        squad_state->respawn_delay_ticks = 0;
                        ready_count = ready_count + 1;
                        ready_mask[squad_index >> 5] = ready_mask[squad_index >> 5] | (1u << (squad_index & 0x1f));
                    } else {
                        squad_state->respawn_delay_ticks = squad_state->respawn_delay_ticks - 0xf;
                    }
                }
            }

            squad_index = squad_index + 1;
        } while (squad_index < (int32_t)encounter_definition->squads.count);

        if ((0 < ready_count) && (self->respawn_delay_ticks == 0)) {
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            squad_index = 0;
            roll = (uint32_t)(((uint64_t)(halo::math::globals().random_seed_global >> 0x10) * (uint32_t)ready_count) >> 0x10);

            if (0 < self->squad_count) {
                do {
                    if ((ready_mask[squad_index >> 5] & (1u << (squad_index & 0x1f))) != 0) {
                        if ((int16_t)roll < 1) {
                            if ((int8_t)halo::ai::encounter_squad_spawn_reinforcement(encounter_index, squad_index) != 0) {
                                return;
                            }
                        } else {
                            roll = roll - 1;
                        }
                    }
                    squad_index = squad_index + 1;
                } while (squad_index < self->squad_count);
            }
        }
    }
    return;
}

/**
 * Behaviour of encounter propagate platoon state to actors, moved unchanged from the original free
 * function.
 *
 * @address 0x439d80
 */
void EncounterView::propagate_platoon_state_to_actors()
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    ScenarioPlatoon *platoon_definition;
    encounter_platoon_state *platoon_state;
    actor *member;
    datum_index actor_index;
    int16_t platoon_index;
    int16_t maneuver_to_squad;
    uint8_t attacking_flag;
    uint8_t ready;

    self = halo::ai::encounter_at(encounter_index);
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];

    actor_index = (datum_index)halo::k_dword_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)halo::k_dword_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            actor_index = self->first_actor;
        }
    }

    while ((halo::ai::globals().state->actors_valid != 0) && (actor_index != (datum_index)halo::k_dword_none)) {
        datum_index current = actor_index;

        member = halo::ai::actor_at(actor_index);
        actor_index = member->next_in_encounter;

        member->stood_down = self->stood_down;
        member->playfight = self->playfight;
        attacking_flag = 0;
        ready = 0;

        if (self->post_combat == 0) {
            member->post_combat_action = 0;
            member->post_combat_prop_index = (datum_index)halo::k_dword_none;
        }

        platoon_index = member->platoon_index;
        if (platoon_index != -1) {
            platoon_state = &halo::ai::globals().platoon_states[(int16_t)(self->first_platoon + platoon_index)];
            attacking_flag = platoon_state->defending;
            if ((((uint8_t *)platoon_state)[1] == 0) || (((uint8_t *)platoon_state)[2] != 0)) {
                ready = 0;
            } else {
                ready = 1;
            }
        }
        member->platoon_defending = attacking_flag;

        if (ready != 0) {
            squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[member->squad_index];
            maneuver_to_squad = squad_definition->maneuver_to_squad;
            if ((-1 < maneuver_to_squad) && (maneuver_to_squad < (int32_t)encounter_definition->squads.count)) {
                halo::ai::actor_reset_squad_link_for_type_change(current, encounter_index, maneuver_to_squad);
                platoon_definition = &((ScenarioPlatoon *)encounter_definition->platoons.pointer)[platoon_index];
                halo::ai::actor_notify_squad_and_flag_danger(current, (uint8_t)((platoon_definition->flags >> 1) & 1),
                    (uint8_t)(platoon_definition->flags & 1));
            }
        }
    }

    halo::ai::encounters_recompute_dirty();
    return;
}

/**
 * Behaviour of encounter recompute morale, moved unchanged from the original free function.
 *
 * @address 0x437940
 */
void EncounterView::recompute_morale()
{
    datum_index encounter_index = handle;
    encounter *enc;
    encounter_squad_state *squad_state;
    encounter_platoon_state *platoon_state;
    actor *a;
    object *obj;
    prop *p;
    datum_index actor_index;
    datum_index current;
    float sample;
    int16_t weight;
    int16_t i;
    int16_t pair;
    uint8_t counts;
    uint8_t engaged;
    uint8_t any_unfriendly_target;
    uint8_t any_vocalizing;
    uint8_t any_flag_8c;
    uint8_t any_flag_8d;
    int32_t retreat_timer;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];

    any_unfriendly_target = 0;
    any_vocalizing = 0;
    any_flag_8c = 0;
    any_flag_8d = 0;

    enc->has_live_target = 0;
    enc->engaged = 0;
    enc->engaged_count = 0;
    enc->combat_count = 0;
    enc->swarm_count = 0;
    enc->living_count = 0;
    enc->average_vitality = 0.0f;

    i = 0;
    if (0 < enc->squad_count) {
        do {
            squad_state = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + i)];
            i = i + 1;
            squad_state->swarm_count = 0;
            squad_state->living_count = 0;
            squad_state->average_vitality = 0.0f;
        } while (i < enc->squad_count);
    }
    i = 0;
    if (0 < enc->platoon_count) {
        do {
            platoon_state = &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + i)];
            i = i + 1;
            platoon_state->swarm_count = 0;
            platoon_state->living_count = 0;
            platoon_state->average_vitality = 0.0f;
        } while (i < enc->platoon_count);
    }

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        actor_index = a->next_in_encounter;
        squad_state = &halo::ai::globals().squad_states[(int16_t)(a->squad_index + enc->first_squad)];

        if (a->unit_index == (datum_index)k_datum_index_none) {
            weight = a->cluster_count;
            sample = (float)(int32_t)weight / (float)(int32_t)a->total_cluster_count;
        } else {
            obj = halo::ai::object_at(a->unit_index);
            sample = obj->body_vitality;
            weight = 1;
        }

        if (a->platoon_index != -1) {
            platoon_state =
                &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + a->platoon_index)];
            platoon_state->living_count = platoon_state->living_count + weight;
            platoon_state->average_vitality = sample + platoon_state->average_vitality;
            platoon_state->swarm_count =
                platoon_state->swarm_count + (int16_t)((uint16_t)a->swarm * weight);
        }

        squad_state->living_count = squad_state->living_count + weight;
        squad_state->average_vitality = sample + squad_state->average_vitality;
        squad_state->swarm_count = squad_state->swarm_count + (int16_t)((uint16_t)a->swarm * weight);

        enc->living_count = enc->living_count + weight;
        enc->swarm_count = enc->swarm_count + (int16_t)((uint16_t)a->swarm * weight);

        counts = (uint8_t)(a->awareness_level == 3 && a->minimum_combat_status < a->combat_status);
        enc->combat_count = enc->combat_count + (int16_t)((uint16_t)counts * weight);

        engaged = (uint8_t)(6 < a->combat_status);
        if (engaged != 0 && a->mode == 4 && 0 < *(int16_t *)(a->mode_data.raw + 0x0c)) {
            engaged = 0;
        }
        enc->average_vitality = sample + enc->average_vitality;
        enc->engaged_count = enc->engaged_count + (int16_t)((uint16_t)engaged * weight);

        if (a->target_unit_index != (datum_index)k_datum_index_none) {
            p = &((prop *)halo::ai::globals().prop_data->data)[a->target_unit_index & halo::k_slot_mask];
            enc->ever_had_target = 1;
            if (a->team < 0 || 9 < a->team || p->team < 0 || 9 < p->team ||
                (pair = (int16_t)((int32_t)p->team + a->team * 10),
                 (team_pair_data->secondary_bits[pair >> 5] & (1 << (pair & 0x1f))) == 0)) {
                any_unfriendly_target = 1;
            }
            if (a->has_engaged != 0) {
                any_flag_8c = 1;
            }
            if (a->witnessed_death != 0) {
                any_flag_8d = 1;
            }
            if (a->combat_status < 7) {
                if (p->state < 2 || 3 < p->state) {
                    obj = halo::ai::object_at(p->object_index);
                    counts = *((uint8_t *)obj + 0x106) & 4;
                } else {
                    counts = p->dead;
                }
                if (counts != 0) {
                    goto tally_vocalization;
                }
            } else {
                enc->engaged = 1;
            }
            enc->has_live_target = 1;
        }
tally_vocalization:
        if (0 < a->post_combat_action) {
            any_vocalizing = 1;
        }
    }

    if (any_unfriendly_target != 0) {
        enc->unknown_46 = 0;
    }

    retreat_timer = enc->ticks_since_engaged;
    if (enc->engaged == 0 &&
        (retreat_timer == -1 || 0x3b < retreat_timer) &&
        ((enc->has_live_target == 0 && ((int32_t)enc->ticks_since_live_target == -1 || 0x3b < (int32_t)enc->ticks_since_live_target)) ||
         retreat_timer == -1 || 0x1c1 < retreat_timer)) {
        if (enc->stood_down == 0) {
            if (enc->post_combat == 0) {
                if (any_flag_8c != 0 && any_flag_8d != 0) {
                    halo::ai::encounter_choose_vocalizations(encounter_index);
                    goto normalize;
                }
            } else {
                enc->post_combat_quiet = (uint8_t)(any_vocalizing == 0);
                if (enc->post_combat_timer != 0) {
                    goto normalize;
                }
            }
            halo::ai::encounter_release_stale_props(encounter_index);
        } else {
            enc->post_combat = 0;
            enc->pre_combat_living_count = enc->living_count;
            enc->last_idle_time = halo::game::globals().game_time->game_time;
            enc->enemy_death_count = 0;
            if (enc->ever_had_target == 0) {
                enc->ticks_since_engaged = (datum_index)k_datum_index_none;
                enc->ticks_since_live_target = (datum_index)k_datum_index_none;
            }
        }
    } else {
        enc->stood_down = 0;
        enc->post_combat = 0;
    }

normalize:
    if (0 < enc->member_count) {
        sample = enc->average_vitality / (float)(int32_t)enc->member_count - 0.001f;
        if (sample < 0.0f) {
            sample = 0.0f;
        }
        enc->average_vitality = sample;
    }
    i = 0;
    if (0 < enc->squad_count) {
        do {
            squad_state = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + i)];
            sample = squad_state->average_vitality /
                     (float)(int32_t)squad_state->member_count - 0.001f;
            if (sample < 0.0f) {
                sample = 0.0f;
            }
            i = i + 1;
            squad_state->average_vitality = sample;
        } while (i < enc->squad_count);
    }
    i = 0;
    if (0 < enc->platoon_count) {
        do {
            platoon_state = &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + i)];
            sample = platoon_state->average_vitality /
                     (float)(int32_t)platoon_state->member_count - 0.001f;
            if (sample < 0.0f) {
                sample = 0.0f;
            }
            i = i + 1;
            platoon_state->average_vitality = sample;
        } while (i < enc->platoon_count);
    }
    enc->dirty = 0;
}

/**
 * Behaviour of encounter redistribute squads toward targets, moved unchanged from the original free
 * function.
 *
 * @address 0x4394a0
 */
void EncounterView::redistribute_squads_toward_targets()
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    int16_t target_mode;
    datum_index targets[8];
    uint16_t target_count;
    real_point3d target_positions[8];
    float best_distance_to_target[8];
    real_point3d chosen_position;
    datum_index chosen_object;
    uint32_t squad_considered_mask[2];
    uint32_t squad_active_mask[2];
    uint32_t squad_occupied_mask[2];
    uint32_t combined_trigger_mask;
    uint32_t squad_trigger_mask[64];
    int16_t total_occupancy;
    int16_t squad_count;
    int16_t squad_index;
    int16_t i;

    self = halo::ai::encounter_at(encounter_index);
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];
    target_mode = self->follow_target_type;
    target_count = 0;

    if (target_mode == 1) {
        data_iterator player_iter;
        void *player_record;

        player_iter.data = halo::game::globals().player_data;
        player_iter.next_index = 0;
        player_iter.index = (datum_index)k_datum_index_none;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        player_record = halo::memory::data_iterator_next(&player_iter);
        if (player_record == 0) {
            return;
        }
        do {
            if ((*(int32_t *)((uint8_t *)player_record + 0x34) != -1) && ((int16_t)target_count < 8)) {
                targets[(int16_t)target_count] = *(datum_index *)((uint8_t *)player_record + 0x34);
                target_count = target_count + 1;
            }
            player_record = halo::memory::data_iterator_next(&player_iter);
        } while (player_record != 0);
    } else if (target_mode == 2) {
        datum_index cached_target = self->follow_target;
        if (halo::objects::object_try_and_get(cached_target, 3) == 0) {
            self->follow_target = (datum_index)halo::k_dword_none;
            return;
        }
        targets[0] = cached_target;
        target_count = 1;
        goto have_targets;
    } else if (target_mode == 3) {
        void *member;
        ai_reference_actor_iterator member_iterator;
        if (self->follow_target == (datum_index)halo::k_dword_none) {
            return;
        }
        halo::ai::ai_reference_actor_iterator_new((uint32_t)self->follow_target, &member_iterator);
        member = halo::ai::ai_reference_actor_iterator_next(&member_iterator);
        if (member == 0) {
            return;
        }
        do {
            if (7 < (int16_t)target_count) break;
            targets[(int16_t)target_count] = ((actor *)member)->unit_index;
            target_count = target_count + 1;
            member = halo::ai::ai_reference_actor_iterator_next(&member_iterator);
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
            if (halo::ai::flag_set(squad_definition->flags, halo::tags::scenario_squad_tag_flag::automatic_migration)) {
                encounter_squad_state *squad_state = &halo::ai::globals().squad_states[(int16_t)(self->first_squad + squad_index)];
                uint32_t bit = 1u << (bit_index & 0x1f);
                int16_t word = bit_index >> 5;

                squad_considered_mask[word] |= bit;
                total_occupancy = total_occupancy + squad_state->living_count;

                if (squad_state->automatic_migration != 0) { // "has valid platoon" gate (encounter_squad_state+0x10)
                    encounter_platoon_state *platoon_state;
                    int16_t platoon_index = squad_definition->platoon;
                    uint32_t trigger;

                    squad_active_mask[word] |= bit;

                    if ((platoon_index < 0) || (encounter_definition->platoons.count <= (uint32_t)platoon_index) ||
                        (((uint8_t *)&halo::ai::globals().platoon_states[(int16_t)(self->first_platoon + platoon_index)])[0] == 0)) {
                        trigger = squad_definition->attacking | squad_definition->attacking_search | squad_definition->attacking_guard;
                    } else {
                        trigger = squad_definition->defending | squad_definition->defending_search | squad_definition->defending_guard;
                    }
                    (void)platoon_state;

                    squad_trigger_mask[bit_index] = trigger;
                    combined_trigger_mask |= trigger;

                    if (0 < squad_state->living_count) {
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
        chosen_object = targets[0];
        halo::objects::object_get_position(&chosen_position, targets[0]);
    } else {
        int16_t t;
        datum_index next_actor;
        uint8_t have_target;

        for (t = 0; t < (int16_t)target_count; t = t + 1) {
            best_distance_to_target[t] = 3.4028235e+38f;
        }
        for (t = 0; t < (int16_t)target_count; t = t + 1) {
            halo::objects::object_get_position(&target_positions[t], targets[t]);
        }

        have_target = 0;
        next_actor = (datum_index)halo::k_dword_none;
        if (halo::ai::globals().state->actors_valid != 0) {
            if (encounter_index == (datum_index)halo::k_dword_none) {
                next_actor = halo::ai::globals().state->first_encounterless_actor;
            } else {
                next_actor = self->first_actor;
            }
            have_target = (uint8_t)(next_actor != (datum_index)halo::k_dword_none);
        }

        while (have_target && (next_actor != (datum_index)halo::k_dword_none)) {
            actor *member = halo::ai::actor_at(next_actor);
            int16_t member_squad = member->squad_index;
            next_actor = member->next_in_encounter;

            if (((squad_considered_mask[member_squad >> 5] & (1u << (member_squad & 0x1f))) != 0) &&
                (0 < (int16_t)target_count)) {
                for (t = 0; t < (int16_t)target_count; t = t + 1) {
                    float dx = member->body_position.x - target_positions[t].x;
                    float dy = member->body_position.y - target_positions[t].y;
                    float dz = member->body_position.z - target_positions[t].z;
                    float d2 = dz * dz + dy * dy + dx * dx;
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
        chosen_object = (datum_index)halo::k_dword_none;
        for (t = 0; t < (int16_t)target_count; t = t + 1) {
            if (best_distance_to_target[t] < 3.4028235e+38f) {
                chosen_object = targets[t];
                chosen_position = target_positions[t];
            }
        }
    }

    if (chosen_object == (datum_index)halo::k_dword_none) {
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
                float d2 = dx * dx + dz * dz + dy * dy;
                if (group_distance[group] <= d2) {
                    d2 = group_distance[group];
                }
                group_distance[group] = d2;
            }
        }

        best_squad_distance = 3.4028235e+38f;
        best_occupied_distance = -3.4028235e+38f;
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
                    float leash = (self->follow_distance <= 0.0f) ? 2.0f : self->follow_distance;
                    if ((float)sqrt(best_occupied_distance) - leash <= (float)sqrt(best_squad_distance)) {
                        return;
                    }
                }

                {
                    datum_index next_actor;
                    uint8_t have_target;

                    have_target = 0;
                    next_actor = (datum_index)halo::k_dword_none;
                    if (halo::ai::globals().state->actors_valid != 0) {
                        if (encounter_index == (datum_index)halo::k_dword_none) {
                            next_actor = halo::ai::globals().state->first_encounterless_actor;
                        } else {
                            next_actor = self->first_actor;
                        }
                        have_target = (uint8_t)(next_actor != (datum_index)halo::k_dword_none);
                    }

                    while (have_target && (next_actor != (datum_index)halo::k_dword_none)) {
                        actor *member = halo::ai::actor_at(next_actor);
                        int16_t member_squad = member->squad_index;
                        datum_index current_actor = next_actor;

                        next_actor = member->next_in_encounter;

                        if (((squad_considered_mask[member_squad >> 5] & (1u << (member_squad & 0x1f))) != 0) &&
                            (member_squad != best_squad)) {
                            member->firing_position_index = (int16_t)halo::k_word_none;
                            if ((member->active_movement.type == 3) || (member->active_movement.type == 4)) {
                                member->active_movement.type = 0;
                                member->active_movement.extra = halo::k_dword_none;
                            }
                            {
                                void (*dispatch)(datum_index) = *(void (**)(datum_index))
                                    ((uint8_t *)&ai_actor_mode_dispatch_table + member->mode * 0x38);
                                if (dispatch != 0) {
                                    dispatch(current_actor);
                                }
                            }

                            if (member->encounterless == 0) {
                                if (member->encounter_index != (datum_index)halo::k_dword_none) {
                                    halo::ai::encounter_remove_actor(current_actor, 0);
                                }
                            } else {
                                halo::ai::ai_actor_unlink_from_unassigned_list(current_actor);
                            }

                            if (encounter_index == (datum_index)halo::k_dword_none) {
                                if (halo::ai::globals().state->actors_valid != 0) {
                                    member->next_in_encounter = halo::ai::globals().state->first_encounterless_actor;
                                    halo::ai::globals().state->first_encounterless_actor = current_actor;
                                    member->encounterless = 1;
                                    *(uint16_t *)&member->activation_delay = -(uint16_t)(member->active != 0) & 0x5a;
                                    member->firing_position_index = (int16_t)halo::k_word_none;
                                    if ((member->active_movement.type == 3) || (member->active_movement.type == 4)) {
                                        member->active_movement.type = 0;
                                        member->active_movement.extra = halo::k_dword_none;
                                    }
                                    {
                                        void (*dispatch)(datum_index) = *(void (**)(datum_index))
                                            ((uint8_t *)&ai_actor_mode_dispatch_table + member->mode * 0x38);
                                        if (dispatch != 0) {
                                            dispatch(current_actor);
                                        }
                                    }
                                }
                            } else {
                                halo::ai::encounter_add_actor(best_squad, current_actor, encounter_index, 1);
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

/**
 * Behaviour of encounter release stale props, moved unchanged from the original free function.
 *
 * @address 0x4382b0
 */
void EncounterView::release_stale_props()
{
    datum_index encounter_index = handle;
    encounter *enc;
    actor *a;
    prop *p;
    prop *props;
    datum_index actor_index;
    datum_index current;
    datum_index prop_index;
    datum_index next_prop;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    enc->stood_down = 1;
    enc->enemy_death_count = 0;
    halo::ai::squad_recent_object_list_clear(encounter_index);

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];
        actor_index = a->next_in_encounter;

        next_prop = a->first_prop;
        while (next_prop != (datum_index)k_datum_index_none) {
            props = (prop *)halo::ai::globals().prop_data->data;
            prop_index = next_prop;
            p = &props[prop_index & halo::k_slot_mask];
            next_prop = p->next_in_actor;

            if (3 < p->state && p->state < 6 && p->enemy != 0 &&
                prop_index != a->target_unit_index) {
                props[p->pair_index & halo::k_slot_mask].pair_index = (datum_index)k_datum_index_none;
                halo::ai::actor_replace_object_reference(current, k_datum_index_none, prop_index);
                halo::ai::actor_unlink_prop(current, prop_index);
                halo::memory::datum_delete(halo::ai::globals().prop_data, prop_index);
            }
        }
    }
}

/**
 * Behaviour of encounter remove actor, moved unchanged from the original free function.
 *
 * @address 0x436620
 */
void Encounters::remove_actor(datum_index actor_index, uint8_t skip_counters)
{
    actor *actors;
    actor *a;
    encounter *enc;
    encounter_squad_state *squad_state;
    encounter_platoon_state *platoon_state;
    datum_index *link;
    datum_index current;

    if (halo::ai::globals().state->actors_valid == 0) {
        return;
    }

    actors = (actor *)halo::ai::globals().actor_data->data;
    a = &actors[actor_index & halo::k_slot_mask];
    if (a->encounter_index == (datum_index)k_datum_index_none) {
        return;
    }
    enc = &((encounter *)halo::ai::globals().encounter_data->data)[a->encounter_index & halo::k_slot_mask];

    link = &enc->first_actor;
    current = enc->first_actor;
    while (current != actor_index) {
        link = &actors[current & halo::k_slot_mask].next_in_encounter;
        current = *link;
    }
    *link = a->next_in_encounter;

    if (skip_counters == 0) {
        squad_state = &halo::ai::globals().squad_states[(int16_t)(a->squad_index + enc->first_squad)];
        enc->member_count = enc->member_count - 1;
        if (a->counts_toward_encounter != 0) {
            enc->live_count = enc->live_count - 1;
        }
        squad_state->member_count = squad_state->member_count - 1;
        if (a->platoon_index != -1) {
            platoon_state =
                &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + a->platoon_index)];
            platoon_state->member_count = platoon_state->member_count - 1;
        }
    }

    a->next_in_encounter = (datum_index)k_datum_index_none;
    a->encounter_index = (datum_index)k_datum_index_none;
    a->platoon_index = -1;
    a->squad_index = -1;
    enc->dirty = 1;
}

/**
 * Behaviour of encounter set team, moved unchanged from the original free function.
 *
 * @address 0x435b30
 */
void EncounterView::set_team(int16_t team)
{
    datum_index encounter_index = handle;
    encounter *enc;
    datum_index actor_index;
    datum_index current;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    enc->team = team;

    actor_index = (datum_index)k_datum_index_none;
    if (halo::ai::globals().state->actors_valid != 0) {
        if (encounter_index == (datum_index)halo::k_dword_none) {
            actor_index = halo::ai::globals().state->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    current = (datum_index)k_datum_index_none;
    while (halo::ai::globals().state->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        actor_index = ((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask].next_in_encounter;
        halo::ai::actor_propagate_unit_field(current, team);
    }
    halo::ai::ai_recompute_all_relationship_flags();
}

/**
 * Behaviour of encounter spawn squads, moved unchanged from the original free function.
 *
 * @address 0x437510
 */
void EncounterView::spawn_squads(int16_t platoon_filter, int16_t squad_filter)
{
    datum_index encounter_index = handle;
    ScenarioEncounter *definition;
    ScenarioSquad *squad;
    encounter *enc;
    int32_t squad_index;
    int32_t spawn_count;
    int16_t actor_type;
    int16_t remaining;
    int32_t leader_chance;
    int32_t margin;
    uint8_t all_squads;

    if (halo::ai::globals().state->actors_valid == 0) {
        return;
    }
    definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];

    all_squads = (uint8_t)(platoon_filter == -1 && squad_filter == -1);

    squad_index = 0;
    if (0 < definition->squads.count) {
        do {
            squad = &((ScenarioSquad *)definition->squads.pointer)[squad_index];
            if (all_squads != 0 || (int16_t)squad_index == squad_filter ||
                ((int16_t)squad->platoon != -1 && (int16_t)squad->platoon == platoon_filter)) {

                leader_chance = 0;
                spawn_count = 0;
                switch (halo::main::globals().game_globals->difficulty) {
                case 0:
                case 1:
                    spawn_count = (int32_t)(uint16_t)squad->normal_diff_count;
                    break;
                case 2:
                    spawn_count = ((int32_t)squad->insane_diff_count +
                                   (int32_t)squad->normal_diff_count) / 2;
                    break;
                case 3:
                    spawn_count = (int32_t)(uint16_t)squad->insane_diff_count;
                    break;
                }

                actor_type = halo::ai::ai_squad_resolve_actor_type(squad);
                remaining = (int16_t)spawn_count;

                switch (squad->unique_leader_type) {
                case 0:
                    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
                    if (actor_type == 7) {
                        if (enc->live_count == 0) {
                            margin = (int32_t)enc->member_count + (int32_t)remaining - 4;
                        } else if (enc->live_count == 1) {
                            margin = (int32_t)enc->member_count + (int32_t)remaining - 10;
                        } else {
                            break;
                        }
                        if (margin >= 0) {
                            goto leader_coin_flip;
                        }
                    }
                    break;
                case 2:
leader_coin_flip:
                    if (actor_type == 7) {
                        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                        leader_chance = 100 + (int32_t)(halo::math::globals().random_seed_global >> 0x1f);
                    }
                    break;
                case 3:
                    if (actor_type == 7) {
                        leader_chance = 100;
                    }
                    break;
                case 4:
                    if (actor_type == 7) {
                        leader_chance = 0x65;
                    }
                    break;
                }

                if (0 < remaining) {
                    uint32_t left = (uint32_t)spawn_count & halo::k_slot_mask;
                    do {
                        halo::ai::encounter_squad_spawn_actor(encounter_index, squad_index,
                                                    leader_chance, 0);
                        leader_chance = 0;
                        left = left - 1;
                    } while (left != 0);
                }
            }
            squad_index = (int32_t)(int16_t)(squad_index + 1);
        } while (squad_index < definition->squads.count);
    }

    halo::ai::encounter_recompute_morale(encounter_index);
    halo::ai::encounters_update_activation();
}

/**
 * Behaviour of encounter squad clear spawn delay, moved unchanged from the original free function.
 *
 * @address 0x439270
 */
void EncounterView::squad_clear_spawn_delay(int16_t squad_index)
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    encounter_squad_state *squad_state;

    self = halo::ai::encounter_at(encounter_index);
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];
    squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];

    squad_state = &halo::ai::globals().squad_states[(int16_t)(self->first_squad + squad_index)];
    squad_state->squad_delay_ticks = 0;

    if (halo::ai::flag_set(squad_definition->flags, halo::tags::scenario_squad_tag_flag::magic_sight_after_timer)) {
        halo::ai::ai_reference_respawn_all_players(((uint32_t)(((uint16_t)squad_index & 0xff) | 0x8000) << 16) |
                                         (encounter_index & halo::k_slot_mask));
    }
    return;
}

/**
 * Behaviour of encounter squad reset starting location mask, moved unchanged from the original free
 * function.
 *
 * @address 0x436f90
 */
void EncounterView::squad_reset_starting_location_mask(int16_t squad_index)
{
    datum_index encounter_index = handle;
    encounter *enc;
    encounter_squad_state *squad_state;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    ScenarioActorStartingLocation *locations;
    uint32_t *fill;
    uint32_t dword_count;
    int16_t i;
    int32_t location_index;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    squad_state = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + squad_index)];

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];
    squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];

    fill = &squad_state->starting_location_free;
    dword_count = (uint32_t)((squad_definition->starting_locations.count + 0x1f) >> 5);
    for (; dword_count != 0; dword_count = dword_count - 1) {
        *fill = halo::k_dword_none;
        fill = fill + 1;
    }

    locations = (ScenarioActorStartingLocation *)squad_definition->starting_locations.pointer;
    i = 0;
    location_index = 0;
    if (0 < squad_definition->starting_locations.count) {
        do {
            if ((locations[location_index].flags & 1) != 0) {
                (&squad_state->starting_location_mask)[location_index >> 5] =
                    (&squad_state->starting_location_mask)[location_index >> 5] |
                    (1 << (location_index & 0x1f));
            }
            i = i + 1;
            location_index = (int32_t)i;
        } while (location_index < squad_definition->starting_locations.count);
    }
}

/**
 * Behaviour of encounter squad spawn actor, moved unchanged from the original free function.
 *
 * @address 0x438e20
 */
uint8_t EncounterView::squad_spawn_actor(int16_t squad_index, uint32_t unit_type_index, uint32_t unused)
{
    datum_index encounter_index = handle;
    uint8_t *encounter_definition = (uint8_t *)halo::scenario::globals().scenario->encounters.pointer + (encounter_index & halo::k_slot_mask) * 0xb0;
    uint8_t *squad = *(uint8_t **)(encounter_definition + 0x84) + squad_index * 0xe8;
    uint8_t *starting_location;
    uint8_t *palette_entry;
    datum_index variant_tag;
    int16_t location_index;
    int16_t palette_index;
    uint8_t use_palette = 0;

    location_index = halo::ai::squad_pick_random_starting_location(encounter_index, squad_index);
    if (location_index == -1) {
        return 0;
    }
    starting_location = *(uint8_t **)(squad + 0xd4) + location_index * 0x1c;
    palette_index = *(int16_t *)(squad + 0x20);
    if (*(int16_t *)(starting_location + 0x18) != -1) {
        palette_index = *(int16_t *)(starting_location + 0x18);
    }
    if (palette_index < 0 || palette_index >= (int32_t)halo::scenario::globals().scenario->actor_palette.count) {
        return 0;
    }
    palette_entry = (uint8_t *)halo::scenario::globals().scenario->actor_palette.pointer + palette_index * 0x10;
    variant_tag = *(datum_index *)(palette_entry + 0xc);
    if (variant_tag == k_datum_index_none) {
        return 0;
    }
    if (*(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[variant_tag & halo::k_slot_mask].data + 0x30) != k_datum_index_none) {
        uint8_t enabled = 0;
        float bias = 0.0f;

        halo::ai::ai_get_difficulty_request(*(int16_t *)(squad + 0x80), &enabled, &use_palette, &bias);
        if (enabled) {
            use_palette = halo::ai::ai_drift_zone_bias(encounter_index, squad_index, bias);
        }
    }
    return halo::ai::actor_place_new_unit(variant_tag, encounter_index, squad_index, use_palette, (uint16_t)unit_type_index,
        (const actor_placement_request *)starting_location) != k_datum_index_none;
}

/**
 * Behaviour of encounter squad spawn reinforcement, moved unchanged from the original free function.
 *
 * @address 0x438f60
 */
uint32_t EncounterView::squad_spawn_reinforcement(int16_t squad_index)
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    encounter_squad_state *squad_state;
    uint32_t result;
    float randomized;

    result = (uint32_t)halo::ai::globals().state;
    if (halo::ai::globals().state->actors_valid != 0) {
        result = halo::ai::encounter_squad_spawn_actor(encounter_index, squad_index, 0, 1);
        if ((int8_t)result != 0) {
            self = halo::ai::encounter_at(encounter_index);
            encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];
            squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];
            squad_state = &halo::ai::globals().squad_states[(int16_t)(self->first_squad + squad_index)];

            self->living_count = self->living_count + 1;
            squad_state->living_count = squad_state->living_count + 1;
            if (0 < squad_definition->respawn_total) {
                squad_state->respawn_budget = squad_state->respawn_budget - 1;
            }

            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            {
                float lo = encounter_definition->respawn_delay[0];
                float hi = encounter_definition->respawn_delay[1];
                float r = (float)((uint32_t)halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536;

                randomized = (r * (hi - lo) + lo) * ticks_per_second;
                self->respawn_delay_ticks = (int16_t)(int32_t)randomized;
            }

            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            {
                float lo = squad_definition->respawn_delay[0];
                float hi = squad_definition->respawn_delay[1];
                float r = (float)((uint32_t)halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536;

                randomized = (r * (hi - lo) + lo) * ticks_per_second;
                squad_state->respawn_delay_ticks = (int16_t)(int32_t)randomized;
            }
        }
    }
    return result & 0xffffff00;
}

/**
 * Behaviour of encounter update platoon defending flag, moved unchanged from the original free function.
 *
 * @address 0x4393b0
 */
void EncounterView::update_platoon_defending_flag()
{
    datum_index encounter_index = handle;
    encounter *self;
    ScenarioEncounter *encounter_definition;
    encounter_platoon_state *platoon_state;
    uint8_t not_defending;
    int16_t platoon_index;

    self = halo::ai::encounter_at(encounter_index);
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index & halo::k_slot_mask];

    platoon_index = 0;
    if (0 < self->platoon_count) {
        do {
            platoon_state = &halo::ai::globals().platoon_states[(int16_t)(self->first_platoon + platoon_index)];

            if (0 < platoon_state->living_count) {
                if (((uint8_t *)platoon_state)[1] == 0) {
                    ((uint8_t *)platoon_state)[1] = halo::ai::encounter_evaluate_platoon_condition(encounter_index,
                        (const ai_platoon_condition *)((uint8_t *)encounter_definition->platoons.pointer + platoon_index * 0xac + 0x3c));
                }
                if ((((uint8_t *)platoon_state)[2] != 0) || (((uint8_t *)platoon_state)[1] == 0)) {
                    not_defending = ~(uint8_t)(((ScenarioPlatoon *)encounter_definition->platoons.pointer)[platoon_index].flags >> 2) & 1;
                    if ((platoon_state->defending != not_defending) &&
                        (halo::ai::encounter_evaluate_platoon_condition(encounter_index,
                            (const ai_platoon_condition *)((uint8_t *)encounter_definition->platoons.pointer + platoon_index * 0xac + 0x30)) != 0)) {
                        platoon_state->defending = not_defending;
                    }
                }
            }

            platoon_index = platoon_index + 1;
        } while (platoon_index < self->platoon_count);
    }
    return;
}

/**
 * Behaviour of encounters initialize, moved unchanged from the original free function.
 *
 * @address 0x435c00
 */
void Encounters::initialize()
{
    uint32_t reserved_size;

    halo::ai::globals().encounter_data = (data_array *)halo::saved_games::game_state_new(const_cast<char *>("encounter"), k_encounter_data_maximum_count, k_encounter_size);

    halo::ai::globals().squad_states = (encounter_squad_state *)(halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x8000;
    reserved_size = 0x8000;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, &reserved_size, 4);

    halo::ai::globals().platoon_states = (encounter_platoon_state *)(halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0x1000;
    reserved_size = 0x1000;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, &reserved_size, 4);

    halo::ai::globals().pursuit_data = (data_array *)halo::saved_games::game_state_new(const_cast<char *>("ai pursuit"), k_ai_pursuit_data_maximum_count, k_ai_pursuit_size);
}

/**
 * Behaviour of encounters note hostile object, moved unchanged from the original free function.
 *
 * @address 0x435f90
 */
void Encounters::note_hostile_object(datum_index object_index)
{
    object *obj;
    encounter_iterator iterator;
    encounter *enc;
    int16_t object_team;
    int16_t encounter_team;
    int32_t pair;
    char hostile;

    obj = halo::ai::object_at(object_index);
    object_team = obj->owner_team;
    if (object_team == -1) {
        return;
    }

    if (halo::ai::globals().state->actors_valid != 0) {
        iterator.data = halo::ai::globals().encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.active_only = 1;
    }

    while (halo::ai::globals().state->actors_valid != 0) {
        do {
            enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        if (enc == 0) {
            return;
        }

        object_team = obj->owner_team;
        encounter_team = enc->team;
        if (halo::game::globals().current_engine == 0) {
            hostile = 0;
            if (0 <= encounter_team && encounter_team < 10 &&
                0 <= object_team && object_team < 10) {
                pair = (int32_t)object_team + encounter_team * 10;
                hostile = (char)(1 - ((1 << (pair & 0x1f)) &
                    team_pair_data->enemy_bits[pair >> 5]) != 0);
                if (hostile == 0) {
                    continue;
                }
            }
        } else {
            hostile = (char)(encounter_team != object_team);
            if (hostile == 0) {
                continue;
            }
        }

        if (enc->ever_had_target != 0 && enc->stood_down == 0 && enc->post_combat == 0) {
            enc->enemy_death_count = enc->enemy_death_count + 1;
        }
    }
}

/**
 * Behaviour of encounters recompute dirty, moved unchanged from the original free function.
 *
 * @address 0x435f00
 */
void Encounters::recompute_dirty()
{
    encounter_iterator iterator;
    encounter *enc;

    if (halo::ai::globals().state->actors_valid != 0) {
        iterator.data = halo::ai::globals().encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.active_only = 0;
    }

    for (;;) {
        if (halo::ai::globals().state->actors_valid == 0) {
            return;
        }
        do {
            enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        iterator.encounter_index = iterator.index;
        if (enc == 0) {
            return;
        }
        if (enc->dirty != 0) {
            halo::ai::encounter_recompute_morale(iterator.encounter_index);
        }
    }
}

/**
 * Behaviour of encounters reset, moved unchanged from the original free function.
 *
 * @address 0x435cb0
 */
void Encounters::reset()
{
    Scenario *scenario;
    int32_t i;
    int16_t encounter_index;
    uint32_t *zero;
    int16_t platoon_cursor;
    int16_t squad_cursor;

    scenario = halo::scenario::globals().scenario;
    squad_cursor = 0;
    platoon_cursor = 0;

    halo::ai::globals().encounter_data->valid = 1;
    halo::memory::data_delete_all(halo::ai::globals().encounter_data);
    halo::ai::globals().pursuit_data->valid = 1;
    halo::memory::data_delete_all(halo::ai::globals().pursuit_data);

    zero = (uint32_t *)halo::ai::globals().squad_states;
    for (i = 0x2000; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }
    zero = (uint32_t *)halo::ai::globals().platoon_states;
    for (i = 0x400; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }

    encounter_index = 0;
    if (0 < scenario->encounters.count) {
        do {
            halo::ai::encounter_new(&squad_cursor,
                &((ScenarioEncounter *)scenario->encounters.pointer)[encounter_index],
                &platoon_cursor);
            encounter_index = encounter_index + 1;
        } while ((int32_t)encounter_index < scenario->encounters.count);
    }
}

/**
 * Behaviour of encounters spawn initial, moved unchanged from the original free function.
 *
 * @address 0x435d50
 */
void Encounters::spawn_initial()
{
    encounter_iterator iterator;
    encounter *enc;
    ScenarioEncounter *definition;

    if (halo::ai::globals().state->actors_valid != 0) {
        iterator.data = halo::ai::globals().encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.active_only = 0;
    }

    for (;;) {
        if (halo::ai::globals().state->actors_valid == 0) {
            return;
        }
        do {
            enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        iterator.encounter_index = iterator.index;
        if (enc == 0) {
            return;
        }
        definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
            [iterator.encounter_index & halo::k_slot_mask];
        if ((~definition->flags & 1) != 0) {
            halo::ai::encounter_spawn_squads(iterator.encounter_index, -1, -1);
        }
    }
}

/**
 * Every 30 ticks it flushes the dirty-encounter recompute and re-evaluates which encounters should be
 * active; every tick it runs the full per-encounter update for the one fifteenth of the live encounters
 * whose index matches this tick.
 *
 * @address 0x435e00
 */
void Encounters::update()
{
    int32_t tick;
    encounter_iterator iterator;
    encounter *enc;

    tick = halo::game::globals().game_time->game_time;
    if (tick % 0x1e == 0) {
        halo::ai::encounters_recompute_dirty();
        halo::ai::encounters_update_activation();
    }

    if (halo::ai::globals().state->actors_valid != 0) {
        iterator.data = halo::ai::globals().encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.active_only = 1;
    }

    for (;;) {
        if (halo::ai::globals().state->actors_valid == 0) {
            return;
        }
        do {
            enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        iterator.encounter_index = iterator.index;
        if (enc == 0) {
            return;
        }
        if ((int16_t)((uint32_t)(iterator.encounter_index & halo::k_slot_mask) % 0xf) ==
            (int16_t)(tick % 0xf)) {
            halo::ai::encounter_recompute_morale(iterator.encounter_index);
            halo::ai::encounter_advance_grenade_timers(iterator.encounter_index);
            halo::ai::encounter_process_squad_reinforcements(iterator.encounter_index);
            halo::ai::encounter_decay_squad_spawn_delays(iterator.encounter_index);
            halo::ai::encounter_update_platoon_defending_flag(iterator.encounter_index);
            halo::ai::encounter_redistribute_squads_toward_targets(iterator.encounter_index);
            halo::ai::encounter_propagate_platoon_state_to_actors(iterator.encounter_index);
        }
    }
}

/**
 * Behaviour of encounters update activation, moved unchanged from the original free function.
 *
 * @address 0x437e20
 */
void Encounters::update_activation()
{
    uint32_t *visible_clusters;
    datum_index actor_index;
    datum_index current;
    actor *a;
    object *obj;
    swarm *sw;
    datum_index object_index;
    datum_index root_index;
    datum_index child_index;
    int16_t cluster;
    int16_t component_count;
    int16_t component_index;
    encounter_iterator iterator;
    encounter *enc;
    ScenarioEncounter *definition;
    uint32_t encounter_clusters[16];
    uint32_t dword_count;
    uint8_t overlaps;
    uint8_t wants_active;
    uint8_t any_dependent_pending;
    int16_t i;
    uint16_t *dependents;

    visible_clusters = (uint32_t *)((uint8_t *)halo::game::globals().local_player_globals + 0x18);

    actor_index = halo::ai::globals().state->first_encounterless_actor;
    while (actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)halo::ai::globals().actor_data->data)[current & halo::k_slot_mask];

        if (a->swarm == 0) {
            root_index = (datum_index)k_datum_index_none;
            if (a->unit_index != (datum_index)k_datum_index_none) {
                object_index = a->unit_index;
                do {
                    root_index = object_index;
                    obj = halo::ai::object_at(root_index);
                    object_index = obj->parent_object;
                } while (object_index != (datum_index)k_datum_index_none);
            }
            obj = halo::ai::object_at(root_index);
            cluster = obj->location_cluster_index;
            if (cluster == -1) {
                a->can_go_dormant = 1;
            } else {
                a->can_go_dormant = (uint8_t)(1 - (((1 << (cluster & 0x1f)) &
                    visible_clusters[(int32_t)cluster >> 5]) != 0));
            }
        } else {
            a->can_go_dormant = 1;
            if (a->swarm_index == (datum_index)k_datum_index_none) {
                child_index = a->cluster_unit_index;
                if (child_index != (datum_index)k_datum_index_none) {
                    do {
                        root_index = (datum_index)k_datum_index_none;
                        for (object_index = child_index;
                             object_index != (datum_index)k_datum_index_none;
                             object_index = *(datum_index *)((uint8_t *)((object_header *)
                                 halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data + 0x11c)) {
                            root_index = object_index;
                        }
                        obj = halo::ai::object_at(root_index);
                        cluster = obj->location_cluster_index;
                        if (cluster != -1 &&
                            (visible_clusters[(int32_t)cluster >> 5] &
                             (1 << (cluster & 0x1f))) != 0) {
                            a->can_go_dormant = 0;
                            break;
                        }
                        obj = halo::ai::object_at(child_index);
                        child_index = *(datum_index *)((uint8_t *)obj + 0x1fc);
                    } while (child_index != (datum_index)k_datum_index_none);
                }
            } else {
                sw = &((swarm *)halo::ai::globals().swarm_data->data)[a->swarm_index & halo::k_slot_mask];
                component_count = sw->component_count;
                component_index = 0;
                if (0 < component_count) {
                    do {
                        root_index = (datum_index)k_datum_index_none;
                        for (object_index = sw->unit_index[component_index];
                             object_index != (datum_index)k_datum_index_none;
                             object_index = *(datum_index *)((uint8_t *)((object_header *)
                                 halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data + 0x11c)) {
                            root_index = object_index;
                        }
                        obj = halo::ai::object_at(root_index);
                        cluster = obj->location_cluster_index;
                        if (cluster != -1 &&
                            (visible_clusters[(int32_t)cluster >> 5] &
                             (1 << (cluster & 0x1f))) != 0) {
                            a->can_go_dormant = 0;
                            break;
                        }
                        component_index = component_index + 1;
                    } while (component_index < component_count);
                }
            }
        }

        if (a->can_go_dormant == 0 || a->force_active != 0) {
            a->activation_delay = 0x5a;
            if (a->active != 1) {
                if (a->swarm == 0 || (halo::ai::actor_create_swarm(current),
                                      a->swarm_index != (datum_index)k_datum_index_none)) {
                    a->active = 1;
                    if (a->awareness_level == 0) {
                        halo::ai::actor_set_units_active(current, 0);
                    }
                } else {
                    a->swarm_pending = 1;
                }
            }
        } else if (a->activation_delay < 0x1f) {
            a->activation_delay = 0;
            if (a->active != 0) {
                halo::ai::actor_clear_perceived_props(current);
                halo::ai::actor_delete_swarm(current);
                halo::ai::actor_set_units_active(current, 1);
                a->active = 0;
                a->deactivation_time = (datum_index)halo::game::globals().game_time->game_time;
            }
        } else {
            a->activation_delay = (int16_t)(a->activation_delay - 0x1e);
        }
        actor_index = a->next_in_encounter;
    }

    iterator.data = halo::ai::globals().encounter_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;

    enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
    while (enc != 0) {
        definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
            [iterator.index & halo::k_slot_mask];
        wants_active = (uint8_t)(0 < enc->respawn_delay_ticks || enc->force_active != 0);

        if ((int16_t)definition->precomputed_bsp_index == -1 ||
            (int16_t)definition->precomputed_bsp_index == halo::scenario::globals().structure_bsp_index) {

            halo::ai::encounter_gather_occupied_clusters(iterator.index, encounter_clusters, 1,
                                               visible_clusters);

            overlaps = 0;
            dword_count = (uint32_t)(((*(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp + 0x134)) + 0x1f) >> 5);
            i = (int16_t)dword_count - 1;
            if (0 <= i) {
                uint32_t n = dword_count & halo::k_slot_mask;
                uint32_t *cursor = encounter_clusters + i;
                do {
                    if ((visible_clusters[cursor - encounter_clusters] & *cursor) != 0) {
                        overlaps = 1;
                    }
                    cursor = cursor - 1;
                    n = n - 1;
                } while (n != 0);
            }

            if (wants_active != 0 || overlaps != 0) {
                enc->activation_delay = 0x96;
                halo::ai::encounter_activate(iterator.index);
                enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
                continue;
            }
        }

        if (enc->units_active == 0 || enc->activation_delay <= 0x1e) {
            any_dependent_pending = 0;
            if (0 < enc->activation_link_count) {
                dependents = (uint16_t *)&enc->activation_link[0];
                for (i = enc->activation_link_count; i != 0; i = i - 1) {
                    if (0 < ((encounter *)halo::ai::globals().encounter_data->data)[*dependents].activation_delay) {
                        any_dependent_pending = 1;
                    }
                    dependents = dependents + 1;
                }
            }
            enc->activation_delay = 0;
            if (any_dependent_pending != 0) {
                halo::ai::encounter_activate(iterator.index);
            } else {
                halo::ai::encounter_deactivate(iterator.index);
            }
        } else {
            enc->activation_delay = (int16_t)(enc->activation_delay - 0x1e);
        }
        enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)&iterator);
    }
}

/**
 * Returns the winning object/unit index, or k_datum_index_none if the list was empty.
 *
 * @address 0x41c2c0
 */
datum_index Encounters::find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index, char stamp_group)
{
    actor *self;
    datum_index swarm_index;
    swarm *group;
    int16_t i;
    swarm_component *component;
    object *obj;
    object *stamp_target;
    datum_index cursor;
    float dx, dy, dz;
    float dist_sq;
    datum_index best;
    float best_dist;
    float rx, ry, rz;

    self = halo::ai::actor_at(actor_index);
    swarm_index = self->swarm_index;
    best = k_datum_index_none;
    best_dist = 3.4028235e+38f;
    rx = *(float *)((uint8_t *)reference + 0xc);
    ry = *(float *)((uint8_t *)reference + 0x10);
    rz = *(float *)((uint8_t *)reference + 0x14);

    if (swarm_index == k_datum_index_none) {
        cursor = self->cluster_unit_index;
        if (cursor == k_datum_index_none) {
            return k_datum_index_none;
        }
        do {
            real_point3d position;
            obj = halo::ai::object_at(cursor);
            halo::objects::object_get_position(&position, cursor);
            dx = rx - position.x;
            dy = ry - position.y;
            dz = rz - position.z;
            dist_sq = dx * dx + dy * dy + dz * dz;
            if (cursor == exclude_index) {
                dist_sq = dist_sq * 0.36f;
            }
            if (dist_sq < best_dist) {
                best = cursor;
                best_dist = dist_sq;
            }
            if (stamp_group != 0) {
                if (obj->cluster_stamp != halo::physics::globals().object_cluster_stamp) {
                    obj->cluster_stamp = halo::physics::globals().object_cluster_stamp;
                }
            }
            cursor = *(datum_index *)((uint8_t *)obj + 0x1fc);
        } while (cursor != k_datum_index_none);
        return best;
    }

    group = halo::ai::swarm_at(swarm_index);
    if (0 < group->component_count) {
        for (i = 0; i < group->component_count; i++) {
            component = (swarm_component *)((uint8_t *)halo::ai::globals().swarm_component_data->data +
                                            (group->component_index[i] & halo::k_slot_mask) * sizeof(swarm_component));
            dx = rx - component->position.x;
            dy = ry - component->position.y;
            dz = rz - component->position.z;
            dist_sq = dx * dx + dy * dy + dz * dz;

            if ((*((uint8_t *)component + 2) & 2) == 0) {
                if (group->unit_index[i] == exclude_index) {
                    dist_sq = dist_sq * 0.36f;
                }
            } else {
                dist_sq = dist_sq * 2.25f;
            }

            if (dist_sq < best_dist) {
                best = group->unit_index[i];
                best_dist = dist_sq;
            }

            if (stamp_group != 0) {
                stamp_target = halo::ai::object_at(group->unit_index[i]);
                if (stamp_target->cluster_stamp != halo::physics::globals().object_cluster_stamp) {
                    stamp_target->cluster_stamp = halo::physics::globals().object_cluster_stamp;
                }
            }
        }
        return best;
    }

    return k_datum_index_none;
}

/**
 * The phase-4 summary ("looks up a squad definition's index... Linear-searches Scenario.encounters for one
 * whose name matches (case-insensitively, up to 32 characters), returning its index or -1 if there are no
 * encounters or none match.
 *
 * @address 0x432200
 */
int32_t Encounters::find_encounter_index_by_name(Scenario *scenario, char *name)
{
    int32_t index;
    uint8_t *cursor;

    if (scenario->encounters.count <= 0) {
        return -1;
    }

    index = 0;
    cursor = (uint8_t *)scenario->encounters.pointer;
    while (_strnicmp((char *)cursor, name, 0x20) != 0) {
        index = index + 1;
        cursor += sizeof(ScenarioEncounter);
        if (scenario->encounters.count <= index) {
            return -1;
        }
    }
    return index;
}

/**
 * Locations still set in starting_location_mask are preferred and are consumed from both masks; once that
 * pool is empty the free mask is used, and when it too runs out it is refilled to all ones and the draw is
 * retried. Returns -1 when the squad has no usable location at all.
 *
 * @address 0x437220
 */
int16_t EncounterView::pick_random_starting_location(int16_t squad_index)
{
    datum_index encounter_index = handle;
    encounter *enc;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    uint32_t *masks;
    uint32_t taken[2];
    uint32_t *slot;
    uint32_t clear_mask;
    uint32_t bit;
    uint32_t draw;
    uint32_t dword_count;
    uint32_t *fill;
    int16_t available;
    int16_t cursor;
    int16_t chosen;
    int16_t free_available;
    int32_t location_index;
    int32_t location_count;
    char any_unavailable;

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
        [encounter_index & halo::k_slot_mask];
    squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];
    masks = &halo::ai::globals().squad_states[(int16_t)(enc->first_squad + squad_index)]
        .starting_location_mask;

    available = 0;
    taken[0] = 0;
    cursor = 0;
    chosen = -1;
    taken[1] = 0;

    if (0 < squad_definition->starting_locations.count) {
        location_index = 0;
        do {
            bit = 1 << (location_index & 0x1f);
            if ((masks[location_index >> 5] & bit) != 0 &&
                (taken[location_index >> 5] & bit) == 0) {
                available = available + 1;
            }
            cursor = cursor + 1;
            location_index = (int32_t)cursor;
        } while (location_index < squad_definition->starting_locations.count);

        if (0 < available) {
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            draw = (uint32_t)(((halo::math::globals().random_seed_global >> 0x10) * (int32_t)available) >> 0x10);
            cursor = 0;
            if (0 < squad_definition->starting_locations.count) {
                location_index = 0;
                do {
                    bit = 1 << (location_index & 0x1f);
                    if ((masks[location_index >> 5] & bit) != 0 &&
                        (taken[location_index >> 5] & bit) == 0) {
                        if ((int16_t)draw == 0) {
                            slot = masks + ((int32_t)cursor >> 5);
                            clear_mask = ~(1 << (cursor & 0x1f));
                            *slot = masks[(int32_t)cursor >> 5] & clear_mask;
                            slot[1] = slot[1] & clear_mask;
                            chosen = cursor;
                            if (cursor != -1) {
                                return chosen;
                            }
                            break;
                        }
                        draw = draw - 1;
                    }
                    cursor = cursor + 1;
                    location_index = (int32_t)cursor;
                } while (location_index < squad_definition->starting_locations.count);
            }
        }
    }

    for (;;) {
        cursor = chosen;
        location_count = squad_definition->starting_locations.count;
        location_index = 0;
        any_unavailable = 0;
        free_available = 0;
        if (location_count < 1) {
            return cursor;
        }
        {
            int32_t i = 0;
            uint32_t scan = 0;
            do {
                bit = 1 << (scan & 0x1f);
                if ((taken[(int32_t)scan >> 5] & bit) == 0) {
                    if ((masks[((int32_t)scan >> 5) + 1] & bit) == 0) {
                        any_unavailable = 1;
                    } else {
                        free_available = free_available + 1;
                    }
                }
                i = i + 1;
                scan = (uint32_t)(int16_t)i;
            } while ((int32_t)scan < location_count);
        }

        if (any_unavailable == 0 || free_available != 0) {
            if (0 < free_available) {
                halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                draw = (uint32_t)(((halo::math::globals().random_seed_global >> 0x10) * (int32_t)free_available) >> 0x10);
                {
                    uint32_t pick = 0;
                    location_index = 0;
                    if (0 < squad_definition->starting_locations.count) {
                        do {
                            bit = 1 << (location_index & 0x1f);
                            if ((masks[(location_index >> 5) + 1] & bit) != 0 &&
                                (taken[location_index >> 5] & bit) == 0) {
                                if ((int16_t)draw == 0) {
                                    masks[((int32_t)(int16_t)pick >> 5) + 1] =
                                        masks[((int32_t)(int16_t)pick >> 5) + 1] &
                                        ~(1 << (pick & 0x1f));
                                    return (int16_t)pick;
                                }
                                draw = draw - 1;
                            }
                            pick = pick + 1;
                            location_index = (int32_t)(int16_t)pick;
                        } while (location_index < squad_definition->starting_locations.count);
                    }
                }
            }
            return cursor;
        }

        fill = masks + 1;
        dword_count = (uint32_t)((location_count + 0x1f) >> 5);
        for (; dword_count != 0; dword_count = dword_count - 1) {
            *fill = halo::k_dword_none;
            fill = fill + 1;
        }
        chosen = cursor;
    }
}

/**
 * Behaviour of squad recent object get or create, moved unchanged from the original free function.
 *
 * @address 0x436c60
 */
datum_index EncounterView::recent_object_get_or_create(int16_t type, int32_t min_last_tick, char create_if_missing)
{
    datum_index encounter_index = handle;
    encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    datum_index cursor = enc->first_pursuit;
    uint8_t stale = 0;
    ai_pursuit *pursuit = 0;

    while (cursor != (datum_index)k_datum_index_none) {
        pursuit = &((ai_pursuit *)halo::ai::globals().pursuit_data->data)[cursor & halo::k_slot_mask];
        if (pursuit->type == type) {
            stale = pursuit->last_tick < min_last_tick;
            goto found;
        }
        cursor = pursuit->next;
    }

    if (create_if_missing != 0) {
        datum_index new_handle = halo::memory::datum_new(halo::ai::globals().pursuit_data);
        if (new_handle != (datum_index)k_datum_index_none) {
            ai_pursuit *new_pursuit = &((ai_pursuit *)halo::ai::globals().pursuit_data->data)[new_handle & halo::k_slot_mask];
            new_pursuit->type = type;
            new_pursuit->next = enc->first_pursuit;
            enc->first_pursuit = new_handle;
            cursor = new_handle;
            pursuit = new_pursuit;
            goto reset;
        }
    }

found:
    if (!stale) {
        return cursor;
    }

reset:
    pursuit->count = 0;
    pursuit->cursor = 0;
    pursuit->last_tick = -1;
    pursuit->object_index[0] = (datum_index)k_datum_index_none;
    pursuit->object_index[1] = (datum_index)k_datum_index_none;
    pursuit->object_index[2] = (datum_index)k_datum_index_none;
    pursuit->object_index[3] = (datum_index)k_datum_index_none;
    pursuit->object_index[4] = (datum_index)k_datum_index_none;
    pursuit->object_index[5] = (datum_index)k_datum_index_none;
    if (create_if_missing == 0) {
        return (datum_index)k_datum_index_none;
    }
    return cursor;
}

/**
 * Behaviour of squad recent object list clear, moved unchanged from the original free function.
 *
 * @address 0x436c10
 */
void EncounterView::recent_object_list_clear()
{
    datum_index encounter_index = handle;
    encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index & halo::k_slot_mask];
    datum_index cursor = enc->first_pursuit;

    while (cursor != (datum_index)k_datum_index_none) {
        ai_pursuit *pursuit = &((ai_pursuit *)halo::ai::globals().pursuit_data->data)[cursor & halo::k_slot_mask];
        enc->first_pursuit = pursuit->next;
        halo::memory::datum_delete(halo::ai::globals().pursuit_data, cursor);
        cursor = enc->first_pursuit;
    }
}

}
