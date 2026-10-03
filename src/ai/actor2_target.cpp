#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"

namespace halo::ai {

namespace actor_target_data_acquire_local {
extern "C" {
extern data_array *prop_data;
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    uint8_t create_if_missing, uint8_t flag);
extern datum_index actor_allocate_paired_prop(datum_index actor_index, datum_index existing_prop);
extern datum_index actor_allocate_paired_prop_with_kind(datum_index actor_index, datum_index existing_prop,
    datum_index reference_prop);
extern void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop);
extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force,
    char allow_reassign);
extern void actor_target_update_tracking_speed(uint32_t actor_index, datum_index target_prop_index,
    void *scratch);
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index);
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);
#define PROP(h) ((prop *)((uint8_t *)prop_data->data + ((h) & 0xffff) * sizeof(prop)))
}
}

/**
 * Actor AI behaviour: target data acquire.
 *
 * @address 0x41f7d0
 */
uint8_t ActorView::target_data_acquire(datum_index object_index, datum_index owner_reference, datum_index pair_reference)
{
    using namespace actor_target_data_acquire_local;
    uint8_t result = 1;
    datum_index resolved;
    datum_index current;
    prop *target;
    uint8_t scratch[0x38];

    resolved = actor_find_or_create_shared_prop(object_index, actor_index, 1, 0);
    if (resolved == k_datum_index_none) {
        return 1;
    }
    target = PROP(resolved);
    current = resolved;
    if (target->state >= 2 && target->state <= 3) {
        result = 0;
    } else if (target->pair_index != k_datum_index_none) {
        datum_index pair = target->pair_index;
        prop *paired = PROP(pair);
        uint8_t fresh = 0;

        if (pair_reference != k_datum_index_none) {
            actor_copy_prop_and_reset(pair, pair_reference);
            target->object_index = paired->object_index;
        } else {
            paired->state = 4;
            paired->inspection_ticks = 0;
            fresh = 1;
        }
        actor_target_data_refresh(actor_index, pair, scratch, (char)fresh, 1);
        actor_target_update_tracking_speed(actor_index, pair, scratch);
        current = pair;
        target = PROP(pair);
    } else {
        datum_index created;

        if (pair_reference != k_datum_index_none) {
            created = actor_allocate_paired_prop_with_kind(actor_index, resolved, pair_reference);
            if (created != k_datum_index_none) {
                prop *copy = PROP(created);

                copy->object_index = target->object_index;
                copy->owner_actor_index = target->owner_actor_index;
                copy->swarm_owned = target->swarm_owned;
            }
        } else {
            actor_target_data_refresh(actor_index, resolved, scratch, 0, 0);
            created = actor_allocate_paired_prop(actor_index, resolved);
        }
        if (created == k_datum_index_none) {
            return 0;
        }
        current = created;
        target = PROP(created);
    }

    if (owner_reference == k_datum_index_none ||
        (pair_reference != k_datum_index_none && PROP(pair_reference)->visual_perception >= 2)) {
        target->has_current_information = 1;
        target->information_age = 0;
        target->information_source_actor = owner_reference;
    }
    target->engaged = actor_target_update_active_flag(actor_index, current);
    target->desirability = actor_rate_potential_target(actor_index, current);
    return result;
}

#undef PROP

namespace actor_target_data_refresh_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;
extern game_time_globals *game_time;
extern char ai_marker_name_a[];
extern char ai_marker_name_b[];
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern void object_get_position(real_point3d *out_position, datum_index object_index);
extern datum_index object_get_root_object_index(uint32_t object_index);
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out);
extern char unit_get_tag_flag_bit7(uint32_t unit_index);
extern datum_index object_find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index, char stamp_group);
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point);
}
}

/**
 * Refreshes a prop's (target-data record's) cached object reference, aim marker offsets, and root-object status
 * for combat targeting.
 *
 * @address 0x41c4b0
 */
void ActorView::target_data_refresh(uint32_t target_prop_index, void *reference, char force, char allow_reassign)
{
    using namespace actor_target_data_refresh_local;
    actor *self;
    prop *target;
    object *unit_obj;
    object *parent_obj;
    object *child_obj;
    datum_index object_index;
    datum_index reassigned;
    datum_index parent_index;
    datum_index child_index;
    uint8_t local_transform[0x6c];
    uint32_t transform_x, transform_y, transform_z;
    uint8_t is_eligible;
    real_vector3d delta;
    float length;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->active == 0) {
        return;
    }

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    object_index = target->object_index;
    unit_obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (force == 0 && 3 < target->state && target->state < 6) {
        if (target->dead_confirmed != 0) {
            goto after_reassign;
        }
        {
            if (((unit_obj->vitality_flags & 4) == 0 || *(int16_t *)((uint8_t *)unit_obj + 0x420) != 0) ||
                (target->perception_level != 0 || 0.010000001f <= halo::math::vector3d_magnitude_squared(unit_obj->velocity))) {
                is_eligible = 0;
            } else {
                is_eligible = 1;
            }

            if ((self->target_unit_index == target_prop_index &&
                 (target->noticed_a == 0 || target->noticed_b == 0)) ||
                is_eligible == 0) {
                goto after_reassign;
            }
            target->dead_confirmed = 1;
            target->dead = 1;
        }
    }

    if (((target->swarm_owned != 0 && target->owner_actor_index != k_datum_index_none) && allow_reassign != 0) &&
        target->swarm_reassign_time + 0x5a <= game_time->game_time) {
        target->swarm_reassign_time = game_time->game_time;
        reassigned = object_find_nearest_squad_member(target->owner_actor_index, (void *)&self->aim_origin, object_index, 0);
        if (reassigned != object_index) {
            target->object_index = reassigned;
            unit_obj = ((object_header *)object_data->data)[reassigned & 0xffff].data;
            if (target->state < 4 || 5 < target->state) {
                if (target->pair_index != k_datum_index_none) {
                    ((prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop)))->object_index = reassigned;
                }
            } else {
                ((prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop)))->object_index = reassigned;
            }
        }
    }

    object_get_node_local_transform(target->object_index, ai_marker_name_a, (object_marker *)local_transform, 1);
    transform_x = *(uint32_t *)(local_transform + 0x60);
    transform_y = *(uint32_t *)(local_transform + 0x64);
    transform_z = *(uint32_t *)(local_transform + 0x68);
    *(uint32_t *)&target->head_position_x = transform_x;
    *(uint32_t *)&target->head_position_y = transform_y;
    *(uint32_t *)&target->head_position_z = transform_z;

    object_get_position(&target->last_known_position, target->object_index);

    object_get_node_local_transform(target->object_index, ai_marker_name_b, (object_marker *)local_transform, 1);
    *(uint32_t *)&target->center_of_mass.x = *(uint32_t *)(local_transform + 0x60);
    *(uint32_t *)&target->center_of_mass.y = *(uint32_t *)(local_transform + 0x64);
    *(uint32_t *)&target->center_of_mass.z = *(uint32_t *)(local_transform + 0x68);
    *(real_vector3d *)&target->velocity = unit_obj->velocity;
    target->pathfinding_surface_index = -1;

    reassigned = object_get_root_object_index(target->object_index);
    parent_obj = ((object_header *)object_data->data)[reassigned & 0xffff].data;
    target->location_leaf_index = *(float *)&parent_obj->location_leaf_index;
    *(uint32_t *)&target->cluster_index = *(uint32_t *)&parent_obj->location_cluster_index;

    target->in_water = scenario_location_get_water_and_weather(&target->center_of_mass, (bsp_leaf_reference *)&target->location_leaf_index, 0);
    target->relationship_object_index = -1;
    target->is_vehicle_gunner = 0;
    target->is_vehicle_driver = 0;
    *(uint32_t *)&target->parent_object_index = 0xffffffff;

    parent_index = unit_obj->parent_object;
    if (parent_index != k_datum_index_none) {
        parent_obj = ((object_header *)object_data->data)[parent_index & 0xffff].data;
        if (parent_obj->type == 1) {
            target->relationship_object_index = parent_index;
            if (*(int32_t *)((uint8_t *)parent_obj + 0x328) == (int32_t)target->object_index ||
                target->actor_type == 0xf) {
                target->is_vehicle_gunner = 1;
            } else {
                target->is_vehicle_gunner = 0;
            }
            if (*(int32_t *)((uint8_t *)parent_obj + 0x324) == (int32_t)target->object_index &&
                unit_get_tag_flag_bit7(parent_index) != 0) {
                target->is_vehicle_driver = 1;
            } else {
                target->is_vehicle_driver = 0;
            }
        } else if ((1 << (parent_obj->type & 0x1f) & 3) != 0) {
            *(uint32_t *)&target->parent_object_index = parent_index;
        }
    }

    target->child_unit_count = 0;
    child_index = unit_obj->first_child_object;
    while (child_index != k_datum_index_none) {
        child_obj = ((object_header *)object_data->data)[child_index & 0xffff].data;
        if ((1 << (child_obj->type & 0x1f) & 3) != 0) {
            target->child_unit_count = target->child_unit_count + 1;
        }
        child_index = child_obj->next_object;
    }

