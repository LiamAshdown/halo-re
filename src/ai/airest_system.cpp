#include "halo/ai/airest_system.hpp"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/ai_constants.hpp"

extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern ai_globals *ai_globals_ptr;
extern int32_t game_engine_get_current_tick(void);
extern void ai_broadcast_communication_event(int16_t gate, real_point3d *point, int32_t source_object, int16_t event_type, int16_t unused);
extern data_array *object_data;
extern data_array *prop_data;
extern data_array *encounter_data;
extern uint8_t *global_structure_bsp;
extern actor *actor_iterator_next(actor_iterator_state *iterator);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern uint32_t object_get_root_object_index(uint32_t object_index);
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point);
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index, void *target_ref, int16_t gate, real_point3d *listener_position);
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag);
extern void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index, int16_t grenade_type);
extern int ai_squad_priority_compare(const ai_priority_target_record *record_a, const ai_priority_target_record *record_b);
extern game_main_globals *main_game_globals;
extern void team_pair_override_add(int16_t index_a, uint8_t unknown_08, int16_t index_b, uint8_t unknown_09, int16_t threshold, int16_t timer_reset, uint8_t unknown_0c);
extern void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only);
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern void actors_initialize(void);
extern void encounters_initialize(void);
extern void ai_communication_initialize(void);
extern void actor_avoidance_build_direction_tables(void);
extern char prop_array_name[];
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index);
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);
extern void team_pair_override_clear_flag(int16_t index_b, int16_t index_a);
extern float k_random_scale_65536;
extern datum_index actor_place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index, int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index, const actor_placement_request *placement_request);
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index);
extern game_engine_definition *current_game_engine;
extern uint8_t *team_pair_data;
extern void actor_dispatch_perception_reset(datum_index actor_index);
extern player_globals *local_player_globals;
extern actor_mode_definition actor_mode_definitions[16];
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index);
extern datum_index actor_new_and_attach_to_unit(char reuse_existing, datum_index unit_index, datum_index actor_variant_tag, uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor, char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68);
extern void object_delete_unparented(uint32_t object_index);
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters);
extern void actor_movement_action_cancel(datum_index actor_index);
extern void actor_clear_target_state(datum_index actor_index);
extern void encounter_deactivate(datum_index encounter_index);
extern void encounters_reset(void);
extern void ai_communication_reset(void);
extern game_time_globals *game_time;
extern void ai_process_vehicle_entry_queue(void);
extern void ai_conversation_update(void);
extern void encounters_update(void);
extern void ai_release_actors_and_swarms(void);
extern void ai_reset_all_actors_perception(void);
extern void ai_actor_unlink_from_unassigned_list(datum_index actor_index);
extern void encounter_add_actor(int16_t squad_index, datum_index actor_index, datum_index encounter_index, uint8_t keep_team);
extern uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin, real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc, real_vector3d *out_direction, real *max_speed_override, real *out_speed, real *out_time_of_flight, real *out_range, real *out_half_gravity_term, real *out_horizontal_speed);
extern uint8_t projectile_solve_straight_line(real_point3d *target, real_point3d *origin, real speed, real *out_time_of_flight, real_vector3d *out_direction, real *out_speed_echo, real *out_length);
extern float k_physics_gravity;
extern double sqrt(double x);
}

