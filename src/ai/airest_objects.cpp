#include "halo/ai/airest_objects.hpp"
#include "halo/math/api.hpp"

#include <stdint.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

extern "C" {
extern data_array *actor_data;
extern ai_globals *ai_globals_ptr;
extern void actor_movement_action_cancel(datum_index actor_index);
extern data_array *object_data;
extern data_array *prop_data;
extern void actor_release_from_cluster_or_delete(datum_index actor_index, datum_index unit_index);
extern void actor_delete(datum_index actor_index, uint32_t flag);
extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference);
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove);
extern void ai_conversation_clear_object_references(datum_index object_index, uint8_t force_full_scan);
extern data_array *object_list_header_data;
extern data_array *object_list_reference_data;
extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters);
extern void encounters_recompute_dirty(void);
extern void object_initialize_shield_stun_thresholds(uint32_t object_index, float *override_max_body_vitality, float *override_max_shield_vitality);
extern game_time_globals *game_time;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern uint32_t ai_actor_type_get_morale_grade(int16_t actor_type_index, uint8_t *command_reference);
extern datum_index object_list_get_first(datum_index header_index, object_list_iterator *iterator_out);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void ai_unit_remap_actor_to_squad(datum_index unit_index, uint32_t packed_reference, char notify);
extern void ai_recompute_all_relationship_flags(void);
extern void actor_clear_perceived_props(datum_index actor_index);
extern void actor_dispatch_perception_reset(datum_index actor_index);
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index);
extern void ai_reference_spawn_starting_location_object(datum_index unit_index, uint32_t packed_reference);
extern uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name, uint8_t interpolate);
extern void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator);
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator);
extern int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector, int16_t *out_indices, int16_t max_indices);
extern uint8_t actor_play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index, char *seat_name, int16_t seat_flags, int16_t count);
extern data_array *ai_pursuit_data;
extern datum_index squad_recent_object_get_or_create(datum_index encounter_index, int16_t type, int32_t min_last_tick, char create_if_missing);
extern void ai_alert_actors_in_grenade_radius(datum_index source_unit_index, int16_t stimulus, int16_t gate);
extern uint8_t *actor_type_procs[];
extern datum_index actor_new(datum_index actor_variant_tag);
extern void actor_attach_to_unit(datum_index actor_index, datum_index unit_index);
extern void ai_actor_link_to_unassigned_list(datum_index actor_index);
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant, void *context);
extern int32_t actor_squad_action_status_broadcast(uint32_t actor_index, int16_t command_list_index, int16_t *record);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
extern int32_t ai_squad_find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index, uint8_t *requested_actor_data, uint8_t *requested_actor_variant_data, char match_by_index);
extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index, int16_t squad_index);
extern void actor_notify_squad_and_flag_danger(datum_index actor_index, uint8_t alternate_event, uint8_t raise_danger_flag);
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor);
extern void actor_update_swarm_component_position(datum_index component_index, datum_index unit_index);
}

namespace halo::ai {

/**
 * Behaviour of ai actor get activity stage, moved unchanged from the original free function.
 *
 * @address 0x435680
 */
int32_t AiActorView::get_activity_stage()
{
    datum_index actor_index = handle;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];

    if (a->active == 0) {
        return 0;
    }
    if (a->awareness_level < 3) {
        return 1;
    }
    if (a->combat_status == 0) {
        return 2;
    }
    if (a->target_combat_status < 6) {
        return 3;
    }
    if (a->target_combat_status < 10) {
        return 4;
    }
    if (*((uint8_t *)a + 0x454) != 0 || *((uint8_t *)a + 0x45c) != 0) {
        return 6;
    }
    return 5;
}

/**
 * Behaviour of ai actor link to unassigned list, moved unchanged from the original free function.
 *
 * @address 0x436940
 */
void AiActorView::link_to_unassigned_list()
{
    datum_index actor_index = handle;
    if (ai_globals_ptr->actors_valid != 0) {
        actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];

        a->next_in_encounter = ai_globals_ptr->first_encounterless_actor;
        ai_globals_ptr->first_encounterless_actor = actor_index;
        a->encounterless = 1;
        *(int16_t *)a->activation_delay = (a->active != 0) ? 0x5a : 0;

        actor_movement_action_cancel(actor_index);
    }
}

/**
 * Behaviour of ai actor type get morale grade, moved unchanged from the original free function.
 *
 * @address 0x434ed0
 */