after_reassign:
    actor_get_firing_positions(actor_index, (uint32_t *)reference, &target->last_known_position);

    target->direction.x = target->last_known_position.x - *(float *)((uint8_t *)reference + 0xc);
    target->direction.y = target->last_known_position.y - *(float *)((uint8_t *)reference + 0x10);
    target->direction.z = target->last_known_position.z - *(float *)((uint8_t *)reference + 0x14);
    length = halo::math::vector3d_normalize_with_length(*((real_vector3d *)&target->direction));
    target->distance = length;

    if (length == 0.0f) {
        *(real_vector3d *)&target->direction = *halo::math::globals().global_forward3d_pointer;
    }
}

namespace actor_target_data_release_local {
extern "C" {
extern data_array *prop_data;
extern uint8_t actor_target_has_conflicting_neighbor(datum_index actor_index, datum_index target_prop_index);
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove);
extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference);
extern void actor_queue_sighted_target_dialogue(datum_index actor_index, datum_index target_prop_index, uint8_t already_noticed);
}
}

/**
 * Releases a linked prop (target-data record) -- deleting its paired datum and clearing the link -- once the
 * actor is done actively engaging it. Returns 1 if the prop's kind made it eligible for release (and was
 * released), 0 otherwise. When out_conflict_flag is non-NULL, it receives actor_target_has_c
 *
 * @address 0x41b980
 */
uint32_t TargetView::target_data_release(uint32_t actor_index, uint8_t *out_conflict_flag)
{
    using namespace actor_target_data_release_local;
    prop *target;
    prop *paired;
    datum_index pair_index;
    uint32_t result;
    uint8_t conflict;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    result = 0;
    conflict = 0;

    if (target->state < 2 || 3 < target->state) {
        pair_index = target->pair_index;
        conflict = actor_target_has_conflicting_neighbor(actor_index, target_prop_index);

        if (pair_index != k_datum_index_none) {
            paired = (prop *)((uint8_t *)prop_data->data + (pair_index & 0xffff) * sizeof(prop));

            target->desirability = paired->desirability;
            target->interest = paired->interest;
            target->interest_satisfied = paired->interest_satisfied;
            target->last_attention_time = paired->last_attention_time;
            target->engaged_ticks = paired->engaged_ticks;
            target->last_engaged_time = paired->last_engaged_time;
            target->engaged = paired->engaged;
            target->friends_killed = paired->friends_killed;
            target->friends_killed_timer = paired->friends_killed_timer;

            actor_replace_object_reference(actor_index, target_prop_index, pair_index);
            actor_unlink_prop(actor_index, pair_index);
            halo::memory::datum_delete(prop_data, pair_index);
            target->pair_index = k_datum_index_none;
        }

        target->state = 3;
        target->noticed_b = 0;
        target->noticed_a = 0;
        target->noticed_c = 0;
        target->combat_dirty = 1;
        actor_queue_sighted_target_dialogue(actor_index, target_prop_index, conflict);
        result = 1;
    }

    if (out_conflict_flag != (uint8_t *)0) {
        *out_conflict_flag = conflict;
    }
    return result;
}

namespace actor_target_get_backup_priority_local {
extern "C" {
extern data_array *prop_data;
}
}

/**
 * Returns a small integer priority ranking how urgently a prop (target-data record) needs backup/assistance
 * based on its current combat sub-state.
 *
 * @address 0x420e50
 */
uint8_t TargetView::target_get_backup_priority()
{
    using namespace actor_target_get_backup_priority_local;
    prop *target;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (1 < target->state && target->state < 4 && target->engaged != 0) {
        if (target->seen != 0) {
            return 4;
        }
        if (target->shooting != 0) {
            return (uint8_t)(((int8_t)target->aiming_at_actor_class <= 1) + 2);
        }
        if (1 < target->visual_perception) {
            return 1;
        }
    }
    return 0;
}

namespace actor_target_get_priority_class_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

/**
 * Computes a priority/urgency class (0-3) for a given target relative to the actor, used to rank target
 * handling. A target in an active-combat kind, occupying a seat, or an exposed non-unit target the actor is
 * alert enough to notice, always ranks 3. Otherwise, a paired prop (e.g. a vehicle and its rid
 *
 * @address 0x41be10
 */
uint16_t ActorView::target_get_priority_class(datum_index target_prop_index)
{
    using namespace actor_target_get_priority_class_local;
    actor *self;
    prop *target;
    prop *paired;
    uint16_t paired_class;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (target_prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

        if (((1 < target->state && target->state < 4) || target->stimulus_type == 1 || target->stimulus_type == 2) ||
            (target->enemy == 0 && (target->dead == 0 || self->awareness_level > 2))) {
            return 3;
        }

        if (target->pair_index != k_datum_index_none) {
            paired = (prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop));
            paired_class = (uint16_t)((paired->has_current_information != 0) + 2);
            if (paired_class != 0xffff) {
                return paired_class;
            }
        }
    }

    if (self->combat_status < 2) {
        return (uint16_t)(self->awareness_level > 2);
    }
    return 2;
}

namespace actor_target_get_relationship_object_local {
extern "C" {
extern data_array *prop_data;
extern int32_t unit_predict_aim_target_position(uint32_t unit_index, real_point3d *out_position);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position);
}
}

/**
 * Lazily resolves and caches a prop's (a target-data record's) associated relationship / obstruction object
 * handle. If the record already has an explicit relationship link, that is resolved through
 * unit_predict_aim_target_position; otherwise, when the local player object is available, the tracked obje
 *
 * @address 0x41f3a0
 */
void TargetView::target_get_relationship_object()
{
    using namespace actor_target_get_relationship_object_local;
    prop *target;
    int32_t *cache;
    datum_index resolved;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    cache = (int32_t *)((uint8_t *)target + 0xec);

    if (*cache == -1) {
        if (target->relationship_object_index != -1) {
            *cache = unit_predict_aim_target_position(target->relationship_object_index,
                                                      (real_point3d *)((uint8_t *)target + 0xf0));
            return;
        }
        resolved = target->object_index;
        if (object_try_and_get(resolved, 1) != (void *)0) {
            resolved = biped_get_cached_look_at_position(resolved, (real_point3d *)((uint8_t *)target + 0xf0));
            *cache = (int32_t)resolved;
        }
    }
}

namespace actor_target_has_conflicting_neighbor_local {
extern "C" {
extern double fabs(double x);
static float fabs_f(float x) { return (float)fabs((double)x); }
extern data_array *actor_data;
extern data_array *prop_data;
}
}

/**
 * Checks whether another of the actor's tracked props occupies a conflicting firing position or object, to avoid
 * duplicate assignment.
 *
 * @address 0x41f410
 */