namespace halo::ai {

/**
 * Behaviour of actors initialize, moved unchanged from the original free function.
 *
 * @address 0x426710
 */
void AiSystem::actors_initialize()
{
    actor_data = (data_array *)game_state_new((char *)"actor", k_actor_data_maximum_count, k_actor_size);
    swarm_data = (data_array *)game_state_new((char *)"swarm", k_swarm_data_maximum_count, k_swarm_size);
    swarm_component_data = (data_array *)game_state_new((char *)"swarm component", k_swarm_component_data_maximum_count, k_swarm_component_size);
}

/**
 * Reports the merged event through ai_broadcast_communication_event whenever an entry's average was just
 * (re)started rather than merely extended.
 *
 * @address 0x42c610
 */
void AiSystem::accumulate_repeated_event(int32_t event_type, real_point3d *position, int16_t event_id, int16_t window_ticks)
{
    int32_t current_tick;
    uint16_t cursor;
    uint16_t free_slot;
    ai_recent_event_record *found;
    ai_recent_event_record *records;
    uint8_t reset_average;
    uint8_t just_reset;

    if (!ai_globals_ptr->actors_valid || window_ticks <= 0) {
        return;
    }
    current_tick = game_engine_get_current_tick();
    records = (ai_recent_event_record *)&ai_globals_ptr->recent_events;

    found = 0;
    just_reset = 1;
    free_slot = halo::k_word_none;

    for (cursor = ai_globals_ptr->recent_event_head; cursor != ai_globals_ptr->recent_event_tail;
         cursor = (cursor + 1) & 0x1f) {
        uint8_t matches_id = 0;
        if (records[cursor].event_id == event_id &&
            halo::math::vector3d_distance_squared(*position, records[cursor].position) < 1.0f) {
            matches_id = 1;
        }
        if (current_tick - 0x78 < records[cursor].last_tick) {
            if (matches_id) {
                found = &records[cursor];
                found->count = found->count + 1;
                reset_average = (found->last_tick < current_tick - 0x1e);
                just_reset = reset_average;
                if (reset_average) {
                    found->position = *position;
                } else {
                    real weight = 1.0f / (real)found->count;
                    real inv_weight = 1.0f - weight;
                    found->position.x = weight * position->x + inv_weight * found->position.x;
                    found->position.y = weight * position->y + inv_weight * found->position.y;
                    found->position.z = weight * position->z + inv_weight * found->position.z;
                }
                break;
            }
        } else {
            records[cursor].event_id = -1;
            if (cursor == ai_globals_ptr->recent_event_head) {
                ai_globals_ptr->recent_event_head = (cursor + 1) & 0x1f;
            } else {
                free_slot = cursor;
            }
        }
    }

    if (found == 0) {
        uint16_t slot;
        if (free_slot == halo::k_word_none) {
            slot = ai_globals_ptr->recent_event_tail;
            ai_globals_ptr->recent_event_tail = (ai_globals_ptr->recent_event_tail + 1) & 0x1f;
            if (ai_globals_ptr->recent_event_tail == ai_globals_ptr->recent_event_head) {
                ai_globals_ptr->recent_event_head = (ai_globals_ptr->recent_event_head + 1) & 0x1f;
            }
        } else {
            slot = free_slot;
        }
        found = &records[slot];
        found->position = *position;
        found->last_tick = current_tick;
        found->event_id = event_id;
        found->count = 1;
    }

    if (just_reset) {
        ai_broadcast_communication_event(window_ticks, &found->position, event_type, found->event_id, found->count);
    }
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
/**
 * Behaviour of ai alert actors in grenade radius, moved unchanged from the original free function.
 *
 * @address 0x42a0e0
 */
void AiSystem::alert_actors_in_grenade_radius(datum_index source_unit_index, int16_t stimulus, int16_t gate)
{
    uint8_t *source = OBJECT_DATA(source_unit_index);
    uint8_t *location = source + 0x98;
    datum_index owner_actor = *(datum_index *)(source + 0x1f8);
    uint32_t cluster_bits[16];
    uint32_t firing_block[14];
    real_point3d source_position;
    actor_iterator_state iterator;
    int32_t cluster_count;
    int16_t source_cluster;
    actor *a;

    if (owner_actor == k_datum_index_none) {
        owner_actor = *(datum_index *)(source + 0x1f4);
    }
    if (((struct object *)source)->parent_object != k_datum_index_none) {
        location = OBJECT_DATA(object_get_root_object_index(source_unit_index)) + 0x98;
    }
    cluster_count = *(int32_t *)(global_structure_bsp + 0x134);
    memset(cluster_bits, 0, sizeof(cluster_bits));
    source_cluster = *(int16_t *)(location + 0x4);
    if (source_cluster != -1) {
        int16_t i;

        for (i = 0; (int32_t)i < cluster_count; i++) {
            uint8_t pas = 0;

            if (source_cluster != i) {
                int32_t lo = source_cluster < i ? source_cluster : i;
                int32_t hi = source_cluster < i ? i : source_cluster;
                int32_t row = (uint16_t)(*(uint16_t *)(global_structure_bsp + 0x134) - 1) * lo - ((lo + 1) * lo) / 2;

                pas = (*(uint8_t **)(global_structure_bsp + 0x220))[(int16_t)(row + hi - 1)];
            }
            if (!(pas & 0x80) && (float)(int32_t)(pas & 0x7f) * 2.015748f < 40.0f) {
                cluster_bits[i >> 5] |= 1u << (i & 0x1f);
            }
        }
    }
    object_get_position(&source_position, source_unit_index);
    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = k_datum_index_none;
        iterator.next_actor_index = -1;
    }
    for (a = actor_iterator_next(&iterator); a != 0; a = actor_iterator_next(&iterator)) {
        datum_index actor_index = iterator.actor_index;
        int16_t actor_cluster = *(int16_t *)((uint8_t *)a + 0x148);
        datum_index prop_index;
        uint8_t *p;

        if (actor_index == owner_actor || actor_cluster == -1 ||
            !(cluster_bits[actor_cluster >> 5] & (1u << (actor_cluster & 0x1f)))) {
            continue;
        }
        actor_get_firing_positions(actor_index, firing_block, &source_position);
        if ((int16_t)actor_target_hearing_check(location, 0, actor_index, firing_block, gate, &source_position) < 2) {
            continue;
        }
        prop_index = actor_find_or_create_shared_prop(source_unit_index, actor_index, 1, 1);
        if (prop_index == k_datum_index_none) {
            continue;
        }
        p = PROP(prop_index);
        if ((int16_t)actor_target_hearing_check(p + 0xfc, (int16_t)*(uint16_t *)(p + 0x38), actor_index, firing_block,
                gate, &((struct prop *)p)->last_known_position) < 2) {
            continue;
        }
        actor_squad_react_to_grenade(actor_index, prop_index, stimulus);
    }
}

#undef OBJECT_DATA
#undef PROP

/**
 * Behaviour of ai build priority target list, moved unchanged from the original free function.
 *
 * @address 0x42acd0
 */
void AiSystem::build_priority_target_list(ai_priority_target_list *out_list)
{
    datum_index actor_index;

    out_list->count = 0;
    out_list->unknown_02 = 0;

    actor_index = ai_globals_ptr->actors_valid ? ai_globals_ptr->first_encounterless_actor : (datum_index)k_datum_index_none;
    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
        datum_index next = a->next_in_encounter;

        if (out_list->count > 0xff) {
            break;
        }
        if (a->active == 0 && a->deactivation_time != (datum_index)k_datum_index_none) {
            ai_priority_target_record *rec = &out_list->records[out_list->count];
            rec->tiebreak = 1;
            rec->handle = actor_index;
            rec->priority = (int32_t)a->deactivation_time;
            out_list->count = out_list->count + 1;
        }
        actor_index = next;
    }

    {
        datum_index encounter_handle = (datum_index)k_datum_index_none;
        uint8_t scan_more = 0;
        data_iterator iterator;

        if (ai_globals_ptr->actors_valid != 0) {
            encounter_handle = (datum_index)k_datum_index_none;
            scan_more = 0;
        }

        iterator.data = encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        while (ai_globals_ptr->actors_valid != 0) {
            encounter *enc;

            do {
                enc = (encounter *)halo::memory::data_iterator_next(&iterator);
                if (enc == 0 || scan_more == 0) {
                    break;
                }
            } while (enc->units_active == 0);

            if (enc == 0 || out_list->count > 0xff) {
                break;
            }
            if (enc->units_active == 0 && enc->living_count > 0 && enc->activation_tick != (datum_index)k_datum_index_none) {
                ai_priority_target_record *rec = &out_list->records[out_list->count];
                rec->tiebreak = 0;
                rec->handle = iterator.index;
                rec->priority = (int32_t)enc->activation_tick;
                out_list->count = out_list->count + 1;
            }
        }
    }

    if (out_list->count > 0) {
        qsort(out_list->records, (size_t)out_list->count, sizeof(ai_priority_target_record),
              (int (*)(const void *, const void *))ai_squad_priority_compare);
    }
}

/**
 * Behaviour of ai category matches wildcard, moved unchanged from the original free function.
 *
 * @address 0x433ba0
 */