uint32_t AiObjects::type_get_morale_grade(int16_t actor_type_index, uint8_t *command_reference)
{
    ScenarioCommandList *command_list =
        &((ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer)[actor_type_index];

    if ((uint32_t)command_reference[0] < (uint32_t)command_list->commands.count &&
        (ScenarioCommand *)command_list->commands.pointer + command_reference[0] != 0) {
        return ((uint8_t)(~command_reference[4]) & 0x10 | 0x20) >> 4;
    }
    return 1;
}

/**
 * Behaviour of ai actor unlink from unassigned list, moved unchanged from the original free function.
 *
 * @address 0x436990
 */
void AiActorView::unlink_from_unassigned_list()
{
    datum_index actor_index = handle;
    if (ai_globals_ptr->actors_valid != 0) {
        actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
        datum_index *link = &ai_globals_ptr->first_encounterless_actor;

        while (*link != actor_index) {
            actor *node = &((actor *)actor_data->data)[*link & halo::k_slot_mask];
            link = &node->next_in_encounter;
        }

        *link = a->next_in_encounter;
        a->encounterless = 0;
        a->next_in_encounter = (datum_index)k_datum_index_none;
        a->force_active = 0;
    }
}

/**
 * Behaviour of ai clear object references, moved unchanged from the original free function.
 *
 * @address 0x42c140
 */
void AiObjects::clear_object_references(datum_index object_index)
{
    object *obj;
    prop *p;
    data_iterator iterator;
    unit_data *unit;
    int16_t queue_count;
    int16_t i;

    if (!ai_globals_ptr->actors_valid) {
        return;
    }
    obj = ((object_header *)object_data->data)[object_index & halo::k_slot_mask].data;
    if (((1 << (obj->type & 0x1f)) & 3) == 0) {
        return;
    }

    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor_delete(unit->actor_index, 0);
    } else if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        actor_release_from_cluster_or_delete(unit->swarm_actor_index, object_index);
    }

    iterator.data = prop_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (prop *)halo::memory::data_iterator_next(&iterator);
    while (p != 0) {
        if (p->object_index == object_index) {
            actor_replace_object_reference(p->actor_index, halo::k_dword_none, iterator.index);
            actor_unlink_prop(p->actor_index, iterator.index);
            halo::memory::datum_delete(prop_data, iterator.index);
        } else if (p->relationship_object_index == (int32_t)object_index) {
            p->relationship_object_index = -1;
            p->is_vehicle_driver = 0;
            p->is_vehicle_gunner = 0;
        }
        p = (prop *)halo::memory::data_iterator_next(&iterator);
    }

    ai_conversation_clear_object_references(object_index, 1);

    queue_count = ai_globals_ptr->vehicle_entry_count;
    for (i = 0; i < queue_count; i++) {
        if (ai_globals_ptr->vehicle_entry_queue[i] == object_index) {
            queue_count = queue_count - 1;
            ai_globals_ptr->vehicle_entry_count = queue_count;
            if (0 < queue_count) {
                ai_globals_ptr->vehicle_entry_queue[i] = ai_globals_ptr->vehicle_entry_queue[queue_count];
            }
        }
    }
}

/**
 * Returns NULL for a null handle, or when the table is full.
 *
 * @address 0x435900
 */
ai_object_attention_record * AiObjects::object_attention_find_or_create(datum_index object_index)
{
    ai_object_attention_record *table;
    ai_object_attention_record *record;
    int16_t index;
    int32_t i;

    table = (ai_object_attention_record *)ai_globals_ptr->object_attention_table;
    record = 0;

    if (object_index == (datum_index)k_datum_index_none) {
        return 0;
    }

    index = 0;
    if (0 < ai_globals_ptr->object_attention_count) {
        index = 0;
        do {
            if (table[index].object_index == object_index) {
                break;
            }
            index = index + 1;
        } while (index < ai_globals_ptr->object_attention_count);
        if (0x1f < index) {
            return 0;
        }
    }

    record = &table[index];
    if (ai_globals_ptr->object_attention_count <= index) {
        for (i = 0; i < 10; i = i + 1) {
            ((int32_t *)record)[i] = 0;
        }
        record->object_index = object_index;
        record->weight = 8.0f;
        ai_globals_ptr->object_attention_count = ai_globals_ptr->object_attention_count + 1;
    }
    return record;
}

/**
 * Behaviour of ai object attention remove, moved unchanged from the original free function.
 *
 * @address 0x435990
 */
void AiObjects::object_attention_remove(datum_index object_index)
{
    ai_object_attention_record *table;
    int16_t index;
    int16_t last;
    int32_t i;
    int32_t *src;
    int32_t *dst;

    table = (ai_object_attention_record *)ai_globals_ptr->object_attention_table;

    if (object_index == (datum_index)k_datum_index_none) {
        return;
    }
    if (ai_globals_ptr->object_attention_count <= 0) {
        return;
    }

    index = 0;
    while (table[index].object_index != object_index) {
        index = index + 1;
        if (ai_globals_ptr->object_attention_count <= index) {
            return;
        }
    }

    last = ai_globals_ptr->object_attention_count - 1;
    ai_globals_ptr->object_attention_count = last;
    if (index < last) {
        src = (int32_t *)&table[last];
        dst = (int32_t *)&table[index];
        for (i = 10; i != 0; i = i - 1) {
            *dst = *src;
            src = src + 1;
            dst = dst + 1;
        }
    }
}