uint8_t ActorView::target_has_conflicting_neighbor(datum_index target_prop_index)
{
    using namespace actor_target_has_conflicting_neighbor_local;
    actor *self;
    prop *target;
    prop *other;
    datum_index prop_index;
    uint8_t conflict;
    float dx, dy;
    int16_t other_kind;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    conflict = 0;

    prop_index = self->first_prop;
    while (prop_index != k_datum_index_none) {
        other = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        if (prop_index != target_prop_index) {
            other_kind = other->state;
            if (((other->object_index == target->object_index ||
                  other->owner_actor_index == target->owner_actor_index) ||
                 ((target->enemy != 0 && other->enemy != 0) &&
                  ((3 < other_kind && other_kind < 6) || (1 < other_kind && other_kind < 4)))) &&
                (((dx = target->last_known_position.x - other->last_known_position.x,
                   dy = target->last_known_position.y - other->last_known_position.y,
                   dx * dx + dy * dy < 6.25f) &&
                  fabs_f(other->last_known_position.z - target->last_known_position.z) < 1.5f) &&
                 (0.5f < other->direction.z * target->direction.z +
                         other->direction.y * target->direction.y +
                         other->direction.x * target->direction.x))) {
                conflict = 1;
            }
        }
        prop_index = other->next_in_actor;
    }

    return conflict;
}

namespace actor_target_hearing_check_local {
extern "C" {
extern double sqrt(double x);
static float sqrt_f(float x) { return (float)sqrt((double)x); }
extern data_array *actor_data;
extern ScenarioStructureBSP *global_structure_bsp;
extern uint8_t scenario_location_background_sound_is_deafening_to_ais(bsp_leaf_reference *location);
}
}

/**
 * REWRITTEN from objdump 0x41c030..0x41c1dc. EAX: actor; ECX: the listener block (+0x0 position, +0x18 facing,
 * +0x24 location, +0x28 cluster word); EBX: the sound's gate (0 = silent); ESI: the sound position; stack: the
 * sound's location (cluster at +0x4) and stance. The Actor tag's hearing distance (+
 *
 * @address 0x41c030
 */
uint16_t ActorOps::target_hearing_check(void *record, int16_t stance, datum_index actor_index, void *target_ref, int16_t gate, real_point3d *listener_position)
{
    using namespace actor_target_hearing_check_local;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *listener = (uint8_t *)target_ref;
    int16_t listener_cluster;
    int16_t source_cluster;
    float range;
    float dx, dy, dz, distance_squared;
    uint8_t pas;
    float sound_distance;
    float distance;

    if (gate == 0) {
        return 0;
    }
    listener_cluster = *(int16_t *)(listener + 0x28);
    if (listener_cluster == -1) {
        return 0;
    }
    source_cluster = *(int16_t *)((uint8_t *)record + 0x4);
    if (source_cluster == -1) {
        return 0;
    }
    range = *(float *)((uint8_t *)halo::cache::globals().tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data + 0x4c);
    dx = listener_position->x - *(float *)(listener + 0x0);
    dy = listener_position->y - *(float *)(listener + 0x4);
    dz = listener_position->z - *(float *)(listener + 0x8);
    distance_squared = dz * dz + dy * dy + dx * dx;
    if (!(dz * *(float *)(listener + 0x20) + dy * *(float *)(listener + 0x1c) + dx * *(float *)(listener + 0x18) >= 0.0f)) {
        range = range * 0.8f;
    }
    if (((actor *)a)->awareness_level == 2) {
        range = range * 0.7f;
    } else if (((actor *)a)->awareness_level == 1) {
        range = range * 0.4f;
    }
    if (gate == 4) {
        range = range * 0.2f;
    } else if (gate == 1) {
        range = range * 0.45f;
    } else if (gate == 3) {
        range = range * 0.7f;
    }
    if (scenario_location_background_sound_is_deafening_to_ais((bsp_leaf_reference *)(listener + 0x24)) ||
        scenario_location_background_sound_is_deafening_to_ais((bsp_leaf_reference *)record)) {
        range = range * 0.25f;
    }
    if (stance != 0 && stance != 1) {
        range = range * 0.7f;
    }
    if (!(range * range > distance_squared)) {
        return 0;
    }
    pas = halo::structures::cluster_sound_distance_lookup(listener_cluster, source_cluster, global_structure_bsp);
    if (pas & 0x80) {
        return 0;
    }
    sound_distance = (float)(int32_t)(pas & 0x7f) * 2.0157480f;
    sound_distance = sound_distance + sound_distance;
    distance = (float)sqrt((double)distance_squared);
    if (!(sound_distance > distance)) {
        sound_distance = distance;
    }
    if (sound_distance >= range) {
        return 0;
    }
    return (uint16_t)((gate >= 3) + 2);
}

namespace actor_target_is_visible_or_object_count_ok_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count);
}
}

/**
 * Actor AI behaviour: target is visible or object count ok.
 *
 * @address 0x40f700
 */
uint8_t ActorView::target_is_visible_or_object_count_ok(int16_t kind)
{
    using namespace actor_target_is_visible_or_object_count_ok_local;
    actor *self;
    prop *p;
    int16_t hostile_count;

    if (kind != 3) {
        return 1;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    p = (prop *)((uint8_t *)prop_data->data + (self->firing_target_prop_index & 0xffff) * sizeof(prop));

    if (p->relationship_object_index != -1) {
        return 1;
    }
    if (p->is_parented != 0) {
        return 0;
    }

    hostile_count = 0;
    actor_score_blast_area_clear(actor_index, 6.0f, 0.0f, &p->last_known_position, &hostile_count);
    return 2 < hostile_count;
}

namespace actor_target_mark_engaged_local {
extern "C" {
extern data_array *prop_data;
extern game_time_globals *game_time;
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index);
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);
}
}

/**
 * Marks (or clears) the given prop (target-data record) as actively engaged and refreshes its derived combat
 * timing fields.
 *
 * @address 0x41fa80
 */
void TargetView::target_mark_engaged(datum_index actor_index, uint8_t mark_engaged)
{
    using namespace actor_target_mark_engaged_local;
    prop *target;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (mark_engaged == 0) {
        target->engaged_ticks = 0;
        target->last_engaged_time = -1;
    } else {
        if (target->engaged_ticks == 0) {
            target->engaged_ticks = 1;
        }
        target->last_engaged_time = game_time->game_time;
    }

    target->engaged = actor_target_update_active_flag(actor_index, target_prop_index);
    target->desirability = actor_rate_potential_target(actor_index, target_prop_index);
}

namespace actor_target_reset_combat_flags_local {
extern "C" {
extern data_array *prop_data;
extern void actor_queue_sighted_target_dialogue(datum_index actor_index, datum_index target_prop_index,
    uint8_t already_noticed);
}
}

/**
 * REWRITTEN from objdump 0x41baf0..0x41bb26. ECX: prop; stack: (actor, a slot the function overwrites with the
 * prop, already_noticed). Clears the prop's three notice flags (+0xb9..+0xbb), marks +0x64, then tail-calls
 * actor_queue_sighted_target_dialogue(actor, prop, already_noticed). The draft had no a
 *
 * @address 0x41baf0
 */
void TargetView::target_reset_combat_flags(datum_index actor_index, uint32_t unused, uint8_t already_noticed)
{
    using namespace actor_target_reset_combat_flags_local;
    uint8_t *p = (uint8_t *)prop_data->data + (target_prop_index & 0xffff) * 0x138;

    (void)unused;
    p[0xba] = 0;
    p[0xb9] = 0;
    p[0xbb] = 0;
    p[0x64] = 1;
    actor_queue_sighted_target_dialogue(actor_index, target_prop_index, already_noticed);
}

namespace actor_target_reset_seen_flags_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

/**
 * Clears the visibility/'seen' state of every prop (target-data record) the actor is currently tracking.
 *
 * @address 0x41f9d0
 */