void AiSystem::category_matches_wildcard(int16_t category, int16_t other_category)
{
    static const int16_t k_forgiveness_ticks[4] = {300, 450, 1200, 2700};
    int16_t team_a = category;
    int16_t team_b = other_category;
    int16_t other = -1;
    int16_t threshold = -1;
    int16_t timer = -1;
    uint8_t betrayable = 0;
    uint8_t human = 0;
    uint8_t b_is_other = 0;
    uint8_t a_is_other = 0;

    if (team_a == -1 || team_b == -1) {
        return;
    }
    if (team_a == 1) {
        other = team_b;
    } else if (team_b == 1) {
        other = team_a;
    }
    if (other == 2 || other == 5) {
        timer = k_forgiveness_ticks[main_game_globals->difficulty & 3];
        human = other == 2;
        betrayable = 1;
        threshold = 5;
        b_is_other = team_b == other;
    }
    if (betrayable && team_a == other) {
        a_is_other = 1;
    }
    team_pair_override_add(team_a, a_is_other, team_b, b_is_other, threshold, timer, human);
}

/**
 * Behaviour of ai count actors in mode9 group, moved unchanged from the original free function.
 *
 * @address 0x433e20
 */
int16_t AiSystem::count_actors_in_mode9_group(int32_t group_id)
{
    int16_t count = 0;
    actor_iterator_state iterator;
    actor *a;

    actor_iterator_new(&iterator, 0);
    a = actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->mode == _actor_mode_vocalize && *(int32_t *)a->mode_data.raw == group_id) {
            count = count + 1;
        }
        a = actor_iterator_next(&iterator);
    }
    return count;
}

/**
 * Behaviour of ai get difficulty request, moved unchanged from the original free function.
 *
 * @address 0x42a950
 */
void AiSystem::get_difficulty_request(int16_t request_code, uint8_t *out_flag_a, uint8_t *out_flag_b, float *out_value)
{
    switch (request_code) {
    case 1:
        *out_flag_a = 1;
        *out_value = weapon_get_zoom_fov(0x1d, main_game_globals->difficulty);
        break;
    case 2:
        *out_flag_a = 1;
        *out_value = weapon_get_zoom_fov(0x1e, main_game_globals->difficulty);
        break;
    case 3:
        *out_flag_a = 0;
        *out_flag_b = 0;
        break;
    case 4:
        *out_flag_a = 0;
        *out_flag_b = 1;
        break;
    default:
        *out_flag_a = 1;
        *out_value = weapon_get_zoom_fov(0x1c, main_game_globals->difficulty);
        break;
    }
}

/**
 * Finds or allocates a small fixed-capacity aggregation-bucket slot keyed by an id, used by the
 * nearby-actor scanning helpers to group results per actor type. Returns the slot index, or -1 if the key
 * was not found and the bucket array is already full.
 *
 * @address 0x420de0
 */
int16_t AiSystem::group_bucket_find_or_add(ai_group_bucket_entry *buckets, int32_t key, int16_t *count, int16_t capacity)
{
    int16_t live_count;
    int16_t i;
    ai_group_bucket_entry *entry;

    live_count = *count;

    for (i = 0; i < live_count; i++) {
        if (buckets[i].key == key) {
            return i;
        }
    }

    if (live_count < capacity) {
        entry = &buckets[live_count];
        *count = live_count + 1;
        entry->prop_index = -1;
        entry->key = -1;
        entry->nearest_friend_actor_index = -1;
        entry->priority = 0;
        entry->prop = 0;
        entry->retreating_friend_count = 0;
        entry->nearest_friend_distance_squared = 3.4028235e+38f;
        return live_count;
    }

    return -1;
}

/**
 * Behaviour of ai initialize for new map, moved unchanged from the original free function.
 *
 * @address 0x42a7c0
 */
void AiSystem::initialize_for_new_map()
{
    ai_globals *globals = (ai_globals *)(game_state_base + game_state_cursor);
    int32_t size = k_ai_globals_size;

    game_state_cursor = game_state_cursor + k_ai_globals_size;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    ai_globals_ptr = globals;
    memset(globals, 0, k_ai_globals_size);

    actors_initialize();
    prop_data = (data_array *)game_state_new(prop_array_name, k_prop_data_maximum_count, k_prop_size);
    encounters_initialize();
    ai_communication_initialize();
    actor_avoidance_build_direction_tables();
}

/**
 * Matches the phase-4 summary exactly. Not simplified away, since it is not clear whether this is
 * intentional or a genuine quirk of the original code.
 *
 * @address 0x4383f0
 */
uint8_t AiSystem::insert_scored_candidate_pair(ai_scored_candidate *list, datum_index handle, float score, datum_index payload, datum_index key)
{
    uint8_t inserted = 0;
    int16_t i;

    for (i = 0; i < 2; i++) {
        if (list[i].score < score) {
            if (i < 1) {
                list[1] = list[0];
            }
            list[i].handle = handle;
            list[i].score = score;
            list[i].payload = payload;
            list[i].key = key;
            inserted = 1;
        }
    }

    return inserted;
}

/**
 * Finishes with a pair-relationship cleanup call.
 *
 * @address 0x42ba80
 */
void AiSystem::mark_recognized_objects_for_reaction(int16_t team_a, int16_t team_b, uint8_t status)
{
    actor_iterator_state iterator;
    actor *a;
    datum_index actor_index;
    int16_t other_team;
    datum_index prop_cursor;
    datum_index current_prop_index;
    prop *p;

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.next_actor_index = -1;
    }

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        other_team = team_b;
        if (a->team != team_a) {
            other_team = team_a;
        }
        if ((a->team == team_a || a->team == team_b) && other_team != (int16_t)-1) {
            actor_index = iterator.actor_index /* the full handle, salt included */;

            prop_cursor = a->first_prop;
            while (prop_cursor != (datum_index)k_datum_index_none) {
                current_prop_index = prop_cursor;
                p = &((prop *)prop_data->data)[current_prop_index & halo::k_slot_mask];
                prop_cursor = p->next_in_actor;
                if (p->team == other_team) {
                    p->allegiance = 1;
                    p->team_pair_status = 0;
                    p->enemy = status;
                    p->engaged = actor_target_update_active_flag(actor_index, current_prop_index);
                    p->desirability = actor_rate_potential_target(actor_index, current_prop_index);
                }
            }
        }
        a = actor_iterator_next(&iterator);
    }
    team_pair_override_clear_flag(team_b, team_a);
}

/**
 * Behaviour of ai notify actors of encounter state change, moved unchanged from the original free
 * function.
 *
 * @address 0x42b940
 */