/**
 * Behaviour of ai object list clear orders with weapon, moved unchanged from the original free function.
 *
 * @address 0x432ad0
 */
void ObjectListView::clear_orders_with_weapon()
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data + (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        } else {
            object_index = (datum_index)k_datum_index_none;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *header = &((object_header *)object_data->data)[object_index & halo::k_slot_mask];
        unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

        if (unit->actor_index != (datum_index)k_datum_index_none) {
            actor_delete(unit->actor_index, 0);
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list detach actors from encounters, moved unchanged from the original free
 * function.
 *
 * @address 0x435260
 */
void ObjectListView::detach_actors_from_encounters()
{
    datum_index object_list_header_handle = handle;
    object_list_header *header;
    object_list_reference *node;
    datum_index node_index;
    datum_index object_index;
    object_header *entry;
    object *obj;
    unit_data *unit;
    actor *a;
    datum_index actor_index;
    int16_t detached;

    object_index = (datum_index)k_datum_index_none;
    node_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    detached = 0;
    if (object_index == (datum_index)k_datum_index_none) {
        return;
    }

    do {
        entry = 0;
        if (object_index != (datum_index)k_datum_index_none &&
            0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)
                [(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 ||
                 candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            obj = entry->data;
            unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if (unit->actor_index != (datum_index)k_datum_index_none &&
                ((actor *)actor_data->data)[unit->actor_index & halo::k_slot_mask].encounter_index !=
                    (datum_index)k_datum_index_none) {

                actor_index = unit->actor_index;
                actor_movement_action_cancel(actor_index);
                encounter_remove_actor(actor_index, 0);

                if (ai_globals_ptr->actors_valid != 0) {
                    a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
                    a->next_in_encounter = ai_globals_ptr->first_encounterless_actor;
                    ai_globals_ptr->first_encounterless_actor = actor_index;
                    a->encounterless = 1;
                    *(uint16_t *)&a->activation_delay[0] =
                        (uint16_t)(-(uint16_t)(a->active != 0) & 0x5a);
                    actor_movement_action_cancel(actor_index);
                }
                detached = detached + 1;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    } while (object_index != (datum_index)k_datum_index_none);

    if (0 < detached) {
        encounters_recompute_dirty();
    }
}

/**
 * Behaviour of ai object list initialize shield stun thresholds, moved unchanged from the original free
 * function.
 *
 * @address 0x561ab0
 */
void ObjectListView::initialize_shield_stun_thresholds(float override_max_body_vitality, float override_max_shield_vitality)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *entry = &((object_header *)object_data->data)[(int16_t)object_index & halo::k_slot_mask];
        if ((entry->data->vitality_flags & _object_health_frozen_bit) == 0) {
            object_initialize_shield_stun_thresholds((uint32_t)object_index,
                &override_max_body_vitality, &override_max_shield_vitality);
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list max flee grade, moved unchanged from the original free function.
 *
 * @address 0x434f20
 */
int16_t ObjectListView::max_flee_grade()
{
    datum_index object_list_header_handle = handle;
    int32_t tick;
    object_list_header *header;
    object_list_reference *node;
    object_header *entry;
    object *obj;
    unit_data *unit;
    actor *a;
    swarm *sw;
    ScenarioCommandList *command_list;
    datum_index node_index;
    datum_index object_index;
    uint32_t grade;
    uint32_t best;
    int16_t component_count;
    int16_t component_index;
    int32_t command_index;

    tick = game_time->game_time;
    object_index = (datum_index)k_datum_index_none;
    node_index = (datum_index)k_datum_index_none;
    best = 0;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }
    if (object_index == (datum_index)k_datum_index_none) {
        return 0;
    }

    do {
        entry = 0;
        if (object_index != (datum_index)k_datum_index_none &&
            0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)
                [(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 ||
                 candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            obj = entry->data;
            unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            grade = 0;

            if (unit->actor_index == (datum_index)k_datum_index_none) {
                if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
                    a = &((actor *)actor_data->data)[unit->swarm_actor_index & halo::k_slot_mask];
                    if (a->mode == _actor_mode_flee &&
                        a->swarm_index != (datum_index)k_datum_index_none) {
                        sw = &((swarm *)swarm_data->data)[a->swarm_index & halo::k_slot_mask];
                        component_count = sw->component_count;
                        component_index = 0;
                        if (0 < component_count) {
                            do {
                                if (sw->unit_index[component_index] == object_index) {
                                    break;
                                }
                                component_index = component_index + 1;
                            } while (component_index < component_count);
                        }
                        if (component_index < component_count &&
                            (((swarm_component *)swarm_component_data->data)
                                 [sw->component_index[component_index] & halo::k_slot_mask].flags & 8) != 0) {
                            grade = (uint32_t)(uint16_t)ai_actor_type_get_morale_grade(*(int16_t *)&((struct actor *)a)->mode_data,
                                (uint8_t *)&((swarm_component *)swarm_component_data->data)
                                    [sw->component_index[component_index] & halo::k_slot_mask] + 0x1c);
                            goto have_grade;
                        }
                    }
                    goto recently_hurt;
                }
            } else {
                a = &((actor *)actor_data->data)[unit->actor_index & halo::k_slot_mask];
                if (a->mode == _actor_mode_flee) {
                    command_list = &((ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer)
                        [*(int16_t *)(a->mode_data.raw + 0x00)];
                    command_index = (int32_t)(uint32_t)a->mode_data.raw[8];
                    if (command_index < command_list->commands.count &&
                        (uint8_t *)command_list->commands.pointer + command_index * 0x20 != 0) {
                        grade = (uint32_t)((((uint8_t)~a->mode_data.raw[0x0c] & 0x10) | 0x20) >> 4);
                    } else {
                        grade = 1;
                    }
have_grade:
                    if ((int16_t)grade != 0) {
                        goto keep_best;
                    }
                }
recently_hurt:
                if (a->command_list_finished_time != -1 && tick <= a->command_list_finished_time + 0x96) {
                    grade = 1;
                }
            }
keep_best:
            if ((int16_t)best <= (int16_t)grade) {
                best = grade;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    } while (object_index != (datum_index)k_datum_index_none);

    return (int16_t)best;
}

/**
 * Behaviour of ai object list remap units and children, moved unchanged from the original free function.
 *
 * @address 0x433a70
 */
void ObjectListView::remap_units_and_children(uint32_t packed_reference, char notify)
{
    datum_index object_list_header = handle;
    if (object_list_header != (datum_index)k_datum_index_none && packed_reference != (uint32_t)k_datum_index_none) {
        object_list_iterator iterator;
        datum_index object_index = object_list_get_first(object_list_header, &iterator);

        while (object_index != (datum_index)k_datum_index_none) {
            object *obj = object_try_and_get(object_index, 3);

            if (obj != 0) {
                datum_index child;
                ai_unit_remap_actor_to_squad(object_index, packed_reference, 0);

                child = obj->first_child_object;
                while (child != (datum_index)k_datum_index_none) {
                    object_header *child_header = &((object_header *)object_data->data)[child & halo::k_slot_mask];
                    if ((1 << (child_header->type & 0x1f) & 3) != 0) {
                        ai_unit_remap_actor_to_squad(child, packed_reference, 0);
                    }
                    child = ((object *)child_header->data)->next_object;
                }
            }

            if (iterator == (datum_index)k_datum_index_none) {
                object_index = (datum_index)k_datum_index_none;
            } else {
                object_list_reference *node =
                    (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                               (iterator & halo::k_slot_mask) * 0x0c);
                iterator = node->next;
                object_index = node->object_index;
            }
        }

        ai_recompute_all_relationship_flags();
        encounters_recompute_dirty();
    }
}

namespace {

static void reset_or_wake(datum_index unit_index, char flag)
{
    object_header *header = &((object_header *)object_data->data)[unit_index & halo::k_slot_mask];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
    datum_index actor_index = unit->actor_index;

    if (actor_index == (datum_index)k_datum_index_none) {
        actor_index = unit->swarm_actor_index;
    }
    if (actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
        if (flag == 0) {
            if (a->awareness_level == 0) {
                a->awareness_level = 2;
            }
        } else {
            a->awareness_level = 0;
            a->mode = 0;
            actor_clear_perceived_props(actor_index);
            actor_dispatch_perception_reset(actor_index); // FIXED: argument from the binary call site (the draft passed none) (EAX = ESI, 0x434697)
            actor_set_units_active(actor_index, 0); // BL = 0 at both call sites (0x43469e, 0x43473f)
        }
    }
}

}

/**
 * Behaviour of ai object list reset or wake awareness, moved unchanged from the original free function.
 *
 * @address 0x434590
 */
void ObjectListView::reset_or_wake_awareness(char flag)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object *obj = object_try_and_get(object_index, 3);
        if (obj != 0) {
            datum_index child = obj->first_child_object;
            reset_or_wake(object_index, flag);
            while (child != (datum_index)k_datum_index_none) {
                object_header *child_header = &((object_header *)object_data->data)[child & halo::k_slot_mask];
                if ((1 << (child_header->type & 0x1f) & 3) != 0) {
                    reset_or_wake(child, flag);
                }
                child = ((object *)child_header->data)->next_object;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list respawn members, moved unchanged from the original free function.
 *
 * @address 0x432e80
 */
void ObjectListView::respawn_members(uint32_t packed_reference)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data + (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        } else {
            object_index = (datum_index)k_datum_index_none;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        ai_reference_respawn_member(packed_reference, object_index);

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list set unit flag 400, moved unchanged from the original free function.
 *
 * @address 0x4347b0
 */
void ObjectListView::set_unit_flag_400(char flag)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object *obj = object_try_and_get(object_index, 3);
        if (obj != 0) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if (flag == 0) {
                unit->flags &= ~0x400u;
            } else {
                unit->flags |= 0x400u;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list set unit flag 800, moved unchanged from the original free function.
 *
 * @address 0x4348c0
 */
void ObjectListView::set_unit_flag_800(char flag)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object *obj = object_try_and_get(object_index, 3);
        if (obj != 0) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if (flag == 0) {
                unit->flags &= ~0x800u;
            } else {
                unit->flags |= 0x800u;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list set unit flag 800000, moved unchanged from the original free function.
 *
 * @address 0x561d50
 */
void ObjectListView::set_unit_flag_800000(char flag)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *entry = 0;
        if (0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)[(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 || candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            unit_data *unit = (unit_data *)((uint8_t *)entry->data + k_unit_data_offset);
            if (flag == 0) {
                unit->flags &= ~(uint32_t)_unit_flag_unknown_800000;
            } else {
                unit->flags |= (uint32_t)_unit_flag_unknown_800000;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list spawn members, moved unchanged from the original free function.
 *
 * @address 0x432a40
 */
void ObjectListView::spawn_members(uint32_t packed_reference)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data + (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        } else {
            object_index = (datum_index)k_datum_index_none;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        ai_reference_spawn_starting_location_object(object_index, packed_reference);

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object list start user animation until failure, moved unchanged from the original free
 * function.
 *
 * @address 0x561e60
 */
uint8_t ObjectListView::start_user_animation_until_failure(datum_index graph_tag_id, const char *animation_name, uint8_t interpolate)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;
    uint8_t still_succeeding = 1;


    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *entry = 0;
        if (0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)[(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 || candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            if (still_succeeding && unit_start_user_animation((uint32_t)object_index, graph_tag_id, animation_name,
                    interpolate) != 0) {
                still_succeeding = 1;
            } else {
                still_succeeding = 0;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
    return still_succeeding;
}

/**
 * Behaviour of ai object list update vitality fractions, moved unchanged from the original free function.
 *
 * @address 0x561cb0
 */
void ObjectListView::update_vitality_fractions(float body_delta, float shield_delta)
{
    datum_index object_list_header_handle = handle;
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & halo::k_slot_mask) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        unit_update_vitality_fractions((uint32_t)object_index, body_delta, shield_delta);

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & halo::k_slot_mask) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

/**
 * Behaviour of ai object process nearby actors, moved unchanged from the original free function.
 *
 * @address 0x433cc0
 */
void AiObjects::object_process_nearby_actors(uint32_t ai_reference, datum_index vehicle_index, char *seat_name, char allow_boarding_actors)
{
    object *obj;

    if (ai_reference == halo::k_dword_none) {
        return;
    }
    obj = object_try_and_get(vehicle_index, 3);
    if (obj != 0) {
        real_point3d reference_position;
        int16_t seat_list[16];
        int16_t seat_count;
        int16_t candidate_count = 0;
        ai_nearby_actor_candidate candidates[0x40];

        object_get_position(&reference_position, vehicle_index);
        seat_count = unit_find_seats_matching_name_and_flags(vehicle_index, seat_name, halo::k_word_none, seat_list, 0x10);

        if (seat_count > 0) {
            ai_reference_actor_iterator iterator;
            actor *a;

            ai_reference_actor_iterator_new(ai_reference, &iterator);
            a = ai_reference_actor_iterator_next(&iterator);
            while (a != 0) {
                if (candidate_count < 0x40) {
                    float dx = reference_position.x - a->body_position.x;
                    float dy = reference_position.y - a->body_position.y;
                    float dz = reference_position.z - a->body_position.z;
                    ai_nearby_actor_candidate *c = &candidates[candidate_count];
                    c->actor_index = iterator.actor_index;
                    c->distance_squared = dz * dz + dx * dx + dy * dy;
                    c->is_type_9 = (a->mode == 9);
                    candidate_count = candidate_count + 1;
                }
                a = ai_reference_actor_iterator_next(&iterator);
            }

            qsort(candidates, candidate_count, sizeof(ai_nearby_actor_candidate), halo::math::object_sort_by_flag_then_distance);

            {
                int16_t i;
                for (i = 0; i < candidate_count; i++) {
                    if (candidates[i].is_type_9 != 0 && allow_boarding_actors == 0) {
                        return;
                    }
                    actor_play_first_valid_vocalization(seat_list, vehicle_index, candidates[i].actor_index, 0, -1,
                                                        seat_count);
                }
            }
        }
    }
}

/**
 * Behaviour of ai pursuit check object, moved unchanged from the original free function.
 *
 * @address 0x436b90
 */
uint8_t AiObjects::pursuit_check_object(datum_index object_index, datum_index encounter_index, int16_t type, int32_t min_last_tick, char create_if_missing, int16_t *out_count, uint32_t *out_last_tick)
{
    uint8_t found = 0;
    int16_t count = 0;
    uint32_t last_tick = (uint32_t)k_datum_index_none;
    datum_index handle = squad_recent_object_get_or_create(encounter_index, type, min_last_tick, create_if_missing);

    if (handle != (datum_index)k_datum_index_none) {
        ai_pursuit *pursuit = &((ai_pursuit *)ai_pursuit_data->data)[handle & halo::k_slot_mask];
        count = pursuit->count;
        last_tick = *(uint32_t *)&pursuit->last_tick;

        if (count < 7) {
            int16_t i;
            for (i = 0; i < k_ai_pursuit_object_count; i++) {
                if (pursuit->object_index[i] == object_index) {
                    found = 1;
                    break;
                }
            }
        } else {
            found = 1;
        }
    }

    if (out_count != 0) {
        *out_count = count;
    }
    if (out_last_tick != 0) {
        *out_last_tick = last_tick;
    }
    return found;
}

/**
 * Behaviour of ai pursuit note object, moved unchanged from the original free function.
 *
 * @address 0x436b10
 */
uint8_t AiObjects::pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type, int32_t min_last_tick)
{
    uint8_t added = 0;
    datum_index handle = squad_recent_object_get_or_create(encounter_index, type, min_last_tick, 1);

    if (handle != (datum_index)k_datum_index_none) {
        ai_pursuit *pursuit = &((ai_pursuit *)ai_pursuit_data->data)[handle & halo::k_slot_mask];
        int16_t i;

        for (i = 0; i < k_ai_pursuit_object_count; i++) {
            if (pursuit->object_index[i] == object_index) {
                goto stamp;
            }
        }

        pursuit->object_index[pursuit->cursor] = object_index;
        pursuit->count = pursuit->count + 1;
        pursuit->cursor = (int16_t)((pursuit->cursor + 1) % k_ai_pursuit_object_count);
        added = 1;

    stamp:
        pursuit->last_tick = game_time->game_time;
    }

    return added;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
/**
 * Behaviour of ai refresh unit stimulus and alert, moved unchanged from the original free function.
 *
 * @address 0x42c2a0
 */
void AiObjects::refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority, int16_t stimulus_value)
{
    uint8_t *obj;
    int32_t now;

    if (!ai_globals_ptr->actors_valid || object_index == k_datum_index_none || priority <= 0) {
        return;
    }
    obj = OBJECT_DATA(object_index);
    now = game_time->game_time;
    if (!(stimulus_value > *(int16_t *)(obj + 0x21c)) && !(now > *(int32_t *)(obj + 0x220) + 0x1e)) {
        return;
    }
    *(int32_t *)(obj + 0x220) = now;
    *(int16_t *)(obj + 0x21c) = stimulus_value;
    if (((object *)obj)->type == 1) {
        datum_index child;

        for (child = ((object *)obj)->first_child_object; child != k_datum_index_none;) {
            uint8_t *c = OBJECT_DATA(child);

            if (((struct object *)c)->type == 0) {
                ai_alert_actors_in_grenade_radius(child, stimulus_value, priority);
            }
            child = ((struct object *)c)->next_object;
        }
    } else if (((object *)obj)->type == 0) {
        ai_alert_actors_in_grenade_radius(object_index, stimulus_value, priority);
    }
}

#undef OBJECT_DATA

/**
 * Behaviour of ai unit clear actor vocalization, moved unchanged from the original free function.
 *
 * @address 0x435a50
 */
void AiUnitView::clear_actor_vocalization()
{
    datum_index unit_index = handle;
    object_header *header = &((object_header *)object_data->data)[unit_index & halo::k_slot_mask];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[unit->actor_index & halo::k_slot_mask];
        ((actor *)a)->vocalization_variant = 0;
        ((actor *)a)->vocalization_line = 0;
        ((actor *)a)->vocalization_state = 0;
    }
}

/**
 * Behaviour of ai unit create actor, moved unchanged from the original free function.
 *
 * @address 0x435420
 */
void AiObjects::create_actor(datum_index actor_variant_tag, datum_index unit_index)
{
    datum_index actor_definition_tag;
    object *unit_definition;
    datum_index actor_index;
    actor *a;

    if (ai_globals_ptr->actors_valid == 0 ||
        unit_index == (datum_index)k_datum_index_none ||
        actor_variant_tag == (datum_index)k_datum_index_none) {
        return;
    }

    actor_definition_tag = *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances
        [actor_variant_tag & halo::k_slot_mask].data + 0x10);
    if (actor_definition_tag == (datum_index)k_datum_index_none) {
        return;
    }
    if ((**(uint32_t **)&halo::cache::globals().tag_instances[actor_definition_tag & halo::k_slot_mask].data & 0x4000000) != 0) {
        return; // Actor.flags bit 26, "swarm"
    }

    unit_definition = object_try_and_get(unit_index, 1);
    if (unit_definition == 0) {
        return;
    }
    if ((*((uint8_t *)unit_definition + 0x106) & 4) != 0) {
        return;
    }

    actor_index = actor_new(actor_variant_tag);
    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }

    a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    ai_actor_link_to_unassigned_list(actor_index);
    a->awareness_level = 2;
    a->pending_order_request = 2;
    a->standing_order_request = 2;
    a->command_list_run_immediately = 0;
    a->command_list_delay = 2;
    a->pending_command_list = -1;
    a->sequence_id = 0;

    if (*((uint8_t *)a + 0x6) != actor_type_procs[((actor *)a)->type][0xd]) {
        actor_delete(actor_index, 0);
        return;
    }
    actor_attach_to_unit(actor_index, unit_index);
}

/**
 * Behaviour of ai unit dispatch actor event d, moved unchanged from the original free function.
 *
 * @address 0x435a00
 */
void AiUnitView::dispatch_actor_event_d(int32_t unused)
{
    datum_index unit_index = handle;
    object_header *header = &((object_header *)object_data->data)[unit_index & halo::k_slot_mask];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

    if (unused != -1 && unit->actor_index != (datum_index)k_datum_index_none) {
        int16_t payload[8] = {0};
        payload[0] = 6;
        *(int32_t *)&payload[2] = unused;
        actor_begin_vocalization(unit->actor_index, 0xd, 1, payload);
    }
}

/**
 * Behaviour of ai unit flee if ready, moved unchanged from the original free function.
 *
 * @address 0x434df0
 */
void AiUnitView::flee_if_ready(uint32_t readiness_param)
{
    datum_index unit_index = handle;
    object *unit_object = object_try_and_get(unit_index, 3);

    if (unit_object != 0) {
        unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
        uint8_t mode_data[0x84];

        if (unit->actor_index != (datum_index)k_datum_index_none &&
            (uint8_t)actor_squad_action_status_broadcast(unit->actor_index, (int16_t)readiness_param,
                (int16_t *)mode_data) != 0) {
            actor_set_mode(unit->actor_index, _actor_mode_flee, mode_data);
        }
    }
}

/**
 * Behaviour of ai unit remap actor to squad, moved unchanged from the original free function.
 *
 * @address 0x433970
 */
void AiUnitView::remap_actor_to_squad(uint32_t packed_reference, char notify)
{
    datum_index unit_index = handle;
    object_header *header = &((object_header *)object_data->data)[unit_index & halo::k_slot_mask];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
    datum_index actor_index = unit->actor_index;

    if (actor_index == (datum_index)k_datum_index_none) {
        actor_index = unit->swarm_actor_index;
    }

    if (actor_index != (datum_index)k_datum_index_none && packed_reference != (uint32_t)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
        char already_in_target = (a->encounter_index & halo::k_slot_mask) == (packed_reference & halo::k_slot_mask);
        uint8_t *actor_tag_data = (uint8_t *)halo::cache::globals().tag_instances[a->actor_definition_tag & halo::k_slot_mask].data;
        uint8_t *actor_variant_data = (uint8_t *)halo::cache::globals().tag_instances[a->actor_variant_tag & halo::k_slot_mask].data;
        int32_t best_squad = ai_squad_find_best_matching_member(packed_reference, a->squad_index, actor_tag_data,
                                                                  actor_variant_data, already_in_target);

        if ((int16_t)best_squad != -1 && (already_in_target == 0 || (int16_t)best_squad != a->squad_index)) {
            actor_reset_squad_link_for_type_change(actor_index, (datum_index)(packed_reference & halo::k_slot_mask),
                (int16_t)best_squad);
            if (notify != 0) {
                actor_notify_squad_and_flag_danger(actor_index, (uint8_t)notify, 0);
            }
        }
    }
}

/**
 * Sets force_active on the unit's actor when that actor has no encounter (hs ai_force_active_by_unit).
 *
 * @address 0x435540
 */
void AiUnitView::set_actor_force_active(uint8_t value)
{
    datum_index unit_index = handle;
    object_header *header = &((object_header *)object_data->data)[unit_index & halo::k_slot_mask];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
    datum_index actor_index = unit->actor_index;

    if (actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
        if (a->encounterless != 0) {
            a->force_active = value;
        }
    }
}

/**
 * Behaviour of ai unit set squad reference, moved unchanged from the original free function.
 *
 * @address 0x435750
 */
void AiObjects::set_squad_reference(datum_index object_index, uint32_t packed_reference)
{
    object *obj;
    ScenarioEncounter *definition;
    int16_t encounter_index;
    int16_t out_encounter;
    uint32_t out_squad;
    uint32_t squad_index;
    int32_t squad_count;
    int32_t i;
    datum_index actor_index;
    actor *a;

    if (object_index == (datum_index)k_datum_index_none) {
        return;
    }
    obj = ((object_header *)object_data->data)[object_index & halo::k_slot_mask].data;

    out_encounter = -1;
    out_squad = halo::k_word_none;

    encounter_index = (int16_t)packed_reference;
    if (packed_reference == halo::k_dword_none || encounter_index < 0 ||
        global_scenario->encounters.count <= (int32_t)encounter_index) {
        goto store;
    }

    definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index];
    squad_index = 0;

    if (packed_reference >> 0x1e == 1) {
        squad_count = definition->squads.count;
        squad_index = 0;
        if (0 < squad_count) {
            i = 0;
            do {
                if (((ScenarioSquad *)definition->squads.pointer)[i].platoon ==
                    (uint16_t)(uint8_t)(packed_reference >> 0x10)) {
                    break;
                }
                squad_index = squad_index + 1;
                i = (int32_t)(int16_t)squad_index;
            } while (i < squad_count);
        }
        if ((int16_t)squad_index >= squad_count) {
            squad_index = 0;
        } else if ((int16_t)squad_index < 0) {
            goto store;
        }
    } else if (packed_reference >> 0x1e == 2) {
        squad_index = (uint32_t)(uint8_t)(packed_reference >> 0x10);
        if ((int16_t)squad_index < 0) {
            goto store;
        }
    }

    if ((int32_t)(int16_t)squad_index < definition->squads.count) {
        out_squad = squad_index;
        out_encounter = encounter_index;
        if (encounter_index != -1 && (int16_t)squad_index != -1 &&
            *(int16_t *)((uint8_t *)obj + 0x334) != -1) {
            datum_index cursor[3];

            ai_reference_actor_iterator_init_cursor((int32_t)*(int16_t *)((uint8_t *)obj + 0x334), cursor);
            actor_index = cursor[2];
            while (ai_globals_ptr->actors_valid != 0 &&
                   actor_index != (datum_index)k_datum_index_none) {
                datum_index current = actor_index;
                a = &((actor *)actor_data->data)[current & halo::k_slot_mask];
                actor_index = a->next_in_encounter;
                if (a->active_unit_index == object_index) {
                    actor_reset_squad_link_for_type_change(current, (datum_index)(int32_t)encounter_index,
                        (int16_t)squad_index);
                }
            }
        }
    }

store:
    *(int16_t *)((uint8_t *)obj + 0x334) = out_encounter;
    *(int16_t *)((uint8_t *)obj + 0x336) = (int16_t)out_squad;
}

/**
 * Behaviour of swarm add component, moved unchanged from the original free function.
 *
 * @address 0x4279a0
 */
void AiObjects::add_component(datum_index component_index, uint32_t unit_index, datum_index swarm_index)
{
    swarm *s = &((swarm *)swarm_data->data)[swarm_index & halo::k_slot_mask];
    swarm_component *component = &((swarm_component *)swarm_component_data->data)[component_index & halo::k_slot_mask];

    component->leap_target_index = halo::k_dword_none;
    s->unit_index[s->component_count] = unit_index;
    s->component_index[s->component_count] = component_index;
    s->component_count = s->component_count + 1;

    actor_update_swarm_component_position(component_index, unit_index);
}

/**
 * Behaviour of unit get move speed for range, moved unchanged from the original free function.
 *
 * @address 0x41bed0
 */
void AiActorView::get_move_speed_for_range(float param_a, float param_b, float param_dist, float *out_a, float *out_b)
{
    datum_index actor_index = handle;
    actor *self;
    Actor *definition;
    float far_value;
    float near_value_07;
    float far_value_07_clamped;
    float fraction;
    float low_break;
    float low_break_08;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;

    if (definition->peripheral_vision_angle < param_dist) {
        *out_b = 0.0f;
        *out_a = 0.0f;
        return;
    }

    param_a = param_a * param_b;
    far_value = param_b * definition->peripheral_distance;
    near_value_07 = param_a * 0.7f;
    far_value_07_clamped = far_value * 0.7f;
    if (3.5f < far_value_07_clamped) {
        far_value_07_clamped = 3.5f;
    }

    if (param_dist <= definition->max_vision_angle) {
        low_break = definition->central_vision_angle;
        low_break_08 = low_break * 0.8f;
        if (low_break <= param_dist) {
            fraction = (param_dist - low_break) / (definition->max_vision_angle - low_break);
            param_a = fraction * far_value + (1.0f - fraction) * param_a;
        }
        if (low_break_08 <= param_dist) {
            fraction = (param_dist - low_break_08) / (definition->max_vision_angle - low_break_08);
            *out_b = param_a;
            *out_a = fraction * far_value_07_clamped + (1.0f - fraction) * near_value_07;
            return;
        }
        *out_b = param_a;
        *out_a = near_value_07;
        return;
    }

    *out_b = far_value;
    *out_a = far_value_07_clamped;
}

}