void ActorView::target_reset_seen_flags()
{
    using namespace actor_target_reset_seen_flags_local;
    actor *self;
    datum_index prop_index;
    prop *target;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    prop_index = self->first_prop;
    while (prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        prop_index = target->next_in_actor;
        target->seen = 0;
        target->seen_state = -1;
    }
}

namespace actor_target_reset_shot_counters_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

/**
 * Resets per-target shot/hit statistics counters for every prop (target-data record) the actor is tracking.
 *
 * @address 0x41fa20
 */
void ActorView::target_reset_shot_counters()
{
    using namespace actor_target_reset_shot_counters_local;
    actor *self;
    datum_index prop_index;
    prop *target;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    prop_index = self->first_prop;
    while (prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        prop_index = target->next_in_actor;
        target->shots_fired = 0;
        target->shots_unknown_ae = 0;
        target->shots_hit = 0;
    }
}

namespace actor_target_scan_potential_targets_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *swarm_data;
extern data_array *object_data;
extern data_array *encounter_data;
extern datum_index *collideable_cluster_first;
extern data_array *collideable_object_references;
extern datum_index *noncollideable_cluster_first;
extern data_array *noncollideable_object_references;
extern ScenarioStructureBSP *global_structure_bsp;
extern object_globals *object_globals_pointer;
extern int32_t object_cluster_stamp;
extern int ai_target_distance_qsort_compare(void *record_a, void *record_b);
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index);
extern int16_t object_get_root_parent_placement(uint32_t object_index,
    object_placement_cursor *out_cursor);
extern void actor_target_evaluate_squad_link(uint32_t actor_index, datum_index object_cursor,
    int16_t *candidates_a, int16_t *candidates_b);
extern datum_index actor_find_or_allocate_prop(uint32_t actor_index, datum_index object_index, char flag);
extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force, char allow_reassign);
extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference);
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove);
}
}

/**
 * TYPES-GAP: the local sort record ai_target_distance_qsort_compare.c already documents 128-entry array this
 * function builds it into (Ghidra's local_c08/local_c04 and local_604/local_600, each pair adjacent on the stack
 * and indexed as one blob). ai_target_candidate now lives in types/ai.h (folded from
 *
 * @address 0x41d7e0
 */