void AiSystem::notify_actors_of_encounter_state_change(int16_t zone_a, int16_t zone_b, uint8_t status, uint8_t force_update)
{
    actor_iterator_state iterator;
    actor *a;
    datum_index actor_index;
    int16_t other_zone;
    datum_index prop_cursor;
    datum_index current_prop_index;
    prop *p;

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.next_actor_index = -1;
        actor_index = (datum_index)k_datum_index_none;
    }

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        other_zone = zone_b;
        if (a->team != zone_a) {
            other_zone = zone_a;
        }
        if ((a->team == zone_a || a->team == zone_b) && other_zone != (int16_t)-1) {
            actor_index = (datum_index)(((uint8_t *)a - (uint8_t *)actor_data->data) / sizeof(actor));

            prop_cursor = a->first_prop;
            while (prop_cursor != (datum_index)k_datum_index_none) {
                current_prop_index = prop_cursor;
                p = &((prop *)prop_data->data)[current_prop_index & halo::k_slot_mask];
                prop_cursor = p->next_in_actor;
                if (p->team == other_zone) {
                    if (force_update == 0) {
                        p->allegiance = 1;
                        p->team_pair_status = 1;
                    }
                    if (status == 0 || force_update != 0) {
                        p->enemy = status;
                        p->engaged = actor_target_update_active_flag(actor_index, current_prop_index);
                        p->desirability = actor_rate_potential_target(actor_index, current_prop_index);
                    }
                }
            }
        }
        a = actor_iterator_next(&iterator);
    }
}

/**
 * Returns the bucket index, or -1 when no bucket has a positive weight with a live handle.
 *
 * @address 0x438480
 */
int16_t AiSystem::pick_weighted_candidate(ai_scored_candidate *table, ai_scored_candidate *out_entry)
{
    float total_weight;
    float running;
    int16_t last_valid;
    int16_t valid_count;
    int16_t i;
    int16_t chosen;
    ai_scored_candidate *entry;

    total_weight = 0.0f;
    last_valid = -1;
    valid_count = 0;

    for (i = 0; i < 4; i = i + 1) {
        entry = &table[i * 2];
        if (0.0f < entry->score && entry->handle != (datum_index)k_datum_index_none) {
            total_weight = total_weight + entry->score;
            valid_count = valid_count + 1;
            last_valid = i;
        }
    }

    chosen = last_valid;
    if (1 < valid_count) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        running = 0.0f;
        for (i = 0; i < 4; i = i + 1) {
            entry = &table[i * 2];
            chosen = last_valid;
            if (0.0f < entry->score && entry->handle != (datum_index)k_datum_index_none) {
                running = running + entry->score;
                chosen = i;
                if ((float)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536 * total_weight <
                    running) {
                    break;
                }
            }
        }
    }

    if (chosen != -1) {
        entry = &table[chosen * 2];
        out_entry->handle = entry->handle;
        out_entry->score = entry->score;
        out_entry->payload = entry->payload;
        out_entry->key = entry->key;
    }
    return chosen;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
/**
 * Behaviour of ai process vehicle entry queue, moved unchanged from the original free function.
 *
 * @address 0x42bf90
 */
void AiSystem::process_vehicle_entry_queue()
{
    int16_t queue_index;

    for (queue_index = 0; queue_index < ai_globals_ptr->vehicle_entry_count; queue_index++) {
        datum_index vehicle_index = ai_globals_ptr->vehicle_entry_queue[queue_index];
        uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)OBJECT_DATA(vehicle_index) & halo::k_slot_mask].data;
        int16_t seat_index;

        for (seat_index = 0; seat_index < *(int32_t *)(vehicle_tag + 0x2e4); seat_index++) {
            datum_index gunner_tag = *(datum_index *)(*(uint8_t **)(vehicle_tag + 0x2e8) + seat_index * 0x11c + 0x104);
            actor_placement_request request;
            uint8_t *vehicle;
            datum_index actor_index;

            if (gunner_tag == k_datum_index_none) {
                continue;
            }
            memset(&request, 0, 0x1c);
            *(int16_t *)((uint8_t *)&request + 0x1a) = -1;
            vehicle = OBJECT_DATA(vehicle_index);
            if (((vehicle_object *)vehicle)->base.parent_object == k_datum_index_none) {
                request.position = *(real_point3d *)&((vehicle_object *)vehicle)->base.position.x;
            } else {
                uint8_t *parent = OBJECT_DATA(((vehicle_object *)vehicle)->base.parent_object);

                halo::math::matrix4x3_transform_point(request.position, *(real_point3d *)(vehicle + 0x5c),
                    *(real_matrix4x3 *)(parent + ((struct object *)parent)->nodes.offset + (int8_t)vehicle[0x120] * 0x34));
            }
            actor_index = actor_place_new_unit(gunner_tag, k_datum_index_none, -1, 0, 0, &request);
            if (actor_index != k_datum_index_none) {
                unit_enter_vehicle_seat(vehicle_index, seat_index,
                    *(datum_index *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size + 0x18));
            }
        }
    }
    ai_globals_ptr->vehicle_entry_count = 0;
}

#undef OBJECT_DATA

/**
 * Behaviour of ai recompute all relationship flags, moved unchanged from the original free function.
 *
 * @address 0x42bbb0
 */
void AiSystem::recompute_all_relationship_flags()
{
    actor_iterator_state iterator;
    actor *a;
    datum_index actor_index;
    datum_index prop_cursor;
    datum_index current_prop_index;
    prop *p;
    object *tracked_object;
    int16_t object_team;
    int16_t actor_team;
    uint8_t hostile;
    uint8_t marked;

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ halo::ai::k_iterator_signature_key;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.next_actor_index = -1;
    }

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        actor_index = iterator.actor_index /* the full handle, salt included */;
        prop_cursor = a->first_prop;
        while (prop_cursor != (datum_index)k_datum_index_none) {
            current_prop_index = prop_cursor;
            p = &((prop *)prop_data->data)[current_prop_index & halo::k_slot_mask];
            prop_cursor = p->next_in_actor;

            tracked_object = ((object_header *)object_data->data)[p->object_index & halo::k_slot_mask].data;
            object_team = ((struct object *)tracked_object)->owner_team;
            p->team = object_team;
            actor_team = a->team;

            hostile = 1;
            if (current_game_engine == 0) {
                if (-1 < actor_team && actor_team < 10 && -1 < object_team && object_team < 10) {
                    int32_t pair = (int32_t)object_team + actor_team * 10;
                    uint32_t bit = *(uint32_t *)(team_pair_data + 0xa4 + (pair >> 5) * 4);
                    hostile = 1 - ((bit & (1u << (pair & 0x1f))) != 0);
                }
            } else {
                hostile = (actor_team != object_team);
            }
            p->enemy = hostile;

            marked = 0;
            if (-1 < actor_team && actor_team < 10 && -1 < object_team && object_team < 10) {
                int32_t pair = (int32_t)object_team + actor_team * 10;
                uint32_t bit = *(uint32_t *)(team_pair_data + 0x94 + (pair >> 5) * 4);
                marked = (bit & (1u << (pair & 0x1f))) != 0;
            }
            p->allegiance = marked;

            p->engaged = actor_target_update_active_flag(actor_index, current_prop_index);
            p->desirability = actor_rate_potential_target(actor_index, current_prop_index);
        }
        a = actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reset all actors perception, moved unchanged from the original free function.
 *
 * @address 0x429080
 */