void ActorView::target_scan_potential_targets()
{
    using namespace actor_target_scan_potential_targets_local;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    prop *props = (prop *)prop_data->data;
    uint32_t *pvs_bitmap = 0;
    uint32_t swarm_pvs[16];
    ai_target_candidate_list list_a;
    ai_target_candidate_list list_b;
    int32_t row_dwords;
    int32_t stamp;
    datum_index next, current;

    list_a.seen_count = 0;
    list_a.entry_count = 0;
    list_b.seen_count = 0;
    list_b.entry_count = 0;

    row_dwords = (global_structure_bsp->clusters.count + 0x1f) >> 5;

    if (!self->swarm) {
        int16_t cluster_ref = *(int16_t *)&self->unknown_138[0x148 - 0x138];
        if (cluster_ref != -1) {
            pvs_bitmap = (uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                       row_dwords * cluster_ref * 4);
        }
    } else {
        swarm *sw = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        uint8_t any = 0;
        int16_t i;

        for (i = 0; i < 16; i++) {
            swarm_pvs[i] = 0;
        }
        for (i = 0; i < sw->component_count; i++) {
            object_header *hdr = (object_header *)object_data->data + (sw->unit_index[i] & 0xffff);
            object *unit_obj = hdr->data;
            int16_t cluster = unit_obj->location_cluster_index;
            if (cluster != -1) {
                int32_t j;
                for (j = row_dwords - 1; j >= 0; j--) {
                    swarm_pvs[j] |= *(uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                                   row_dwords * cluster * 4 + j * 4);
                }
                any = 1;
            }
        }
        if (any) {
            pvs_bitmap = swarm_pvs;
        }
    }

    object_cluster_stamp++;
    object_globals_pointer->collecting_in_clusters = 1;
    stamp = object_cluster_stamp;

    next = self->first_prop;
    while (current = next, current != k_datum_index_none) {
        prop *p = &props[current & 0xffff];
        uint8_t accept;
        uint8_t unit_bucket;

        next = p->next_in_actor;

        if (p->state < 4 || 5 < p->state) {
            float dist_sq = p->distance * p->distance;
            actor *owner = (p->owner_actor_index == k_datum_index_none)
                               ? (actor *)0
                               : &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
            uint16_t threshold_bits;

            unit_bucket = 0;

            if (p->is_parented) {
                accept = 1;
            } else if (owner != (actor *)0 && !(owner->active != 0 && owner->keep_unit_alive == 0)) {
                accept = 0;
            } else if (p->in_use != 0 || 0 < p->retain_timer) {
                accept = 1;
            } else if (1600.0f < dist_sq) {
                accept = 0;
            } else if (!p->dead) {
                if (!p->enemy) {
                    accept = dist_sq < 225.0f;
                    if (3 < self->combat_status) {
                        unit_bucket = 1;
                        goto merged;
                    }
                    if (self->unknown_1cc == 0) {
                        threshold_bits = (uint16_t)(dist_sq < 16.0f) << 8 |
                                          (uint16_t)(dist_sq == 16.0f) << 0xe;
                        goto shared_threshold;
                    }
                    unit_bucket = 0;
                } else {
                    threshold_bits = (uint16_t)(dist_sq < 36.0f) << 8 |
                                      (uint16_t)(dist_sq == 36.0f) << 0xe;
                    accept = 1;
shared_threshold:
                    unit_bucket = 1;
                    if (threshold_bits == 0) goto merged;
                    unit_bucket = 0;
                }
            } else {
                datum_index encounter_idx = self->encounter_index;
                accept = 1;
                if (encounter_idx != k_datum_index_none) {
                    encounter *enc = &((encounter *)encounter_data->data)[encounter_idx & 0xffff];
                    int32_t gate = (enc->last_idle_time <= self->found_body_time) ? self->found_body_time
                                                                           : enc->last_idle_time;
                    if (gate != -1) {
                        object_header *ohdr = (object_header *)object_data->data + (p->object_index & 0xffff);
                        unit_data *u = (unit_data *)((uint8_t *)ohdr->data + k_unit_data_offset);
                        int32_t last_seen = u->death_time;
                        if (last_seen == -1 || last_seen < gate) {
                            accept = 0;
                        }
                    }
                    if (!(enc->engaged == 0 && enc->has_live_target == 0 && enc->stood_down == 0)) {
                        goto encounter_gate_open;
                    }
                    if (!accept) goto merged;
                    if (dist_sq < 225.0f) {
                        accept = 1;
                        goto merged;
                    }
                    goto not_accepted;
                }
encounter_gate_open:
                if (p->danger_radius <= 0.0f) {
                    if (!p->enemy || p->dead_ticks < 0x97) {
                        int16_t grade;
                        grade = actor_get_current_mode_combat_grade(actor_index);
                        if (grade < 2) {
                            float grade_threshold = 16.0f;
                            if (!p->enemy && self->awareness_level < 3) {
                                grade_threshold = 64.0f;
                            }
                            if (dist_sq < grade_threshold) {
                                accept = 1;
                                goto merged;
                            }
                        }
not_accepted:
                        accept = 0;
                    } else {
                        accept = 0;
                    }
                } else {
                    accept = 1;
                }
            }
merged:
            if (accept && pvs_bitmap != 0) {
                accept = 0;
                {
                    object_placement_cursor cursor;
                    int16_t ref = object_get_root_parent_placement(p->object_index, &cursor);
                    while (ref != -1) {
                        if (pvs_bitmap[(int16_t)ref >> 5] & (1u << ((uint8_t)ref & 0x1f))) {
                            accept = 1;
                            break;
                        }
                        if (cursor.next_reference == k_datum_index_none) {
                            ref = -1;
                        } else {
                            data_array *references = (data_array *)cursor.cluster_globals[2];
                            object_cluster_reference *node =
                                (object_cluster_reference *)references->data + (cursor.next_reference & 0xffff);
                            cursor.next_reference = node->next_reference;
                            ref = (int16_t)node->object_index;
                        }
                    }
                }
            }

            if (p->swarm_owned && p->owner_actor_index != k_datum_index_none) {
                actor *owner2 = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                datum_index cluster_head = owner2->swarm_index;
                if (cluster_head == k_datum_index_none) {
                    datum_index u = self->cluster_unit_index;
                    while (u != k_datum_index_none) {
                        object_header *ohdr = (object_header *)object_data->data + (u & 0xffff);
                        object *uobj = ohdr->data;
                        if (uobj->cluster_stamp != stamp) {
                            uobj->cluster_stamp = stamp;
                        }
                        u = ((unit_data *)((uint8_t *)uobj + k_unit_data_offset))->swarm_next_unit_index;
                    }
                } else {
                    swarm *sw2 = &((swarm *)swarm_data->data)[cluster_head & 0xffff];
                    int16_t i;
                    for (i = 0; i < sw2->component_count; i++) {
                        object_header *ohdr = (object_header *)object_data->data + (sw2->unit_index[i] & 0xffff);
                        object *uobj = ohdr->data;
                        if (uobj->cluster_stamp != stamp) {
                            uobj->cluster_stamp = stamp;
                        }
                    }
                }
            }
            {
                object_header *ohdr = (object_header *)object_data->data + (p->object_index & 0xffff);
                object *tobj = ohdr->data;
                if (tobj->cluster_stamp != stamp) {
                    tobj->cluster_stamp = stamp;
                }
            }

            if (accept) {
                ai_target_candidate_list *list = p->enemy ? &list_a : &list_b;
                if (unit_bucket) {
                    if (list->entry_count < 0x80) {
                        list->entries[list->entry_count].object_index = p->object_index;
                        list->entries[list->entry_count].prop_index = current;
                        list->entries[list->entry_count].distance = dist_sq * 0.6944444f;
                        list->entry_count++;
                    }
                } else if (!p->dead) {
                    list->seen_count++;
                }
            } else {
                if ((p->state < 4 || 5 < p->state) && p->pair_index != k_datum_index_none) {
                    actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(p->pair_index));
                    actor_unlink_prop(actor_index, p->pair_index);
                    halo::memory::datum_delete(prop_data, p->pair_index);
                }
                actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(current));
                actor_unlink_prop(actor_index, current);
                halo::memory::datum_delete(prop_data, current);
            }
        }
    }

    if (pvs_bitmap != 0 && global_structure_bsp->clusters.count > 0) {
        int32_t cluster;
        for (cluster = 0; cluster < global_structure_bsp->clusters.count; cluster++) {
            if (pvs_bitmap[cluster >> 5] & (1u << (cluster & 0x1f))) {
                datum_index head;
                int32_t owner_cluster_ref, chain_object;

                head = collideable_cluster_first[cluster];
                if (head == k_datum_index_none) {
                    owner_cluster_ref = -1; chain_object = -1;
                } else {
                    object_cluster_reference *node =
                        (object_cluster_reference *)collideable_object_references->data + (head & 0xffff);
                    owner_cluster_ref = node->next_reference;
                    chain_object = node->object_index;
                }
                while (chain_object != -1) {
                    actor_target_evaluate_squad_link(actor_index, chain_object,
                                                     (int16_t *)&list_a, (int16_t *)&list_b);
                    if (owner_cluster_ref == -1) {
                        chain_object = -1;
                    } else {
                        object_cluster_reference *node =
                            (object_cluster_reference *)collideable_object_references->data +
                            (owner_cluster_ref & 0xffff);
                        owner_cluster_ref = node->next_reference;
                        chain_object = node->object_index;
                    }
                }

                head = noncollideable_cluster_first[cluster];
                if (head == k_datum_index_none) {
                    owner_cluster_ref = -1; chain_object = -1;
                } else {
                    object_cluster_reference *node =
                        (object_cluster_reference *)noncollideable_object_references->data + (head & 0xffff);
                    owner_cluster_ref = node->next_reference;
                    chain_object = node->object_index;
                }
                while (chain_object != -1) {
                    actor_target_evaluate_squad_link(actor_index, chain_object,
                                                     (int16_t *)&list_a, (int16_t *)&list_b);
                    if (owner_cluster_ref == -1) {
                        chain_object = -1;
                    } else {
                        object_cluster_reference *node =
                            (object_cluster_reference *)noncollideable_object_references->data +
                            (owner_cluster_ref & 0xffff);
                        owner_cluster_ref = node->next_reference;
                        chain_object = node->object_index;
                    }
                }
            }
        }
    }

    if (list_a.entry_count > 0) {
        int16_t i = 0;
        int16_t bound = list_a.entry_count;

        if (list_a.seen_count < 4) {
            qsort(list_a.entries, list_a.entry_count, sizeof(ai_target_candidate),
                   (int32_t (*)(const void *, const void *))ai_target_distance_qsort_compare);
            if (list_a.entry_count > 0) {
                do {
                    if (list_a.entries[i].prop_index == k_datum_index_none) {
                        datum_index new_prop = actor_find_or_allocate_prop(actor_index, list_a.entries[i].object_index, 1);
                        if (new_prop != k_datum_index_none) {
                            actor_target_data_refresh(actor_index, new_prop, swarm_pvs, 0, 0);
                            goto list_a_counted;
                        }
                    } else {
list_a_counted:
                        list_a.seen_count++;
                        bound = list_a.entry_count;
                        if (3 < list_a.seen_count) goto list_a_evict;
                    }
                    i++;
                } while (i < list_a.entry_count);
            }
        } else {
list_a_evict:
            if (i < bound) {
                do {
                    if (list_a.entries[i].prop_index != k_datum_index_none) {
                        prop *existing = &props[list_a.entries[i].prop_index & 0xffff];
                        if ((existing->state < 4 || 5 < existing->state) &&
                            existing->pair_index != k_datum_index_none) {
                            actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(existing->pair_index));
                            actor_unlink_prop(actor_index, existing->pair_index);
                            halo::memory::datum_delete(prop_data, existing->pair_index);
                        }
                        actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(list_a.entries[i].prop_index));
                        actor_unlink_prop(actor_index, list_a.entries[i].prop_index);
                        halo::memory::datum_delete(prop_data, list_a.entries[i].prop_index);
                    }
                    i++;
                } while (i < list_a.entry_count);
            }
        }
    }

    if (list_b.entry_count > 0) {
        int16_t i = 0;
        int16_t combined = (int16_t)(list_a.seen_count + list_b.seen_count);
        int16_t cap = list_a.seen_count + 2;

        if (cap < 5) cap = 4;

        if (combined < cap) {
            qsort(list_b.entries, list_b.entry_count, sizeof(ai_target_candidate),
                   (int32_t (*)(const void *, const void *))ai_target_distance_qsort_compare);
            if (list_b.entry_count > 0) {
                do {
                    if (list_b.entries[i].prop_index == k_datum_index_none) {
                        datum_index new_prop = actor_find_or_allocate_prop(actor_index, list_b.entries[i].object_index, 0);
                        if (new_prop != k_datum_index_none) {
                            actor_target_data_refresh(actor_index, new_prop, swarm_pvs, 0, 0);
                            goto list_b_counted;
                        }
                    } else {
list_b_counted:
                        list_b.seen_count++;
                        combined++;
                        if (cap <= combined) goto list_b_evict;
                    }
                    i++;
                } while (i < list_b.entry_count);
                object_globals_pointer->collecting_in_clusters = 0;
                return;
            }
        } else {
list_b_evict:
            if (list_b.entry_count <= i) {
                object_globals_pointer->collecting_in_clusters = 0;
                return;
            }
            do {
                if (list_b.entries[i].prop_index != k_datum_index_none) {
                    prop *existing = &props[list_b.entries[i].prop_index & 0xffff];
                    if ((existing->state < 4 || 5 < existing->state) &&
                        existing->pair_index != k_datum_index_none) {
                        actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(existing->pair_index));
                        actor_unlink_prop(actor_index, existing->pair_index);
                        halo::memory::datum_delete(prop_data, existing->pair_index);
                    }
                    actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(list_b.entries[i].prop_index));
                    actor_unlink_prop(actor_index, list_b.entries[i].prop_index);
                    halo::memory::datum_delete(prop_data, list_b.entries[i].prop_index);
                }
                i++;
            } while (i < list_b.entry_count);
        }
    }

    object_globals_pointer->collecting_in_clusters = 0;
}