void AiSystem::reset_all_actors_perception()
{
    actor_iterator_state iterator;
    actor *a;

    iterator.filter_array = encounter_data;
    iterator.next_index = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ halo::ai::k_iterator_signature_key;
    iterator.encounterless_done = 0;
    iterator.active = 1;
    iterator.actor_index = -1;
    iterator.next_actor_index = -1;

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        datum_index actor_index = iterator.actor_index /* the full handle, salt included */;
        actor_dispatch_perception_reset(actor_index);
        a = actor_iterator_next(&iterator);
    }
}

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
#define OBJ(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
#define ai_globals_ptr (*reinterpret_cast<uint8_t * *>(&ai_globals_ptr))
namespace {

static uint8_t ai_bsp_actor_should_carry(uint8_t *actor)
{
    if (((struct actor *)actor)->target_unit_index != k_datum_index_none && ((struct actor *)actor)->target_combat_status >= 5) {
        uint8_t *target = PROP(((struct actor *)actor)->target_unit_index);
        int32_t fired = ((struct actor *)actor)->ticks_since_threatened;

        if (*(int16_t *)(target + 0x24) >= 4 && *(int16_t *)(target + 0x24) <= 5) {
            target = PROP(((struct prop *)target)->pair_index);
        }
        return target[0x12e] != 0 && fired != -1 && fired < 0x5a && ((struct prop *)target)->distance < 10.0f;
    }
    {
        int16_t team = ((struct actor *)actor)->team;
        uint8_t enemies;
        uint8_t carry = 0;
        datum_index prop_index;

        if (current_game_engine != 0) {
            enemies = team != 1;
        } else {
            int32_t index;

            if (team < 0 || team >= 10) {
                return 0;
            }
            index = team * 10 + 1;
            enemies = (*(uint32_t *)(team_pair_data + 0xa4 + (index >> 5) * 4) & (1u << (index & 0x1f))) == 0;
        }
        if (enemies) {
            return 0;
        }
        for (prop_index = ((struct actor *)actor)->first_prop; prop_index != k_datum_index_none;) {
            uint8_t *p = PROP(prop_index);

            prop_index = *(datum_index *)(p + 8);
            if (p[0x12e] != 0 && (((struct prop *)p)->visual_perception >= 2 || ((struct prop *)p)->distance < 3.0f)) {
                carry = 1;
            }
        }
        return carry;
    }
}

static uint8_t ai_bsp_split_swarm(datum_index actor_index, uint8_t *actor)
{
    uint8_t *swarm;
    int16_t count;
    int16_t hidden = 0;
    datum_index hidden_units[16];
    int16_t i;

    if (((struct actor *)actor)->swarm_index == k_datum_index_none) {
        return 0;
    }
    swarm = (uint8_t *)swarm_data->data + (((struct actor *)actor)->swarm_index & halo::k_slot_mask) * k_swarm_size;
    count = *(int16_t *)(swarm + 2);
    for (i = 0; i < count; i++) {
        datum_index unit_index = *(datum_index *)(swarm + 0x18 + i * 4);
        datum_index root = unit_index;
        int16_t cluster;

        while (*(datum_index *)(OBJ(root) + 0x11c) != k_datum_index_none) {
            root = *(datum_index *)(OBJ(root) + 0x11c);
        }
        cluster = *(int16_t *)(OBJ(root) + 0x9c);
        if (cluster == -1 ||
            (*(uint32_t *)&local_player_globals->cluster_pvs[(cluster >> 5)] & (1u << (cluster & 0x1f))) == 0) {
            hidden_units[hidden++] = unit_index;
        }
    }
    if (hidden == 0) {
        return 1;
    }
    if (hidden == count) {
        return 0;
    }
    for (i = 0; i < hidden; i++) {
        datum_index unit_index = hidden_units[i];

        actor = ACTOR(actor_index);
        actor_remove_from_unit_cluster(actor_index, unit_index);
        actor = ACTOR(actor_index);
        if (actor_new_and_attach_to_unit(1, unit_index, ((struct actor *)actor)->actor_variant_tag, *(uint32_t *)&((struct actor *)actor)->encounter_index,
                ((struct actor *)actor)->squad_index, 0, actor_index, 0, 2, 0, halo::k_word_none, 0) == k_datum_index_none) {
            int32_t kind = *(int32_t *)(OBJ(unit_index) + 4);

            if (kind == 0) {
                object_delete_unparented(unit_index);
                object_delete_recursive(unit_index, 0);
            } else if (kind == 3) {
                object_delete_recursive(unit_index, 0);
            }
        }
    }
    return 1;
}

}

/**
 * Behaviour of ai reset fire group assignments, moved unchanged from the original free function.
 *
 * @address 0x42c940
 */
void AiSystem::reset_fire_group_assignments()
{
    int32_t encounter_count = *(int32_t *)&halo::scenario::globals().scenario->encounters.count;
    int16_t e;
    datum_index actor_index;

    for (e = 0; e < encounter_count; e++) {
        uint8_t *encounter = (uint8_t *)encounter_data->data + (e & halo::k_slot_mask) * k_encounter_size;
        datum_index next;

        if (encounter[0xd] == 0 || ((struct encounter *)encounter)->living_count <= 0) {
            continue;
        }
        next = ((struct encounter *)encounter)->first_actor;
        while (ai_globals_ptr[1] != 0 && next != k_datum_index_none) {
            uint8_t *actor;
            uint8_t carry;

            actor_index = next;
            actor = ACTOR(actor_index);
            next = ((struct actor *)actor)->next_in_encounter;
            carry = ai_bsp_actor_should_carry(actor);
            if (!carry) {
                continue;
            }
            if (actor[6] != 0 && !ai_bsp_split_swarm(actor_index, actor)) {
                continue;
            }
            actor = ACTOR(actor_index);
            *(int32_t *)&((struct actor *)actor)->original_encounter_index = e;
            ((struct actor *)actor)->original_squad_index = ((struct actor *)actor)->squad_index;
            ((struct actor *)actor)->firing_position_index = -1;
            if (((struct actor *)actor)->active_movement.type == 3 || ((struct actor *)actor)->active_movement.type == 4) {
                ((struct actor *)actor)->active_movement.type = 0;
                *(datum_index *)&((struct actor *)actor)->active_movement.extra = k_datum_index_none;
            }
            {
                void (*carry_proc)(datum_index) =
                    *(void (**)(datum_index))((uint8_t *)&actor_mode_definitions[((struct actor *)actor)->mode] + 0x24);

                if (carry_proc != 0) {
                    carry_proc(actor_index);
                }
            }
            encounter_remove_actor(actor_index, 0);
            if (ai_globals_ptr[1] == 0) {
                break;
            }
            actor = ACTOR(actor_index);
            ((struct actor *)actor)->next_in_encounter = *(datum_index *)(ai_globals_ptr + 8);
            *(datum_index *)(ai_globals_ptr + 8) = actor_index;
            actor[9] = 1;
            *(int16_t *)(actor + 0x10) = actor[8] != 0 ? 0x5a : 0;
            actor_movement_action_cancel(actor_index);
        }
        encounter = (uint8_t *)encounter_data->data + (e & halo::k_slot_mask) * k_encounter_size;
        ((struct encounter *)encounter)->activation_delay = 0;
        encounter_deactivate((datum_index)(int32_t)e);
    }

    for (actor_index = *(datum_index *)(ai_globals_ptr + 8); actor_index != k_datum_index_none;) {
        datum_index following = ((struct actor *)ACTOR(actor_index))->next_in_encounter;
        datum_index prop_index;

        actor_clear_target_state(actor_index);
        for (prop_index = ((struct actor *)ACTOR(actor_index))->first_prop; prop_index != k_datum_index_none;) {
            uint8_t *p = PROP(prop_index);

            prop_index = *(datum_index *)(p + 8);
            *(int16_t *)(p + 0x100) = -1;
            *(int32_t *)(p + 0xfc) = -1;
            *(int32_t *)(p + 0xec) = -1;
        }
        actor_index = following;
    }
}

#undef ACTOR
#undef PROP
#undef OBJ
#undef ai_globals_ptr

/**
 * Behaviour of ai reset for new map, moved unchanged from the original free function.
 *
 * @address 0x42a840
 */
void AiSystem::reset_for_new_map()
{
    ai_globals *g = ai_globals_ptr;

    memset(g, 0, k_ai_globals_size);

    g->ai_active = 1;
    g->ai_was_active = 1;
    g->first_encounterless_actor = (datum_index)k_datum_index_none;
    g->grenades_enabled = 1;
    g->dialogue_triggers_enabled = 1;
    for (int32_t tier = 0; tier < 3; tier++) {
        g->loudest_line_tick[tier][0] = -1;
        g->loudest_line_tick[tier][1] = -1;
    }

    actor_data->valid = 1;
    halo::memory::data_delete_all(actor_data);
    swarm_data->valid = 1;
    halo::memory::data_delete_all(swarm_data);
    swarm_component_data->valid = 1;
    halo::memory::data_delete_all(swarm_component_data);
    prop_data->valid = 1;
    halo::memory::data_delete_all(prop_data);

    encounters_reset();
    ai_communication_reset();

    g->recent_event_tail = 0;
    g->recent_event_head = 0;
    memset(g->recent_events, 0, sizeof(g->recent_events));

    g->actors_valid = 1;
}

/**
 * Beyond that, a prop of an unusual kind seen within the last 90 ticks, or close (kind outside 4..5,
 * within 4 units), or -- when the actor has no target -- of kind 2/3, or of kind 4/5 that is either
 * already noticed or (kind 4, within 12 units) wins a random roll, counts as activity found.
 *
 * @address 0x42c3e0
 */