namespace actor_target_update_active_flag_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

/**
 * Recomputes whether a prop (target-data record) counts as actively engaged in combat this tick and clears stale
 * shot-counter and backup-request bookkeeping when it stops being active. Returns the new active flag.
 *
 * @address 0x41fc60
 */
uint8_t ActorView::target_update_active_flag(datum_index target_prop_index)
{
    using namespace actor_target_update_active_flag_local;
    actor *self;
    prop *target;
    int16_t kind;
    uint8_t active;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    kind = target->state;
    active = 0;

    if ((((1 < kind && kind < 4) && target->enemy != 0) && target->dead == 0) &&
        (((target->engaged_ticks != 0 &&
           (self->target_unit_index == target_prop_index || self->tally.unit_props_unseen == 0)) ||
          ((target->is_vehicle_gunner != 0 || target->is_vehicle_driver != 0) &&
           (self->vehicle_gunner == 0 && self->tally.group_a_marked_135 == 0))) ||
         (target->actor_type == 0xf))) {
        active = 1;
    }

    if (target->engaged != 0 && active == 0) {
        target->shots_fired = 0;
        target->shots_unknown_ae = 0;
        target->shots_hit = 0;
    }

    if (((1 < kind && kind < 4) && active == 0) &&
        (0 < self->retreat_timer && self->retreat_prop_index == target_prop_index)) {
        self->retreat_timer = 0;
        self->retreat_prop_index = k_datum_index_none;
    }

    target->engaged = active;
    return active;
}

namespace actor_target_update_tracking_speed_local {
extern "C" {
extern double sqrt(double x);
static float sqrtf_(float x) { return (float)sqrt((double)x); }
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;
extern data_array *encounter_data;
extern game_time_globals *game_time;
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index);
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity);
extern void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index,
    uint8_t mark_engaged);
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index);
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);
extern float actor_compute_target_priority_weight(datum_index prop_index, datum_index actor_index);
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
    actor_vocalization_context *context);
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index,
    void *target_ref, int16_t gate, real_point3d *listener_position);
extern uint16_t actor_target_get_priority_class(datum_index actor_index, datum_index target_prop_index);
extern uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index,
    datum_index object_index, uint8_t unknown_byte);
extern uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index,
    float radius, float distance, char accept_flag, uint8_t unknown_byte);
extern uint8_t actor_check_burst_length_exceeded(uint32_t actor_index);
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying);
extern int16_t actor_dispatch_look_handler_by_posture(int16_t kind, uint32_t actor_index, void *scratch, void *out_record,
    uint8_t rate_flag, uint8_t urgent_flag,
    uint16_t priority_class);
}
}

/**
 * Refreshes one prop's (target-data record's) derived aim/tracking classification: how fast the tracked object
 * is moving and turning relative to the actor's aim (buckets at +0x123/ +0x124), lead-distance and
 * cone-visibility buckets (+0x120..+0x122), the "same squad/ platoon" flag (+0x134), reachabilit
 *
 * @address 0x41c8f0
 */
void ActorView::target_update_tracking_speed(datum_index target_prop_index, void *scratch)
{
    using namespace actor_target_update_tracking_speed_local;
    actor *self;
    prop *p;
    Actor *actor_def;
    encounter *enc;
    object *unit_obj;
    unit_data *unit;
    uint8_t team_gate;
    int32_t tick;
    real_vector3d velocity;
    float speed;
    float closing_rate;
    uint8_t old_speed_bucket;
    uint8_t reachable;
    uint8_t owner_not_fully_aware;
    uint8_t owner_stalled;
    uint8_t engage_flag;

    self = &((actor *)actor_data->data)[actor_index & 0xffff];
    if (!self->active) {
        return;
    }

    actor_def = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & 0xffff].data;
    enc = (self->encounter_index == (datum_index)k_datum_index_none)
              ? (encounter *)0
              : &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
    p = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    unit_obj = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    tick = game_time->game_time;

    team_gate = 0;
    if ((enc != (encounter *)0 && enc->blind != 0) || self->awareness_level == 1) {
        team_gate = 1;
    }

    p->disregarded = (uint8_t)(unit->flags >> 10) & 1;
    if (p->enemy) {
        p->preferred_target = (uint8_t)(unit->flags >> 11) & 1;
        if (self->try_to_fight_type == 1) {
            if (p->owner_actor_index != (datum_index)k_datum_index_none) {
                uint32_t team_ref = self->try_to_fight_reference;
                if (team_ref != 0xffffffff) {
                    actor *owner = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                    if (((owner->encounter_index ^ team_ref) & 0xffff) == 0) {
                        uint32_t team_kind = team_ref >> 0x1e;
                        uint8_t match = 1;
                        if (team_kind == 1) {
                            match = (*(uint16_t *)((uint8_t *)&self->try_to_fight_reference + 2)  == (uint16_t)owner->squad_index);
                        } else if (team_kind == 2) {
                            match = (*(uint16_t *)((uint8_t *)&self->try_to_fight_reference + 2) == (uint16_t)owner->platoon_index);
                        } else if (team_kind != 0) {
                            match = 0;
                        }
                        if (match) {
                            p->preferred_target = 1;
                        }
                    }
                }
            }
        } else if (self->try_to_fight_type == 2 && p->is_parented) {
            p->preferred_target = 1;
        }
    }

    old_speed_bucket = p->speed_class;
    object_get_root_object_velocities(p->object_index, &velocity, (real_vector3d *)0);
    speed = sqrtf_(velocity.i * velocity.i + velocity.j * velocity.j + velocity.k * velocity.k);
    if (speed < 0.0033333334f) {
        p->speed_class = 0;
    } else if (speed < 0.016666668f) {
        p->speed_class = 1;
    } else if (speed < 0.033333335f) {
        p->speed_class = 2;
    } else {
        p->speed_class = 3;
    }

    closing_rate = -((velocity.i - *(float *)((uint8_t *)scratch + 0x2c)) * p->direction.x +
                      (velocity.j - *(float *)((uint8_t *)scratch + 0x30)) * p->direction.y +
                      (velocity.k - *(float *)((uint8_t *)scratch + 0x34)) * p->direction.z);
    if (closing_rate < -0.033333335f) {
        p->closing_speed_class = 0;
    } else if (closing_rate < -0.016666668f) {
        p->closing_speed_class = 1;
    } else if (closing_rate < -0.0033333334f) {
        p->closing_speed_class = 2;
    } else if (closing_rate < 0.0033333334f) {
        p->closing_speed_class = 3;
    } else if (closing_rate < 0.016666668f) {
        p->closing_speed_class = 4;
    } else if (closing_rate < 0.033333335f) {
        p->closing_speed_class = 5;
    } else {
        p->closing_speed_class = 6;
    }

    if (p->state > 1 && p->state < 4 && old_speed_bucket < 2 && p->speed_class > 1) {
        actor_vocalization_context ctx;
        ctx.kind = 1;
        ctx.handle = target_prop_index;
        actor_begin_vocalization(actor_index, 2, 1, &ctx);
    }

    if (p->distance < 1.0f) {
        p->distance_class = 0;
    } else if (p->distance < 6.0f) {
        p->distance_class = 1;
    } else if (p->distance < 10.0f) {
        p->distance_class = 2;
    } else if (p->distance < 30.0f) {
        p->distance_class = 3;
    } else {
        p->distance_class = 4;
    }

    {
        real_vector3d aim = unit->aiming_vector;
        float cos_angle = -(aim.i * p->direction.x + aim.k * p->direction.z + aim.j * p->direction.y);
        float lateral;

        if (cos_angle < 0.0f) {
            lateral = 3.4028235e+38f;
        } else if (cos_angle < 1.0f) {
            lateral = sqrtf_(1.0f - cos_angle * cos_angle) * p->distance;
        } else {
            lateral = 0.0f;
        }

        if (0.9925f < cos_angle || lateral < 0.5f) {
            p->aiming_at_actor_class = 0;
        } else if (0.9063f < cos_angle || lateral < 1.5f) {
            p->aiming_at_actor_class = 1;
        } else if (cos_angle <= 0.5f) {
            p->aiming_at_actor_class = (cos_angle <= 0.0f) ? 4 : 3;
        } else {
            p->aiming_at_actor_class = 2;
        }
    }

    p->shooting = (p->stimulus_type == 1);

    if (p->state < 4 || 5 < p->state) {
        engage_flag = 0;

        {
            int16_t kind_flag = (!p->is_parented || !p->enemy) ? 0 : 2;
            p->obstruction = (int16_t)actor_evaluate_engagement_reachability(
                *(int16_t *)((uint8_t *)scratch + 0x28), p->cluster_index, (real_point3d *)&p->head_position_x,
                (real_point3d *)scratch, kind_flag, 0, p->relationship_object_index,
                self->active_unit_index != (datum_index)k_datum_index_none);
        }
        p->perception_range_class = 2;

        if (unit_obj->type == _object_type_biped) {
            void *own_tag_data = halo::cache::globals().tag_instances[unit_obj->definition_tag & 0xffff].data;
            p->flying = (uint8_t)((*(uint32_t *)((uint8_t *)own_tag_data + 0x2f4)) >> 2) & 1;
        } else {
            p->flying = 0;
        }
        p->camouflaged = (0.5f < unit->active_camouflage_power);
        p->flashlight_on = (uint8_t)(unit->flags >> 0x13) & 1;

        {
            uint8_t frozen = (unit_obj->vitality_flags & _object_health_frozen_bit) != 0;
            uint8_t not_feigning = frozen && unit->feign_death_ticks == 0;

            p->just_died = (frozen && p->dead == 0) ? 1 : 0;
            p->dead = frozen;
            p->dead_not_feigning = not_feigning;

            if (p->just_died != 0 && !p->enemy && self->awareness_level < 3) {
                engage_flag = 1;
            }
            if (frozen) {
                p->retain_timer = 0;
            }
        }

        {
            uint32_t new_owner = unit->swarm_actor_index;
            uint8_t owner_is_none = (new_owner == (uint32_t)k_datum_index_none);
            if (owner_is_none) {
                new_owner = unit->actor_index;
            }
            if (new_owner != (uint32_t)p->owner_actor_index) {
                p->swarm_owned = !owner_is_none;
                p->owner_actor_index = new_owner;
                if (p->pair_index != (datum_index)k_datum_index_none) {
                    prop *paired = &((prop *)prop_data->data)[p->pair_index & 0xffff];
                    paired->owner_actor_index = new_owner;
                    paired->swarm_owned = p->swarm_owned;
                }
            }

            if (new_owner == (uint32_t)k_datum_index_none) {
                reachable = (p->dead == 0);
                owner_not_fully_aware = 0;
                owner_stalled = 0;
            } else {
                actor *owner = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                owner_not_fully_aware = (owner->awareness_level < 3);
                owner_stalled = (owner->awareness_level == 3 && owner->minimum_combat_status < owner->combat_status);
                reachable = actor_check_burst_length_exceeded(p->owner_actor_index);
                if (owner_stalled && p->owner_stalled == 0 && !p->enemy && self->awareness_level < 3) {
                    engage_flag = 1;
                }
            }
            p->unknown_12d = reachable;
            p->owner_not_in_combat = owner_not_fully_aware;
            p->owner_stalled = owner_stalled;
        }

        if (engage_flag) {
            int16_t result = 0;
            if (!team_gate) {
                uint8_t rate_flag = p->flashlight_on ? 2 : p->perception_range_class;
                uint16_t priority_class = actor_target_get_priority_class(actor_index, target_prop_index);
                result = actor_dispatch_look_handler_by_posture(p->obstruction, actor_index, scratch, (void *)((uint8_t *)p + 0x104),
                                       rate_flag, 1, priority_class);
                if (result > 1) goto after_engage;
            }
            p->perception_level = result;
            p->visual_perception = result;
            p->obstruction = 0;
        }
after_engage:
        if (!p->disregarded) {
            uint8_t did_track = 0;
            if (!p->camouflaged) {
                if (team_gate) {
                    p->visual_perception = 0;
                    p->just_sighted = 0;
                    did_track = 1;
                }
            } else if (p->enemy || (p->is_parented && 4.0f < p->distance)) {
                p->visual_perception = 0;
                p->just_sighted = 0;
                did_track = 1;
            }
            if (!did_track) {
                uint8_t use_urgent = 1;
                if (self->vehicle_driving_type == 4 || self->type == 0xf) {
                    use_urgent = 0;
                } else if (!p->enemy) {
                    use_urgent = 0;
                    if (self->awareness_level < 3 && (p->dead || p->owner_stalled != 0)) {
                        use_urgent = 1;
                    }
                } else if (2 <= p->state && p->state < 4) {
                    use_urgent = 0;
                }
                {
                    uint8_t rate_flag = p->flashlight_on ? 2 : p->perception_range_class;
                    uint16_t priority_class = actor_target_get_priority_class(actor_index, target_prop_index);
                    int16_t result = actor_dispatch_look_handler_by_posture(p->obstruction, actor_index, scratch, (void *)((uint8_t *)p + 0x104),
                                                   rate_flag, use_urgent, priority_class);
                    p->just_sighted = (p->visual_perception == 0 && result > 0);
                    p->visual_perception = result;
                    if (result != 0) {
                        p->last_seen_position_x = ((struct prop *)p)->head_position_x;
                        p->last_seen_position_y = ((struct prop *)p)->head_position_y;
                        p->last_seen_position_z = ((struct prop *)p)->head_position_z;
                        p->last_seen_time = tick;
                    }
                }
            }
            if (enc == (encounter *)0 || enc->deaf == 0) {
                if (p->stimulus_type == 1 || p->stimulus_type == 2) {
                    *(int16_t *)&((struct prop *)p)->auditory_perception = 3;
                } else {
                    *(int16_t *)&((struct prop *)p)->auditory_perception =
                        actor_target_hearing_check((uint8_t *)p + 0xfc, p->obstruction, actor_index,
                                                    scratch,  0, &p->last_known_position);
                }
            } else {
                *(int16_t *)&((struct prop *)p)->auditory_perception = 0;
            }
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            if (p->stimulus_type == 0) {
                *(int16_t *)((uint8_t *)p + 0x36) = 3;
            }
            if (p->flashlight_on != 0 && p->aiming_at_actor_class < 3 && p->distance_class < 3 &&
                (p->obstruction == 0 || p->obstruction == 1)) {
                int16_t v = *(int16_t *)((uint8_t *)p + 0x36);
                if (v < 2) v = 1;
                *(int16_t *)((uint8_t *)p + 0x36) = v;
            }
            {
                int16_t a = *(int16_t *)&((struct prop *)p)->auditory_perception;
                int16_t b = *(int16_t *)((uint8_t *)p + 0x36);
                int16_t best = (a <= b) ? b : a;
                int16_t chosen = p->visual_perception;
                if (chosen <= best) {
                    chosen = best;
                }
                p->perception_level = chosen;
                if (chosen == 1 && 2 <= p->state && p->state < 4) {
                    p->perception_level = 2;
                }
            }
        } else {
            p->perception_level = 0;
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            *(int16_t *)&((struct prop *)p)->auditory_perception = 0;
            p->visual_perception = 0;
        }

        if (p->perception_level != 0) {
            p->last_perceived_position = p->last_known_position;
            p->last_perceived_time = tick;
        }

        if (2 <= p->state && p->state < 4 &&
            (1 < p->visual_perception ||
             (p->has_current_information != 0 &&p->information_source_actor != -1 &&
              ((actor *)halo::memory::datum_get(p->information_source_actor, actor_data)) != (actor *)0 &&
              9 < ((actor *)halo::memory::datum_get(p->information_source_actor, actor_data))->target_combat_status &&
              ((actor *)halo::memory::datum_get(p->information_source_actor, actor_data))->target_unit_index != (datum_index)k_datum_index_none &&
              ((actor *)halo::memory::datum_get(p->information_source_actor, actor_data))->wants_to_fire != 0 &&
              (((prop *)prop_data->data)[((actor *)halo::memory::datum_get(p->information_source_actor, actor_data))->target_unit_index & 0xffff]).object_index == p->object_index))) {
            p->has_current_information = 1;
            p->information_age = 0;
        }
    } else {
        int16_t kind_flag = (!p->is_parented || !p->enemy) ? 0 : 2;
        p->obstruction = (int16_t)actor_evaluate_engagement_reachability(
            *(int16_t *)((uint8_t *)scratch + 0x28), p->cluster_index, (real_point3d *)&p->head_position_x,
            (real_point3d *)scratch, kind_flag, 0, p->relationship_object_index,
            self->active_unit_index != (datum_index)k_datum_index_none);
        if (p->disregarded || team_gate) {
            p->perception_level = 0;
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            *(int16_t *)&((struct prop *)p)->auditory_perception = 0;
            p->visual_perception = 0;
        } else {
            int16_t result = actor_dispatch_look_handler_by_posture(p->obstruction, actor_index, scratch, (void *)((uint8_t *)p + 0x104),
                                           p->perception_range_class, 1, 2);
            p->visual_perception = result;
            *(int16_t *)&((struct prop *)p)->auditory_perception = 0;
            *(int16_t *)((uint8_t *)p + 0x36) = 0;
            p->perception_level = result;
        }
    }

    if (p->is_vehicle_driver != 0) {
        actor_danger_register_stationary_object((const float *)scratch, actor_index,
                                                 p->relationship_object_index, 1 < p->perception_level);
    }

    if (0.0f < p->danger_radius &&
        (p->dead || *((uint8_t *)actor_def + 0x2a3) == 0x1e)) {
        actor_danger_register_point(actor_index, p->object_index, p->danger_radius, p->distance,
                                     p->enemy, 1 < p->perception_level);
    }

    if (p->enemy && 2 <= p->state && p->state < 4 &&
        ((actor_has_unshielded_threat_weapon(actor_index) != 0 && p->distance < self->maximum_firing_distance) ||
         ((actor_def->flags & 0x08000000u) != 0 && p->distance < actor_def->melee_fudge_factor))) {
        actor_target_mark_engaged(target_prop_index, actor_index, 0);
    }

    if (p->last_engaged_time != -1 && p->last_engaged_time + 0x96 < tick) {
        actor_target_mark_engaged(target_prop_index, actor_index, 0);
    }

    if (p->just_created != 0) {
        float dist_sq = p->distance * p->distance;
        actor *owner = (p->owner_actor_index == (datum_index)k_datum_index_none)
                           ? (actor *)0
                           : &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
        uint8_t drop = 0;

        if (p->is_parented) {
            drop = 0;
        } else if (owner != (actor *)0 && !(owner->active != 0 && owner->keep_unit_alive == 0)) {
            drop = 1;
        } else if (p->in_use != 0) {
            drop = 0;
        } else if (1600.0f < dist_sq) {
            drop = 1;
        } else if (!p->dead) {
            if (!p->enemy) {
                if (225.0f <= dist_sq) drop = 1;
            }
        } else {
            uint32_t enc_idx = self->encounter_index;
            uint8_t ok = 1;
            if (enc_idx != (uint32_t)k_datum_index_none) {
                encounter *e = &((encounter *)encounter_data->data)[enc_idx & 0xffff];
                int32_t gate = (e->last_idle_time <= self->found_body_time) ? self->found_body_time : e->last_idle_time;
                object_header *ohdr = (object_header *)object_data->data + (p->object_index & 0xffff);
                unit_data *u2 = (unit_data *)((uint8_t *)ohdr->data + k_unit_data_offset);
                int32_t last_seen = u2->death_time;
                ok = (gate == -1 || (last_seen != -1 && gate <= last_seen));
                if (!ok) {
                    drop = 1;
                } else if (e->engaged == 0 && e->has_live_target == 0 && e->stood_down == 0) {
                    if (225.0f <= dist_sq) drop = 1;
                }
            } else if (p->danger_radius <= 0.0f) {
                if (!p->enemy || p->dead_ticks < 0x97) {
                    int16_t grade = actor_get_current_mode_combat_grade(actor_index);
                    if (grade < 2) {
                        float threshold = 16.0f;
                        if (!p->enemy && self->awareness_level < 3) threshold = 64.0f;
                        if (dist_sq >= threshold) drop = 1;
                    } else {
                        drop = 1;
                    }
                } else {
                    drop = 1;
                }
            }
        }

        if (drop) {
            p->retain_timer = 0;
        }
    }
    p->just_created = 0;

    p->engaged = actor_target_update_active_flag(actor_index, target_prop_index);
    p->desirability = actor_rate_potential_target(actor_index, target_prop_index);
    p->interest = actor_compute_target_priority_weight(target_prop_index, actor_index);
    p->combat_dirty = 1;
}