int32_t AiSystem::scan_for_recent_combat_activity(uint8_t hard_difficulty)
{
    data_iterator iterator;
    prop *p;
    int32_t current_tick;
    object *tracked_object;
    actor *a;
    datum_index linked_unit_index;
    object *linked_object;
    Unit *linked_unit_tag;
    uint8_t skip_close_check;
    int16_t kind;

    current_tick = game_time->game_time;

    iterator.data = prop_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (prop *)halo::memory::data_iterator_next(&iterator);

    while (p != 0) {
        if (p->is_parented && p->enemy) {
            tracked_object = ((object_header *)object_data->data)[p->object_index & halo::k_slot_mask].data;
            if (((unit_data *)((uint8_t *)tracked_object + k_unit_data_offset))->controlling_player !=
                (datum_index)k_datum_index_none) {
                a = &((actor *)actor_data->data)[p->actor_index & halo::k_slot_mask];
                linked_unit_index = a->swarm ? a->cluster_unit_index : a->unit_index;

                linked_object = ((object_header *)object_data->data)[linked_unit_index & halo::k_slot_mask].data;
                linked_unit_tag = (Unit *)halo::cache::globals().tag_instances[linked_object->definition_tag & halo::k_slot_mask].data;

                skip_close_check = 0;
                if ((linked_unit_tag->unit_flags & 0x80000) != 0) { // "inconsequential"
                    if (4.0f < p->distance) {
                        skip_close_check = 1;
                    }
                }

                if (hard_difficulty != 0 && a->firing_state == 0 && a->mode != _actor_mode_vehicle) {
                    if (15.0f < p->distance) {
                        goto next_prop;
                    }
                }

                if (!skip_close_check) {
                    kind = p->state;
                    if ((kind < 4 || 5 < kind) && p->last_seen_time != -1 &&
                        current_tick <= p->last_seen_time + 0x5a) {
                        return 1;
                    }
                    if ((kind < 4 || 5 < kind) && p->distance < 4.0f) {
                        return 1;
                    }
                    if (a->target_unit_index == (datum_index)k_datum_index_none) {
                        if (1 < kind && kind < 4) {
                            return 1;
                        }
                        if (3 < kind && kind < 6) {
                            if (p->has_current_information != 0) {
                                return 1;
                            }
                            if (kind == 4 && p->distance < 12.0f) {
                                prop *pair = &((prop *)prop_data->data)[p->pair_index & halo::k_slot_mask];
                                if (halo::math::vector3d_distance_squared(p->last_known_position, pair->last_known_position) < 16.0f) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
next_prop:
        p = (prop *)halo::memory::data_iterator_next(&iterator);
    }
    return 0;
}

/**
 * TYPES-GAP: no header defines the local {..., distance} sort record built by
 * actor_target_scan_potential_targets; only the +8 float this function reads is named here.
 *
 * @address 0x41d7a0
 */
int AiSystem::target_distance_qsort_compare(void *record_a, void *record_b)
{
    if (*(float *)((uint8_t *)record_a + 8) < *(float *)((uint8_t *)record_b + 8)) {
        return -1;
    }
    if (*(float *)((uint8_t *)record_b + 8) < *(float *)((uint8_t *)record_a + 8)) {
        return 1;
    }
    return 0;
}

/**
 * Behaviour of ai tick dispatcher, moved unchanged from the original free function.
 *
 * @address 0x42a900
 */
void AiSystem::tick_dispatcher()
{
    if (ai_globals_ptr->actors_valid != 0) {
        ai_process_vehicle_entry_queue();
        if (ai_globals_ptr->ai_active != 0) {
            ai_conversation_update();
            encounters_update();
            ai_release_actors_and_swarms();
            ai_globals_ptr->ai_was_active = 1;
            return;
        }
        if (ai_globals_ptr->ai_was_active != 0) {
            ai_reset_all_actors_perception();
            ai_globals_ptr->ai_was_active = 0;
        }
    }
}

/**
 * Behaviour of ai unassigned actors attach to structure bsp, moved unchanged from the original free
 * function.
 *
 * @address 0x42ce90
 */
void AiSystem::unassigned_actors_attach_to_structure_bsp()
{
    int16_t bsp_index = halo::scenario::globals().structure_bsp_index;
    datum_index actor_index = ai_globals_ptr->first_encounterless_actor;

    while (actor_index != k_datum_index_none) {
        actor *entry = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
        datum_index encounter_index = entry->original_encounter_index;
        datum_index next = entry->next_in_encounter;

        if (encounter_index != k_datum_index_none &&
            *(int16_t *)((uint8_t *)global_scenario->encounters.pointer + (encounter_index & halo::k_slot_mask) * 0xb0 + 0x7e) ==
                bsp_index) {
            ai_actor_unlink_from_unassigned_list(actor_index);
            encounter_add_actor(entry->original_squad_index, actor_index, entry->original_encounter_index, 1);
        }
        actor_index = next;
    }
}

/**
 * Returns -1 if count is not positive or every weight is excluded / non-positive.
 *
 * @address 0x432100
 */
int32_t AiSystem::weighted_random_index(int16_t weight_offset, void *base, int16_t stride, uint16_t count, uint32_t *exclude_mask)
{
    float total;
    uint8_t *cursor;
    int32_t i;

    if ((int16_t)count < 1) {
        return -1;
    }

    total = 0.0f;
    cursor = (uint8_t *)base + weight_offset;
    for (i = 0; i < (uint16_t)count; i++) {
        if ((exclude_mask[i >> 5] & (1u << (i & 0x1f))) == 0) {
            total += *(float *)cursor;
        }
        cursor += stride;
    }

    if (total > 0.0f) {
        float roll;
        float running = 0.0f;
        int16_t chosen = 0;

        cursor = (uint8_t *)base + weight_offset;
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        roll = (float)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f * total;

        while ((exclude_mask[chosen >> 5] & (1u << (chosen & 0x1f))) != 0 ||
               (running = running + *(float *)cursor, (roll < running) == (roll == running))) {
            chosen = chosen + 1;
            cursor += stride;
            if ((int16_t)count <= chosen) {
                return -1;
            }
        }
        return (int32_t)chosen;
    }
    return -1;
}

namespace {

class StraightLineSolver final : public AimSolver {
public:
    uint8_t solve(const AimRequest &r) const override
    {
        return projectile_solve_straight_line(r.target, r.origin, r.speed, r.out_time, r.out_direction,
            r.out_speed, r.out_range);
    }

    uint8_t is_straight_line() const override { return 1; }
};

class BallisticArcSolver final : public AimSolver {
public:
    uint8_t solve(const AimRequest &r) const override
    {
        return projectile_solve_ballistic_arc(r.target, r.origin, r.speed, r.gravity_scale, r.max_time,
            r.use_high_arc, r.out_direction, r.max_speed_override, r.out_speed, r.out_time, r.out_range,
            (real *)0, (real *)0);
    }

    uint8_t is_straight_line() const override { return 0; }
};

constexpr StraightLineSolver k_straight_line_solver;
constexpr BallisticArcSolver k_ballistic_arc_solver;

}

/**
 * Chooses between the gravity-arc and straight-line aiming solvers for a shot from *origin to *target
 * using tag's Projectile definition, and reports which one it used through *out_used_straight_line (1 for
 * the straight-line solver, 0 for the gravity-arc solver; either out pointer may be NULL).
 *
 * @address 0x4beec0
 */
uint8_t ProjectileAim::get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag, real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override, uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed, real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line)
{
    real speed;
    uint8_t solved;

    if (speed_in == (real *)0) {
        speed = tag->initial_velocity;
    } else {
        speed = *speed_in;
    }

    AimRequest request = { target, origin, speed, tag->air_gravity_scale, max_time, use_high_arc, out_direction,
        max_speed_override, out_speed, out_time_or_fraction, out_range_or_length };
    const AimSolver *solver;
    if (((tag->projectile_flags & _projectile_definition_ai_must_use_ballistic_aiming_bit) == 0) ||
        (tag->air_gravity_scale <= 0.0f)) {
        solver = &k_straight_line_solver;
    } else {
        solver = &k_ballistic_arc_solver;
    }
    solved = solver->solve(request);
    if (out_used_straight_line != (uint8_t *)0) {
        *out_used_straight_line = solver->is_straight_line();
    }
    return solved;
}

/**
 * Solves a gravity-arc firing solution from *origin to *target: gravity is k_physics_gravity *
 * gravity_scale (clamped to >= 0), the launch speed is capped by speed_limit (or by *max_speed_override
 * when non-NULL, which also skips the max_time-derived tightening of that cap), and use_high_arc selects
 * the lofted root of the two solutions to the vertical-velocity quadratic when a real root exists. The
 * resulting unit direction is written to *out_direction (falling back to the straight-line direction to
 * the target, and then to the global up vector, if the solved direction is degenerate).
 *
 * @address 0x4beb30
 */
uint8_t ProjectileAim::solve_ballistic_arc(real_point3d *target, real_point3d *origin, real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc, real_vector3d *out_direction, real *max_speed_override, real *out_speed, real *out_time_of_flight, real *out_range, real *out_half_gravity_term, real *out_horizontal_speed)
{
    real dx, dy, dz;
    real dxy2, qg, twoqg, distance_sq, disc, shallow_time, dzg, chosen_max, a, disc2, root2, t;
    double g, d2, disc_ext, neg_root, shallow_speed_ext, dzg_ext, s_ext;
    double inv_t, vertical_velocity_ext;
    real_vector3d dir;
    real length;
    real horizontal_speed, vertical_velocity;
    uint8_t used_root = 1;

    dx = target->x - origin->x;
    dy = target->y - origin->y;
    dz = target->z - origin->z;
    dxy2 = (real)((double)dy * dy + (double)dx * dx);

    g = (double)halo::physics::globals().gravity * (double)gravity_scale;
    if (g < 0.0) {
        g = 0.0;
    }
    qg = (real)(g * g * 0.25);
    d2 = (double)dz * dz + (double)dxy2;
    distance_sq = (real)d2;
    disc_ext = d2 * (double)qg * 4.0;
    disc = (real)disc_ext;
    neg_root = -sqrt(disc_ext);
    twoqg = qg + qg;
    shallow_time = (real)sqrt((-1.0 / (double)twoqg) * neg_root);
    dzg_ext = g * (double)dz;
    dzg = (real)dzg_ext;
    s_ext = dzg_ext - neg_root;
    shallow_speed_ext = (s_ext < 0.0) ? 0.0 : sqrt(s_ext);

    if (max_speed_override != (real *)0) {
        chosen_max = *max_speed_override;
    } else {
        chosen_max = speed_limit;
        if ((max_time != (real *)0) && (0.0f < *max_time)) {
            double scaled = (double)shallow_time * (double)*max_time;
            double scaled_sq = scaled * scaled;
            double sum = scaled_sq * (double)qg + (double)distance_sq / scaled_sq;
            double candidate = sqrt((double)dzg + sum);

            if ((double)speed_limit > candidate) {
                chosen_max = (real)candidate;
            }
        }
    }

    if (!((double)chosen_max < shallow_speed_ext)) {
        double a_ext = (double)dzg - (double)chosen_max * (double)chosen_max;
        double disc2_ext;

        a = (real)a_ext;
        disc2_ext = a_ext * (double)a - (double)disc;
        disc2 = (real)disc2_ext;
        if ((a < 0.0f) && (0.0f <= disc2)) {
            double root2_ext = (sqrt((double)disc2) * (double)(int)((use_high_arc != 0) * 2 - 1) - (double)a) / (double)twoqg;

            root2 = (real)root2_ext;
            if (root2_ext > 0.0) {
                t = (real)sqrt((double)root2);
                goto have_root;
            }
        }
    }
    used_root = 0;
    t = shallow_time;
    chosen_max = (real)shallow_speed_ext;

have_root:
    inv_t = 1.0 / (double)t;
    dir.i = (real)((double)dx * inv_t);
    dir.j = (real)((double)dy * inv_t);
    vertical_velocity_ext = inv_t * (double)dz + (double)t * g * 0.5;
    dir.k = (real)vertical_velocity_ext;
    vertical_velocity = (real)vertical_velocity_ext;
    horizontal_speed = (real)sqrt((double)dir.j * dir.j + (double)dir.i * dir.i);

    length = halo::math::vector3d_normalize_with_length(dir);
    if (length == 0.0f) {
        used_root = 0;
        dir.i = dx;
        dir.j = dy;
        dir.k = dz;
        length = halo::math::vector3d_normalize_with_length(dir);
        if (length == 0.0f) {
            dir = *halo::math::globals().global_up3d_pointer;
            used_root = 0;
        }
    }

    out_direction->i = dir.i;
    out_direction->j = dir.j;
    out_direction->k = dir.k;

    if (out_range != (real *)0) {
        *out_range = (real)((double)t * (double)chosen_max);
    }
    if (out_speed != (real *)0) {
        *out_speed = chosen_max;
    }
    if (out_half_gravity_term != (real *)0) {
        *out_half_gravity_term = vertical_velocity;
    }
    if (out_horizontal_speed != (real *)0) {
        *out_horizontal_speed = horizontal_speed;
    }
    if (out_time_of_flight != (real *)0) {
        *out_time_of_flight = t;
    }
    return used_root;
}

/**
 * Computes the straight-line direction (unnormalized, target - origin) from *origin to *target, its
 * length, and the flight time at the given speed (0 when speed is not positive). Writes the direction to
 * *out_direction unconditionally; out_speed_echo, out_length and out_time_of_flight may be NULL.
 *
 * @address 0x4bee20
 */
uint8_t ProjectileAim::solve_straight_line(real_point3d *target, real_point3d *origin, real speed, real *out_time_of_flight, real_vector3d *out_direction, real *out_speed_echo, real *out_length)
{
    real_vector3d scratch;
    real length, time_fraction;

    scratch.i = target->x - origin->x;
    scratch.j = target->y - origin->y;
    scratch.k = target->z - origin->z;
    length = halo::math::vector3d_normalize_with_length(scratch);

    if (!(speed > 0.0f)) {
        time_fraction = 0.0f;
    } else {
        time_fraction = length / speed;
    }

    *out_direction = scratch;

    if (out_length != (real *)0) {
        *out_length = length;
    }
    if (out_speed_echo != (real *)0) {
        *out_speed_echo = speed;
    }
    if (out_time_of_flight != (real *)0) {
        *out_time_of_flight = time_fraction;
    }
    return 1;
}

}