namespace actor_targets_share_descriptor_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

/**
 * Actor AI behaviour: targets share descriptor.
 *
 * @address 0x40e380
 */
uint8_t ActorOps::targets_share_descriptor(datum_index actor_a, datum_index actor_b)
{
    using namespace actor_targets_share_descriptor_local;
    actor *self_a, *self_b;
    int16_t *desc_a, *desc_b;

    self_a = (actor *)((uint8_t *)actor_data->data + (actor_a & 0xffff) * sizeof(actor));
    self_b = (actor *)((uint8_t *)actor_data->data + (actor_b & 0xffff) * sizeof(actor));

    desc_a = (self_a->mode == 7 || self_a->mode == 5) ? (int16_t *)&self_a->mode_data.raw[8] : (int16_t *)0;
    desc_b = (self_b->mode == 7 || self_b->mode == 5) ? (int16_t *)&self_b->mode_data.raw[8] : (int16_t *)0;

    if (desc_a == (int16_t *)0 || desc_b == (int16_t *)0) {
        return 0;
    }

    if (desc_a[0] == 0 && desc_b[0] == 0) {
        datum_index datum_a = self_a->target_unit_index;
        datum_index datum_b = self_b->target_unit_index;
        prop *prop_a = (prop *)halo::memory::datum_get(datum_a, prop_data);
        prop *prop_b = (prop *)halo::memory::datum_get(datum_b, prop_data);
        if (prop_a == (prop *)0 || prop_b == (prop *)0) return 0;
        if (0.48999998f <= halo::math::vector3d_distance_squared(prop_b->last_known_position, prop_a->last_known_position)) return 0;
    } else if (desc_a[0] == 1 && desc_b[0] == 1) {
        return desc_a[1] == desc_b[1];
    } else if (desc_a[0] != 2 || desc_b[0] != 2) {
        return 0;
    }
    return 1;
}

}
