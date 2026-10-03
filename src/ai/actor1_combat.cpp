#include "halo/ai/actor_combat.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

namespace c_actor_check_burst_length_exceeded {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" uint8_t actor_check_burst_length_exceeded(datum_index actor_index);

/**
 * actor_check_burst_length_exceeded: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_burst_length_exceeded.c.txt.
 *
 * @address 0x4281b0
 */
uint8_t halo::ai::combat_ops::check_burst_length_exceeded()
{
    using namespace c_actor_check_burst_length_exceeded;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t result = self->combat_status > 6;

    if (result && self->mode == _actor_mode_death && *(int16_t *)&self->mode_data.raw[0xc] > 0) {
        result = 0;
    }
    return result;
}

extern "C" uint8_t actor_check_burst_length_exceeded(datum_index actor_index)
{
    return halo::ai::combat_ops(actor_index).check_burst_length_exceeded();
}

namespace c_actor_check_melee_target_reachable {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern ScenarioStructureBSP *global_structure_bsp;

extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override);
extern uint32_t actor_find_best_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok);
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok);
extern void actor_target_get_relationship_object(datum_index target_prop_index);
extern uint8_t path_find_run(path_find_context *context);
extern uint8_t path_find_find_unobstructed_ancestor(path_find_context *context, uint32_t vertex_id, real_point3d *point,
    uint8_t *out_used_start, real_point3d *out_position);
}
}

extern "C" void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order);

/**
 * actor_check_melee_target_reachable: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_melee_target_reachable.c.txt.
 *
 * @address 0x403f00
 */
void halo::ai::combat_ops::check_melee_target_reachable(int16_t *order)
{
    using namespace c_actor_check_melee_target_reachable;
    uint32_t actor_index = datum;
    uint8_t *record = (uint8_t *)order;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * 0x724;
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((struct actor *)actor)->actor_definition_tag & halo::k_slot_mask].data;
    static actor_firing_position_query query;
    static path_find_context path_context;
    actor_firing_position_candidate candidate;
    uint32_t previous_owner = halo::k_dword_none;
    uint8_t path_ok = 0;
    int16_t found;
    int16_t claimed;

    memset(&query, 0, sizeof(query));
    memset(&candidate, 0, sizeof(candidate));
    query.use_last_seen_position = record[4];
    if (*(int16_t *)(record + 0xc) > 0) {
        query.goal_kind = 1;
        if (*(int16_t *)record > 0) {
            query.collect_all = 1;
            query.allow_random_fallback = 1;
        }
        *((uint8_t *)&query + 0x36) = 1;
        *(float *)((uint8_t *)&query + 0x38) = 10.0f;
        *(float *)((uint8_t *)&query + 0x3c) = 6.0f;
    } else {
        query.goal_kind = 2;
        *((uint8_t *)&query + 0x8) = record[5];
        query.search_radius = ((Actor *)actor_tag)->max_seek_cover_distance > 0.0f ? ((Actor *)actor_tag)->max_seek_cover_distance : 6.0f;
    }
    query.group_mask = actor_get_firing_position_group_mask(actor_index, query.goal_kind, 0);
    found = (int16_t)actor_find_best_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
        &path_ok);
    *(int16_t *)(record + 8) = found;
    claimed = actor_claim_firing_position(actor_index, previous_owner, &path_context, found, path_ok);
    *(int16_t *)(record + 8) = claimed;
    record[0xa] = (claimed != -1 && path_ok == 0) ? 1 : 0;
    record[0x20] = 0;

    if (claimed != -1 && *(datum_index *)(record + 0x1c) != k_datum_index_none) {
        datum_index prop_index = *(datum_index *)(record + 0x1c);
        uint8_t *target = (uint8_t *)prop_data->data + (prop_index & halo::k_slot_mask) * 0x138;
        uint32_t ignore_object;
        uint32_t request[0x12];
        uint8_t *goal = (uint8_t *)candidate.position;
        static path_find_context target_context;

        if (((prop *)target)->state >= 2 && ((prop *)target)->state <= 3) {
            actor_target_get_relationship_object(prop_index);
        }
        ignore_object = *(uint32_t *)&((prop *)target)->relationship_object_index;
        if (ignore_object == halo::k_dword_none) {
            ignore_object = *(uint32_t *)&((prop *)target)->object_index;
        }
        actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * 0x724;
        memset(request, 0, sizeof(request));
        request[0] = *(uint32_t *)&((Actor *)actor_tag)->pathfinding_radius;
        ((uint8_t *)request)[4] = 0;
        request[2] = ignore_object;
        request[3] = *(uint32_t *)&((struct actor *)actor)->unit_index;
        ((uint8_t *)request)[0x10] = 1;
        *(real_point3d *)&request[5] = *(real_point3d *)&((prop *)target)->pathfinding_point.x;
        request[8] = *(uint32_t *)&((prop *)target)->pathfinding_surface_index;
        memset(&target_context, 0, sizeof(target_context));
        target_context.structure_bsp = (uint32_t)global_structure_bsp;
        memcpy(&target_context, request, sizeof(request));
        target_context.obstacle_cache = 0;
        target_context.have_goal = 1;
        target_context.goal_position = *(real_point3d *)goal;
        target_context.goal_vertex_id = *(uint32_t *)(goal + 0x14);
        *(uint32_t *)((uint8_t *)&target_context + 0x60) = 0;
        if (path_find_run(&target_context)) {
            uint8_t used_start = 0;

            record[0x20] = path_find_find_unobstructed_ancestor(&target_context, *(uint32_t *)(goal + 0x14),
                (real_point3d *)goal, &used_start, (real_point3d *)(record + 0x24));
        }
    }
    record[6] = 0;
}

extern "C" void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order)
{
    halo::ai::combat_ops(actor_index).check_melee_target_reachable(order);
}

namespace c_actor_check_vehicle_target_available {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;

extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
}
}

extern "C" uint8_t actor_check_vehicle_target_available(datum_index vehicle_object_index, datum_index actor_index, uint8_t flag_pursue);

/**
 * actor_check_vehicle_target_available: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_vehicle_target_available.c.txt.
 *
 * @address 0x42b810
 */
uint8_t halo::ai::combat_ops::check_vehicle_target_available(datum_index vehicle_object_index, datum_index actor_index, uint8_t flag_pursue)
{
    using namespace c_actor_check_vehicle_target_available;
    object *vehicle_object;
    unit_data *vehicle_unit;
    actor *self;

    if (vehicle_object_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    vehicle_object = ((object_header *)object_data->data)[vehicle_object_index & halo::k_slot_mask].data;
    vehicle_unit = (unit_data *)((uint8_t *)vehicle_object + k_unit_data_offset);
    if (vehicle_unit->controlling_player == (datum_index)k_datum_index_none) {
        return 0;
    }
    self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];

    if (teams_are_enemies(((struct actor *)self)->team, ((struct object *)vehicle_object)->owner_team) != 0) {
        return 0;
    }
    if (flag_pursue) {
        self->vehicle_eviction = 1;
    }
    return 1;
}

extern "C" uint8_t actor_check_vehicle_target_available(datum_index vehicle_object_index, datum_index actor_index, uint8_t flag_pursue)
{
    return halo::ai::combat_ops::check_vehicle_target_available(vehicle_object_index, actor_index, flag_pursue);
}

namespace c_actor_check_weapon_pickup_reachable {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern Scenario *global_scenario;

extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point,
    uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator);
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying);
}
}

extern "C" uint8_t actor_check_weapon_pickup_reachable(uint32_t actor_index, uint8_t *record);

/**
 * actor_check_weapon_pickup_reachable: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_weapon_pickup_reachable.c.txt.
 *
 * @address 0x4041d0
 */
uint8_t halo::ai::combat_ops::check_weapon_pickup_reachable(uint8_t *record)
{
    using namespace c_actor_check_weapon_pickup_reachable;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    datum_index target_prop_index = *(datum_index *)(record + 0x1c);
    int16_t firing_position_index = *(int16_t *)(record + 8);
    uint8_t result = 0;

    if (target_prop_index == (datum_index)k_datum_index_none || a->encounter_index == (datum_index)k_datum_index_none || firing_position_index == -1) {
        return 0;
    }

    {
        prop *p = &((prop *)prop_data->data)[target_prop_index & halo::k_slot_mask];
        ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
        ScenarioFiringPosition *positions = (ScenarioFiringPosition *)encounters[a->encounter_index & halo::k_slot_mask].firing_positions.pointer;
        int16_t status;
        real_point3d self_position;

        unit_add_marker_relative_offset(a->unit_index, 2, (float *)&positions[firing_position_index], 0, 0, &self_position);
        status = (int16_t)actor_evaluate_engagement_reachability(
            *(int16_t *)((uint8_t *)&positions[firing_position_index] + 0xe), p->cluster_index,
            (real_point3d *)&p->head_position_x, &self_position, 1, 0, p->relationship_object_index,
            a->active_unit_index != (datum_index)k_datum_index_none);

        if (p->state > 1 && p->state < 4) {
            result = (status == 0);
        }
        if (status == 0 || status == 3) {
            record[0x20] = 1;
            *(real_point3d *)(record + 0x24) = p->last_known_position;
        }
    }
    return result;
}

extern "C" uint8_t actor_check_weapon_pickup_reachable(uint32_t actor_index, uint8_t *record)
{
    return halo::ai::combat_ops(actor_index).check_weapon_pickup_reachable(record);
}

namespace c_actor_choose_best_target {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;

extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);
extern void actor_update_target_combat_status(datum_index actor_index);
extern void actor_update_awareness_level(datum_index actor_index);
}
}

extern "C" void actor_choose_best_target(datum_index actor_index);

/**
 * actor_choose_best_target: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_choose_best_target.c.txt.
 *
 * @address 0x4203a0
 */
void halo::ai::combat_ops::choose_best_target()
{
    using namespace c_actor_choose_best_target;
    datum_index actor_index = datum;
    actor *self;
    actor *target_actor;
    prop *p;
    object *tracked_object;
    datum_index prop_index;
    datum_index best;
    datum_index previous;
    datum_index owning_actor_index;
    float best_desirability;
    uint32_t *clear;
    int32_t i;
    int16_t actor_type_slot;
    int16_t threat_class;
    uint8_t in_group_a;
    uint8_t in_group_b;
    uint8_t in_group_c;
    uint8_t low_priority_kind;
    uint8_t suppress_close_bonus;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    best_desirability = 0.0f;
    best = (datum_index)k_datum_index_none;

    suppress_close_bonus = 0;
    if (self->berserking != 0 || self->mode == 10) {
        suppress_close_bonus = 1;
    }

    clear = (uint32_t *)&self->tally;
    for (i = 0x1e; i != 0; i--) {
        *clear = 0;
        clear++;
    }
    *(int16_t *)clear = 0;
    *((uint8_t *)clear + 2) = 0;

    for (prop_index = self->first_prop; prop_index != (datum_index)k_datum_index_none;
         prop_index = p->next_in_actor) {
        p = &((prop *)prop_data->data)[prop_index & halo::k_slot_mask];

        if (p->state > 1 && p->state < 4 && p->dead == 0) {
            if (p->enemy == 0) {
                tracked_object = ((object_header *)object_data->data)[p->object_index & halo::k_slot_mask].data;
                owning_actor_index = *(datum_index *)((uint8_t *)tracked_object + 0x1f4);
                target_actor = (actor *)0;
                if (owning_actor_index != (datum_index)k_datum_index_none) {
                    target_actor = (actor *)((uint8_t *)actor_data->data +
                                             (owning_actor_index & halo::k_slot_mask) * sizeof(actor));
                }

                if (*(int32_t *)((uint8_t *)tracked_object + 0x218) != -1) {
                    actor_type_slot = 6;
                } else if (target_actor == (actor *)0) {
                    actor_type_slot = 14;
                } else {
                    actor_type_slot = target_actor->type;
                }

                in_group_a = 0;
                in_group_b = 0;
                if (p->distance >= 8.0f) {
                    if (p->owner_burst_length_exceeded != 0 && target_actor != (actor *)0 &&
                        self->target_unit_index != (datum_index)k_datum_index_none &&
                        target_actor->target_unit_index != (datum_index)k_datum_index_none &&
                        ((prop *)prop_data->data)[self->target_unit_index & halo::k_slot_mask].object_index ==
                        ((prop *)prop_data->data)[target_actor->target_unit_index & halo::k_slot_mask].object_index) {
                        in_group_a = 1;
                    }
                } else {
                    in_group_a = 1;
                }

                in_group_c = 0;
                if (p->obstruction == 0 || p->obstruction == 1) {
                    in_group_b = 1;
                    in_group_c = (uint8_t)(p->distance < 3.0f);
                }

                if (in_group_a) {
                    self->tally.group_a_total = (uint8_t)(self->tally.group_a_total + 1);
                    if (p->owner_burst_length_exceeded != 0) {
                        self->tally.group_a_marked = (uint8_t)(self->tally.group_a_marked + 1);
                        if (p->owner_burst_length_exceeded != 0 && p->is_vehicle_gunner != 0) {
                            self->tally.group_a_marked_135 =
                                (uint8_t)(self->tally.group_a_marked_135 + 1);
                        }
                    }
                    self->tally.group_a_by_actor_type[actor_type_slot] =
                        (uint8_t)(self->tally.group_a_by_actor_type[actor_type_slot] + 1);
                    if (p->owner_burst_length_exceeded != 0) {
                        self->tally.group_a_marked_by_actor_type[actor_type_slot] =
                            (uint8_t)(self->tally.group_a_marked_by_actor_type[actor_type_slot] + 1);
                    }
                }
                if (in_group_b) {
                    self->tally.group_b_total = (uint8_t)(self->tally.group_b_total + 1);
                    if (p->owner_burst_length_exceeded != 0) {
                        self->tally.group_b_marked = (uint8_t)(self->tally.group_b_marked + 1);
                    }
                    self->tally.group_b_by_actor_type[actor_type_slot] =
                        (uint8_t)(self->tally.group_b_by_actor_type[actor_type_slot] + 1);
                    if (p->owner_burst_length_exceeded != 0) {
                        self->tally.group_b_marked_by_actor_type[actor_type_slot] =
                            (uint8_t)(self->tally.group_b_marked_by_actor_type[actor_type_slot] + 1);
                    }
                }
                if (in_group_c) {
                    self->tally.group_c_total = (uint8_t)(self->tally.group_c_total + 1);
                    if (p->owner_burst_length_exceeded != 0) {
                        self->tally.group_c_marked = (uint8_t)(self->tally.group_c_marked + 1);
                    }
                    self->tally.group_c_by_actor_type[actor_type_slot] =
                        (uint8_t)(self->tally.group_c_by_actor_type[actor_type_slot] + 1);
                    if (p->owner_burst_length_exceeded != 0) {
                        self->tally.group_c_marked_by_actor_type[actor_type_slot] =
                            (uint8_t)(self->tally.group_c_marked_by_actor_type[actor_type_slot] + 1);
                    }
                }
            } else {
                low_priority_kind = (uint8_t)(p->visual_perception < 2);
                threat_class = 0;
                self->tally.unit_props = (uint8_t)(self->tally.unit_props + 1);

                if (low_priority_kind && (p->shooting == 0 || p->obstruction != 0)) {

                } else {
                    if (!low_priority_kind) {
                        if (p->engaged_ticks == 0) {
                            self->tally.unit_props_unseen =
                                (uint8_t)(self->tally.unit_props_unseen + 1);
                        }
                        self->tally.threat_class_ge_1 =
                            (uint8_t)(self->tally.threat_class_ge_1 + 1);
                        threat_class = 1;
                    }
                    if (p->seen != 0) {
                        self->tally.threat_class_8 = (uint8_t)(self->tally.threat_class_8 + 1);
                        if (threat_class < 9) {
                            threat_class = 8;
                        }
                    }
                    if (p->shooting != 0) {
                        self->tally.threat_class_4 = (uint8_t)(self->tally.threat_class_4 + 1);
                        if (threat_class < 5) {
                            threat_class = 4;
                        }
                    }
                    if ((int8_t)p->aiming_at_actor_class < 3) {
                        if (!low_priority_kind) {
                            self->tally.threat_class_2 = (uint8_t)(self->tally.threat_class_2 + 1);
                            if (threat_class < 3) {
                                threat_class = 2;
                            }
                            if (suppress_close_bonus == 0 && p->distance < 2.0f) {
                                self->tally.threat_class_7 =
                                    (uint8_t)(self->tally.threat_class_7 + 1);
                                if (threat_class < 8) {
                                    threat_class = 7;
                                }
                            }
                        }
                        if ((int8_t)p->aiming_at_actor_class < 2) {
                            if (p->shooting != 0) {
                                self->tally.threat_class_5 =
                                    (uint8_t)(self->tally.threat_class_5 + 1);
                                if (threat_class < 6) {
                                    threat_class = 5;
                                }
                            }
                            if ((int8_t)p->aiming_at_actor_class < 1) {
                                if (!low_priority_kind) {
                                    self->tally.threat_class_3 =
                                        (uint8_t)(self->tally.threat_class_3 + 1);
                                    if (threat_class < 4) {
                                        threat_class = 3;
                                    }
                                }
                                if (p->shooting != 0) {
                                    self->tally.threat_class_6 =
                                        (uint8_t)(self->tally.threat_class_6 + 1);
                                    if (threat_class < 7) {
                                        threat_class = 6;
                                    }
                                }
                            }
                        }
                    }
                }
                self->tally.by_threat_class[threat_class] =
                    (uint8_t)(self->tally.by_threat_class[threat_class] + 1);
            }
        }

        if (best_desirability < p->desirability) {
            best = prop_index;
            best_desirability = p->desirability;
        }
    }

    previous = self->target_unit_index;
    if (best != previous) {
        self->target_combat_status = 0;
        self->target_unit_index = best;
        self->target_last_seen_time = (datum_index)k_datum_index_none;
        if (previous != (datum_index)k_datum_index_none) {
            ((prop *)prop_data->data)[previous & halo::k_slot_mask].desirability =
                actor_rate_potential_target(actor_index, previous);
        }
        if (best != (datum_index)k_datum_index_none) {
            ((prop *)prop_data->data)[best & halo::k_slot_mask].desirability =
                actor_rate_potential_target(actor_index, self->target_unit_index);
        }
    }
    actor_update_target_combat_status(actor_index);
    actor_update_awareness_level(actor_index);
}

extern "C" void actor_choose_best_target(datum_index actor_index)
{
    halo::ai::combat_ops(actor_index).choose_best_target();
}

namespace c_actor_choose_random_point_near {
extern "C" {
extern double fcos(double x);
extern double fsin(double x);
}
}

extern "C" void actor_choose_random_point_near(real_point3d *inout_point, float radius);

/**
 * actor_choose_random_point_near: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_choose_random_point_near.c.txt.
 *
 * @address 0x40faf0
 */
void halo::ai::combat_ops::choose_random_point_near(real_point3d *inout_point, float radius)
{
    using namespace c_actor_choose_random_point_near;
    real_point3d base;
    real_point3d chosen;
    real_vector3d delta;
    float cos_angle, sin_angle;
    collision_result line_result;
    float clear_fraction;

    base.x = halo::math::globals().global_up3d_pointer->i * 1.5f + inout_point->x;
    base.y = halo::math::globals().global_up3d_pointer->j * 1.5f + inout_point->y;
    base.z = halo::math::globals().global_up3d_pointer->k * 1.5f + inout_point->z;

    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    {
        uint32_t roll = halo::math::globals().random_seed_global >> 0x10;
        double angle = (double)roll * 1.5259022e-05 * 6.2831855 - 3.1415927;
        cos_angle = (float)fcos(angle);
        sin_angle = (float)fsin(angle);
    }

    chosen.x = cos_angle * radius + base.x;
    chosen.y = sin_angle * radius + base.y;
    chosen.z = radius * 0.0f + base.z;

    delta.i = base.x - inout_point->x;
    delta.j = base.y - inout_point->y;
    delta.k = base.z - inout_point->z;

    if (halo::physics::collision_test_movement_segment(0x23, inout_point, &delta, (uint32_t)-1, &line_result) != 0) {
        base = *inout_point;
    }

    delta.i = chosen.x - base.x;
    delta.j = chosen.y - base.y;
    delta.k = chosen.z - base.z;

    if (halo::physics::collision_test_movement_segment(0x23, &base, &delta, (uint32_t)-1, &line_result) != 0) {
        clear_fraction = line_result.t * radius - 0.1f;
        if (clear_fraction < 0.0f) {
            clear_fraction = 0.0f;
        }
        chosen.x = cos_angle * clear_fraction + base.x;
        chosen.y = sin_angle * clear_fraction + base.y;
        chosen.z = clear_fraction * 0.0f + base.z;
    }

    *inout_point = chosen;
}

extern "C" void actor_choose_random_point_near(real_point3d *inout_point, float radius)
{
    halo::ai::combat_ops::choose_random_point_near(inout_point, radius);
}

namespace c_actor_clear_target_state {
extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern actor_mode_definition actor_mode_definitions[16];
}
}

extern "C" void actor_clear_target_state(datum_index actor_index);

/**
 * actor_clear_target_state: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_clear_target_state.c.txt.
 *
 * @address 0x4286c0
 */
void halo::ai::combat_ops::clear_target_state()
{
    using namespace c_actor_clear_target_state;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];

    *(int16_t *)((uint8_t *)self + 0x148) = -1;
    *(uint32_t *)((uint8_t *)self + 0x144) = halo::k_dword_none;
    self->pathfinding_surface_index = halo::k_dword_none;
    self->search_surface_index = halo::k_dword_none;

    if (self->queued_movement.type == 2) {
        self->queued_movement.parameter = halo::k_dword_none;
    }
    if (self->active_movement.type == 2) {
        self->active_movement.parameter = halo::k_dword_none;
    }

    self->destination_surface_index = halo::k_dword_none;

    if (self->swarm != 0 && self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & halo::k_slot_mask];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            swarm_component *component = &((swarm_component *)swarm_component_data->data)[s->component_index[i] & halo::k_slot_mask];
            component->marker_index = (datum_index)k_datum_index_none;
        }
    }

    {

        uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0x28);
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }
}

extern "C" void actor_clear_target_state(datum_index actor_index)
{
    halo::ai::combat_ops(actor_index).clear_target_state();
}

namespace c_actor_compute_accuracy_scale {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
}
}

extern "C" float actor_compute_accuracy_scale(datum_index actor_index);

/**
 * actor_compute_accuracy_scale: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_compute_accuracy_scale.c.txt.
 *
 * @address 0x429620
 */
float halo::ai::combat_ops::compute_accuracy_scale()
{
    using namespace c_actor_compute_accuracy_scale;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    float scale = 0.5f;

    if (self->active_unit_index != (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)object_data->data)[self->active_unit_index & halo::k_slot_mask].data;
        void *tag_data = halo::cache::globals().tag_instances[unit_object->definition_tag & halo::k_slot_mask].data;
        scale = *(float *)((uint8_t *)tag_data + 0x384);
    }

    if (self->mode == 11 && self->mode_data.raw[0x54] != 0) {
        scale = *(float *)&self->mode_data.raw[0x58];
    }
    if (self->mode == 9) {
        return 0.7f;
    }
    if (!(scale > 0.2f)) {
        scale = 0.2f;
    }
    return scale;
}

extern "C" float actor_compute_accuracy_scale(datum_index actor_index)
{
    return halo::ai::combat_ops(actor_index).compute_accuracy_scale();
}

namespace c_actor_compute_target_priority_weight {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
}

extern "C" float actor_compute_target_priority_weight(datum_index prop_index, datum_index actor_index);

/**
 * actor_compute_target_priority_weight: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_compute_target_priority_weight.c.txt.
 *
 * @address 0x414590
 */
float halo::ai::combat_ops::compute_target_priority_weight(datum_index prop_index, datum_index actor_index)
{
    using namespace c_actor_compute_target_priority_weight;
    prop *p;
    actor *self;
    float weight;
    float occupancy_bonus;
    float relationship_scale;
    datum_index active_unit;

    p = &((prop *)prop_data->data)[prop_index & halo::k_slot_mask];
    weight = 0.0f;

    if (p->state < 2 || 3 < p->state) {
        if (p->enemy != 0 && 3 < p->state && p->state < 6) {
            weight = 1.5f;
        }
    } else if (p->dead == 0) {
        weight = (p->enemy == 0) ? 1.0f : 2.0f;
    } else if (p->dead_ticks < 0xd2) {
        weight = 1.8f;
    } else {
        weight = 0.4f;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    active_unit = self->active_unit_index;
    if (p->object_index == active_unit || (datum_index)p->relationship_object_index == active_unit) {
        weight = 0.0f;
    }

    relationship_scale = (p->relationship_object_index == -1) ? 1.0f : 1.5f;

    switch (p->speed_class) {
    case 1:
        occupancy_bonus = 0.5f * relationship_scale;
        weight += occupancy_bonus;
        break;
    case 2:
        occupancy_bonus = relationship_scale;
        weight += occupancy_bonus;
        break;
    case 3:
        occupancy_bonus = relationship_scale + relationship_scale;
        weight += occupancy_bonus;
        break;
    default:

        break;
    }

    if (p->shooting != 0) {
        weight = relationship_scale + relationship_scale + weight;
    }

    switch (p->distance_class) {
    case 1:
        weight *= 0.6f;
        break;
    case 3:
        return weight * 0.4f;
    case 4:
        return weight * 0.2f;
    default:
        break;
    }
    return weight;
}

extern "C" float actor_compute_target_priority_weight(datum_index prop_index, datum_index actor_index)
{
    return halo::ai::combat_ops::compute_target_priority_weight(prop_index, actor_index);
}

namespace c_actor_consider_target_candidate {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);
extern void actor_update_target_combat_status(datum_index actor_index);
extern void actor_update_awareness_level(datum_index actor_index);
}
}

extern "C" uint16_t actor_consider_target_candidate(datum_index actor_index, datum_index candidate_prop_index);

/**
 * actor_consider_target_candidate: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_consider_target_candidate.c.txt.
 *
 * @address 0x4208a0
 */
uint16_t halo::ai::combat_ops::consider_target_candidate(datum_index candidate_prop_index)
{
    using namespace c_actor_consider_target_candidate;
    datum_index actor_index = datum;
    actor *self;
    prop *candidate;
    prop *current_target;
    datum_index current_target_index;
    float score;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    candidate = (prop *)((uint8_t *)prop_data->data + (candidate_prop_index & halo::k_slot_mask) * sizeof(prop));

    current_target_index = self->target_unit_index;
    current_target = (current_target_index == k_datum_index_none) ? (prop *)0 :
                      (prop *)((uint8_t *)prop_data->data + (current_target_index & halo::k_slot_mask) * sizeof(prop));

    score = actor_rate_potential_target(actor_index, candidate_prop_index);
    candidate->desirability = score;

    if (0.0f < score) {
        if (current_target != (prop *)0 && score < current_target->desirability) {
            return 0;
        }
        self->target_combat_status = 0;
        self->target_unit_index = candidate_prop_index;
        self->target_last_seen_time = k_datum_index_none;
        actor_update_target_combat_status(actor_index);
        actor_update_awareness_level(actor_index);
        return 1;
    }
    return 0;
}

extern "C" uint16_t actor_consider_target_candidate(datum_index actor_index, datum_index candidate_prop_index)
{
    return halo::ai::combat_ops(actor_index).consider_target_candidate(candidate_prop_index);
}

namespace c_actor_evaluate_custom_charge_trigger {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *prop_data;

extern void * actor_get_actor_definition(datum_index actor_index);
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator);
}
}

extern "C" uint8_t actor_evaluate_custom_charge_trigger(datum_index actor_index);

/**
 * actor_evaluate_custom_charge_trigger: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_evaluate_custom_charge_trigger.c.txt.
 *
 * @address 0x424090
 */
uint8_t halo::ai::combat_ops::evaluate_custom_charge_trigger()
{
    using namespace c_actor_evaluate_custom_charge_trigger;
    datum_index actor_index = datum;
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * 0x724;
    const uint8_t *variant = (const uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((actor *)self)->actor_variant_tag & halo::k_slot_mask].data;
    const uint8_t *def = (const uint8_t *)actor_get_actor_definition(actor_index);
    uint32_t unit_index = *(uint32_t *)&((actor *)self)->unit_index;
    const uint8_t *unit;
    const uint8_t *target = 0;
    uint8_t decision;
    int16_t variant_mode;

    if (!unit_is_in_busy_animation_state(unit_index) && self[0x4a8] == 0) {
        goto return_true;
    }
    if (((actor *)self)->awareness_level < 3) {
        goto return_true;
    }
    if (((struct actor *)self)->combat_status < 5) {
        goto return_false;
    }
    unit = (const uint8_t *)((object_header *)object_data->data)[unit_index & halo::k_slot_mask].data;
    if (*(uint32_t *)&((actor *)self)->target_unit_index != halo::k_dword_none) {
        target = (const uint8_t *)prop_data->data + (*(uint32_t *)&((actor *)self)->target_unit_index & halo::k_slot_mask) * 0x138;
    }
    if (unit[0x2a3] == 0x17 && self[0x378] == 0) {
        goto return_true;
    }
    if (target != 0 && *(const float *)(target + 0x11c) > *(const float *)(def + 0x74)) {
        goto return_false;
    }
    if (self[0x378] != 0 && target != 0 &&
        *(const float *)(target + 0x11c) > *(const float *)(def + 0x16c)) {
        goto return_false;
    }
    if ((int8_t)unit[0x106] < 0) {
        goto return_true;
    }
    if (self[0x378] != 0) {
        goto return_false;
    }
    if (((actor *)self)->mode == 10 &&
        (*(int16_t *)(self + 0xa0) == 2 || *(int16_t *)(self + 0xa0) == 3)) {
        goto return_false;
    }
    if (!actor_has_unshielded_threat_weapon(actor_index)) {
        goto return_false;
    }
    if (self[0x15d] != 0) {
        goto return_false;
    }
    variant_mode = *(int16_t *)&((ActorVariant *)variant)->movement_type;
    if (variant_mode == 0) {
        goto return_false;
    }
    if (variant_mode == 1) {
        goto return_true;
    }
    if (target != 0 && !(*(const float *)(target + 0x11c) >= *(const float *)(def + 0xa0))) {
        goto return_true;
    }

    if (self[0x362] != 0) {
        if (*(int16_t *)(self + 0x366) > 0) {
            *(int16_t *)(self + 0x366) -= 1;
        } else if ((variant[0] & 8) != 0 && (int8_t)self[0x245] > 0) {

            const uint8_t *axis_prop = (const uint8_t *)prop_data->data +
                (*(uint32_t *)&((actor *)self)->target_unit_index & halo::k_slot_mask) * 0x138;
            actor_prop_iterator iterator;
            uint32_t cursor;
            int32_t ahead = 0, level = 0, behind = 0;

            actor_prop_iterator_init(actor_index, &iterator);
            cursor = iterator.next;
            while (cursor != halo::k_dword_none) {
                const uint8_t *p = (const uint8_t *)prop_data->data + (cursor & halo::k_slot_mask) * 0x138;
                int16_t kind = *(const int16_t *)(p + 0x24);
                uint32_t owner;
                const uint8_t *other;
                float dot;

                cursor = *(const uint32_t *)(p + 0x8);
                if (kind < 2 || kind > 3 || p[0x60] != 0 || p[0x127] != 0) {
                    continue;
                }
                if (*(const float *)(p + 0x11c) >= 15.0f) {
                    continue;
                }
                owner = *(const uint32_t *)(p + 0x1c);
                if (owner == halo::k_dword_none) {
                    continue;
                }
                other = (const uint8_t *)actor_data->data + (owner & halo::k_slot_mask) * 0x724;
                if (other[0x362] == 0) {
                    continue;
                }
                dot = (*(const float *)(other + 0x134) - ((actor *)self)->body_position.z) * *(const float *)(axis_prop + 0xe8) +
                      (*(const float *)(other + 0x130) - ((actor *)self)->body_position.y) * *(const float *)(axis_prop + 0xe4) +
                      (*(const float *)(other + 0x12c) - ((actor *)self)->body_position.x) * *(const float *)(axis_prop + 0xe0);
                if (dot > 1.4f) {
                    ahead++;
                } else if (dot >= -1.4f) {
                    level++;
                } else {
                    behind++;
                }
            }

            if (self[0x363] != 0) {
                if ((int16_t)behind == 0 && (int16_t)ahead > (int16_t)level) {
                    decision = 0;
                    goto apply_decision;
                }
            } else if ((int16_t)ahead == 0 && (int16_t)behind > (int16_t)level) {
                goto flip_decision;
            }
        }
        *(int16_t *)(self + 0x364) -= 1;
        if (*(int16_t *)(self + 0x364) != 0) {
            return self[0x363];
        }
    flip_decision:
        decision = (self[0x363] == 0);
    } else {
        float chance = ((ActorVariant *)variant)->initial_crouch_chance;
        float roll;

        if ((int8_t)self[0x200] > 0) {
            uint32_t cursor = *(uint32_t *)&((actor *)self)->first_prop;
            int16_t without = 0, with = 0;

            while (cursor != halo::k_dword_none) {
                const uint8_t *p = (const uint8_t *)prop_data->data + (cursor & halo::k_slot_mask) * 0x138;
                int16_t kind = *(const int16_t *)(p + 0x24);
                uint32_t owner;
                const uint8_t *other;

                cursor = *(const uint32_t *)(p + 0x8);
                if (kind < 2 || kind > 3 || p[0x60] != 0 || p[0x127] != 0) {
                    continue;
                }
                owner = *(const uint32_t *)(p + 0x1c);
                if (owner == halo::k_dword_none) {
                    continue;
                }
                other = (const uint8_t *)actor_data->data + (owner & halo::k_slot_mask) * 0x724;
                if (*(const int16_t *)(other + 0x4) != ((actor *)self)->type ||
                    *(const int16_t *)(other + 0x6e) < 5) {
                    continue;
                }
                if (other[0x358] != 0) {
                    with++;
                } else {
                    without++;
                }
            }
            chance = chance - ((1.0f - chance) * (float)(int32_t)with +
                               (-chance) * (float)(int32_t)without) * 0.5f;
        }

        halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
        self[0x362] = 1;
        roll = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f;
        decision = (roll >= chance) ? 0 : 1;
    }

apply_decision:
    {
        float ticks;

        self[0x363] = decision;
        if (decision != 0) {
            ticks = halo::math::random_real_range(*(const float *)(variant + 0x54), *(const float *)(variant + 0x58));
        } else {
            ticks = halo::math::random_real_range(*(const float *)(variant + 0x5c), *(const float *)(variant + 0x60));
        }
        ticks = ticks * 30.0f;
        if (!(ticks > 31.0f)) {
            ticks = 31.0f;
        }
        *(int16_t *)(self + 0x364) = (int16_t)(int32_t)ticks;
        *(int16_t *)(self + 0x366) = 0x1e;
    }
    return self[0x363];

return_true:
    self[0x362] = 0;
    return 1;

return_false:
    self[0x362] = 0;
    return 0;
}

extern "C" uint8_t actor_evaluate_custom_charge_trigger(datum_index actor_index)
{
    return halo::ai::combat_ops(actor_index).evaluate_custom_charge_trigger();
}

namespace c_actor_evaluate_flank_offset {
extern "C" {
extern double sqrt(double x);
static float sqrt_f(float x) { return (float)sqrt((double)x); }
extern double fabs(double x);
static float fabs_f(float x) { return (float)fabs((double)x); }
}
}

extern "C" int16_t actor_evaluate_flank_offset(real_vector3d *cover_direction, real_vector3d *out_offset, real_point3d *threat_position, real_point3d *candidate_position);

/**
 * actor_evaluate_flank_offset: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_evaluate_flank_offset.c.txt.
 *
 * @address 0x420b10
 */
int16_t halo::ai::combat_ops::evaluate_flank_offset(real_vector3d *cover_direction, real_vector3d *out_offset, real_point3d *threat_position, real_point3d *candidate_position)
{
    using namespace c_actor_evaluate_flank_offset;
    float cover_len;
    float dx, dy;
    float projection;
    float offset_x, offset_y, offset_z;
    float horizontal_sq;
    int16_t grade;

    cover_len = sqrt_f(cover_direction->j * cover_direction->j + cover_direction->i * cover_direction->i);

    if (0.0001f <= fabs_f(cover_len) && 0.0f < cover_len) {
        dx = threat_position->x - candidate_position->x;
        dy = threat_position->y - candidate_position->y;
        projection = dy * (1.0f / cover_len) * cover_direction->j + dx * cover_direction->i * (1.0f / cover_len);

        if (sqrt_f(dx * dx + dy * dy) * 0.8660254f < projection) {
            projection = -projection;
            offset_x = projection * cover_direction->i + dx;
            offset_y = projection * cover_direction->j + dy;
            offset_z = projection * cover_direction->k + (threat_position->z - candidate_position->z);

            if (out_offset != (real_vector3d *)0) {
                out_offset->i = -offset_x;
                out_offset->j = -offset_y;
                out_offset->k = -offset_z;
            }

            if (offset_z <= -0.5f || 0.9f <= offset_z) {
                if (offset_z <= -0.8f) {
                    return 0;
                }
                if (1.2f <= offset_z) {
                    return 0;
                }
                grade = 1;
            } else {
                grade = 2;
            }

            horizontal_sq = offset_x * offset_x + offset_y * offset_y;
            if (horizontal_sq < 0.36f) {
                return grade;
            }
            if (1.21f <= horizontal_sq) {
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

extern "C" int16_t actor_evaluate_flank_offset(real_vector3d *cover_direction, real_vector3d *out_offset, real_point3d *threat_position, real_point3d *candidate_position)
{
    return halo::ai::combat_ops::evaluate_flank_offset(cover_direction, out_offset, threat_position, candidate_position);
}

namespace c_actor_forward_target_object_reference {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern uint8_t actor_target_data_acquire(datum_index actor_index, datum_index object_index,
    datum_index owner_reference, datum_index pair_reference);
}
}

extern "C" void actor_forward_target_object_reference(datum_index actor_index, uint32_t param);

/**
 * actor_forward_target_object_reference: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_forward_target_object_reference.c.txt.
 *
 * @address 0x428420
 */
void halo::ai::combat_ops::forward_target_object_reference(uint32_t param)
{
    using namespace c_actor_forward_target_object_reference;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[self->target_unit_index & halo::k_slot_mask];

        actor_target_data_acquire(param, p->object_index, actor_index, self->target_unit_index);
    }
}

extern "C" void actor_forward_target_object_reference(datum_index actor_index, uint32_t param)
{
    halo::ai::combat_ops(actor_index).forward_target_object_reference(param);
}

namespace c_actor_get_aim_from_position {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction,
                                                          uint8_t use_aiming_bounds);
}
}

extern "C" void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3]);

/**
 * actor_get_aim_from_position: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_aim_from_position.c.txt.
 *
 * @address 0x40f9b0
 */
void halo::ai::combat_ops::get_aim_from_position(uint32_t out_position[3])
{
    using namespace c_actor_get_aim_from_position;
    datum_index actor_index = datum;
    actor *self;
    datum_index unit_index;
    object_header *hdr;
    object *unit_obj;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    unit_index = self->unit_index;

    if (self->vehicle_gunner != 0) {
        unit_index = self->active_unit_index;
        hdr = (object_header *)object_data->data + (unit_index & halo::k_slot_mask);
        unit_obj = hdr->data;
        if ((*(uint32_t *)((uint8_t *)halo::cache::globals().tag_instances[unit_obj->definition_tag & halo::k_slot_mask].data + 0x2f0) & 0x100) != 0) {
            out_position[0] = *(uint32_t *)&unit_obj->forward.i;
            out_position[1] = *(uint32_t *)&unit_obj->forward.j;
            out_position[2] = *(uint32_t *)&unit_obj->forward.k;
            return;
        }
    }

    hdr = (object_header *)object_data->data + (unit_index & halo::k_slot_mask);
    unit_obj = hdr->data;
    out_position[0] = *(uint32_t *)((uint8_t *)unit_obj + 0x23c);
    out_position[1] = *(uint32_t *)((uint8_t *)unit_obj + 0x240);
    out_position[2] = *(uint32_t *)((uint8_t *)unit_obj + 0x244);

    unit_clamp_direction_to_aim_or_look_bounds(unit_index, (real_vector3d *)out_position, 1);
}

extern "C" void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3])
{
    halo::ai::combat_ops(actor_index).get_aim_from_position(out_position);
}

namespace c_actor_get_consideration_wait_threshold {
extern "C" {
extern data_array *actor_data;

extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
}
}

extern "C" float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode, actor_combat_consideration *consideration);

/**
 * actor_get_consideration_wait_threshold: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_consideration_wait_threshold.c.txt.
 *
 * @address 0x4028e0
 */
float halo::ai::combat_ops::get_consideration_wait_threshold(int16_t mode, actor_combat_consideration *consideration)
{
    using namespace c_actor_get_consideration_wait_threshold;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    float result = 0.0f;

    if (mode == 2 || mode == 3) {
        Actor *actor_def = (Actor *)halo::cache::globals().tag_instances[a->actor_definition_tag & halo::k_slot_mask].data;

        if (mode == 3 && actor_def->melee_leap_range[1] >= 0.0f) {
            result = actor_def->melee_leap_range[1];
        }
        if (consideration->suicidal == 0) {
            float candidate = actor_def->melee_fudge_factor + consideration->distance_delta;
            if (result <= candidate) {
                result = candidate;
            }
        } else if (result <= actor_def->melee_fudge_factor) {
            return actor_def->melee_fudge_factor;
        }
    } else if (mode == 4 || mode == 0) {
        if (actor_has_unshielded_threat_weapon(actor_index) != 0 && a->target_combat_status > 6 && a->maximum_firing_distance >= 0.0f) {
            return a->maximum_firing_distance;
        }
    }
    return result;
}

extern "C" float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode, actor_combat_consideration *consideration)
{
    return halo::ai::combat_ops(actor_index).get_consideration_wait_threshold(mode, consideration);
}

namespace c_actor_get_relevant_squad_member_target {
extern "C" {
extern data_array *prop_data;
extern data_array *object_data;

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
}
}

extern "C" datum_index actor_get_relevant_squad_member_target(uint32_t unused_param, datum_index member_prop_index, char require_is_unit);

/**
 * actor_get_relevant_squad_member_target: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_relevant_squad_member_target.c.txt.
 *
 * @address 0x41f550
 */
datum_index halo::ai::combat_ops::get_relevant_squad_member_target(uint32_t unused_param, datum_index member_prop_index, char require_is_unit)
{
    using namespace c_actor_get_relevant_squad_member_target;
    prop *member;
    object_header *headers;
    unit_data *member_unit;
    int32_t i;
    datum_index responsible;
    int16_t responsible_index;
    int16_t responsible_salt;
    int16_t header_identifier;
    object_header *attacker_header;
    object *attacker_obj;
    unit_data *attacker_unit;
    datum_index resolved_object;
    datum_index resolved_prop;
    prop *candidate;
    int32_t best_tick;
    datum_index best_prop;

    (void)unused_param;

    member = (prop *)((uint8_t *)prop_data->data + (member_prop_index & halo::k_slot_mask) * sizeof(prop));
    member_unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[member->object_index & halo::k_slot_mask].data +
                                 k_unit_data_offset);

    best_prop = k_datum_index_none;
    best_tick = 0;

    for (i = 0; i < k_unit_recent_damage_count; i++) {
        responsible = member_unit->recent_damage[i].responsible_unit;
        if (responsible != k_datum_index_none) {
            resolved_object = k_datum_index_none;
            responsible_index = (int16_t)responsible;
            if (-1 < responsible_index && responsible_index < object_data->maximum_count) {
                headers = (object_header *)object_data->data;
                header_identifier = headers[responsible_index].identifier;
                responsible_salt = (int16_t)(responsible >> 16);
                if (header_identifier != 0 && (responsible_salt == 0 || header_identifier == responsible_salt)) {
                    resolved_object = responsible;

                    attacker_header = &headers[responsible_index];

                    if ((1 << (attacker_header->type & 0x1f) & 3) != 0) {
                        attacker_obj = attacker_header->data;
                        if (attacker_obj != (object *)0) {
                            attacker_unit = (unit_data *)((uint8_t *)attacker_obj + k_unit_data_offset);
                            resolved_object = attacker_unit->gunner_unit_index;
                            if (resolved_object == k_datum_index_none) {
                                resolved_object = responsible;
                                if (attacker_unit->driver_unit_index != k_datum_index_none) {
                                    resolved_object = attacker_unit->driver_unit_index;
                                }
                            }
                        } else {
                            resolved_object = k_datum_index_none;
                        }
                    } else {
                        resolved_object = k_datum_index_none;
                    }
                }
            }

            if (resolved_object != k_datum_index_none) {
                resolved_prop = actor_find_prop_for_object(resolved_object, (datum_index)unused_param);
                if (resolved_prop != k_datum_index_none) {
                    candidate = (prop *)((uint8_t *)prop_data->data + (resolved_prop & halo::k_slot_mask) * sizeof(prop));
                    if ((1 < candidate->state && candidate->state < 4) &&
                        ((candidate->enemy != 0 || require_is_unit == 0) &&
                         (best_tick < member_unit->recent_damage[i].tick))) {
                        best_tick = member_unit->recent_damage[i].tick;
                        best_prop = resolved_prop;
                    }
                }
            }
        }
    }

    return best_prop;
}

extern "C" datum_index actor_get_relevant_squad_member_target(uint32_t unused_param, datum_index member_prop_index, char require_is_unit)
{
    return halo::ai::combat_ops::get_relevant_squad_member_target(unused_param, member_prop_index, require_is_unit);
}

namespace c_actor_get_squad_recent_attacker_target {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *prop_data;

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
}
}

extern "C" datum_index actor_get_squad_recent_attacker_target(datum_index actor_index, char require_is_unit);

/**
 * actor_get_squad_recent_attacker_target: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_squad_recent_attacker_target.c.txt.
 *
 * @address 0x41f6b0
 */
datum_index halo::ai::combat_ops::get_squad_recent_attacker_target(char require_is_unit)
{
    using namespace c_actor_get_squad_recent_attacker_target;
    datum_index actor_index = datum;
    datum_index unit_index = *(datum_index *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * 0x724 + 0x18);
    datum_index best_prop = k_datum_index_none;
    uint32_t best_tick = 0;
    uint8_t *record;
    int32_t i;

    if (unit_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    record = (uint8_t *)((object_header *)object_data->data)[unit_index & halo::k_slot_mask].data + 0x430;
    for (i = 4; i != 0; i--, record += 0x10) {
        datum_index responsible = *(datum_index *)(record + 0x8);
        uint8_t *unit;
        datum_index source;
        datum_index prop_index;
        uint8_t *p;

        if (responsible == k_datum_index_none) {
            continue;
        }
        unit = (uint8_t *)object_try_and_get(responsible, 3);
        if (unit == 0) {
            continue;
        }
        source = ((unit_object *)unit)->unit.gunner_unit_index;
        if (source == k_datum_index_none) {
            source = ((unit_object *)unit)->unit.driver_unit_index;
            if (source == k_datum_index_none) {
                source = responsible;
            }
        }
        if (source == k_datum_index_none) {
            continue;
        }
        prop_index = actor_find_prop_for_object(source, actor_index);
        if (prop_index == k_datum_index_none) {
            continue;
        }
        p = (uint8_t *)prop_data->data + (prop_index & halo::k_slot_mask) * 0x138;
        if (*(int16_t *)(p + 0x24) < 2 || *(int16_t *)(p + 0x24) > 3) {
            continue;
        }
        if (!p[0x60] && require_is_unit) {
            continue;
        }
        if (*(uint32_t *)record > best_tick) {
            best_prop = prop_index;
            best_tick = *(uint32_t *)record;
        }
    }
    return best_prop;
}

extern "C" datum_index actor_get_squad_recent_attacker_target(datum_index actor_index, char require_is_unit)
{
    return halo::ai::combat_ops(actor_index).get_squad_recent_attacker_target(require_is_unit);
}

namespace c_actor_get_target_state_flags {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern void actor_target_get_relationship_object(datum_index target_prop_index);

extern uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point,
    uint32_t start_surface_index, int16_t kind);
}
}

extern "C" void actor_get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index, int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g, uint8_t *out_h, uint8_t *out_i);

/**
 * actor_get_target_state_flags: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_target_state_flags.c.txt.
 *
 * @address 0x40cc70
 */
void halo::ai::combat_ops::get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index, int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g, uint8_t *out_h, uint8_t *out_i)
{
    using namespace c_actor_get_target_state_flags;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];

    if (ax_mode == 2 || (ax_mode == 1 && shared_flag != 0)) {
        *out_f = 0;
    }
    if (cx_mode == 2 || (cx_mode == 1 && shared_flag != 0)) {
        *out_g = 0;
        *out_h = 0;
    } else if (mode_b == 2 || (mode_b == 1 && shared_flag != 0)) {
        *out_h = 0;
    }
    if (force_c != 0) {
        *out_g = 0;
        *out_h = 0;
        *out_i = 0;
    }

    if (a->target_unit_index == (datum_index)k_datum_index_none) {
        *out_a = 0;
        *out_f = 0;
        return;
    }

    {
        prop *p = &((prop *)prop_data->data)[a->target_unit_index & halo::k_slot_mask];
        uint8_t no_noticed_b = (p->noticed_b == 0);

        *out_a = (p->noticed_a == 0);
        *out_f = no_noticed_b;

        if (force_d == 0) {
            if (no_noticed_b) {
                if (p->state > 1 && p->state < 4) {

                    actor_target_get_relationship_object(a->target_unit_index);
                }
                *out_f = actor_firing_position_near_point(actor_index, (real_point3d *)((uint8_t *)p + 0xf0), *(uint32_t *)&((struct prop *)p)->pathfinding_surface_index, 1);
            }
        } else {
            *out_a = 0;
        }
    }

    if (*out_in_e == 0 && a->combat_status < 3) {
        *out_a = 0;
    }
    if (a->vehicle_driving_type > 0) {
        *out_i = 0;
    }
    if (a->vehicle_driving_type == 4) {
        *out_f = 0;
        *out_h = 0;
    }
}

extern "C" void actor_get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index, int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g, uint8_t *out_h, uint8_t *out_i)
{
    halo::ai::combat_ops::get_target_state_flags(ax_mode, cx_mode, shared_flag, actor_index, mode_b, force_c, force_d, out_a, out_in_e, out_f, out_g, out_h, out_i);
}

namespace c_actor_get_threat_weapon_definition {
extern "C" {
extern data_array *object_data;
extern datum_index actor_get_threat_weapon_object_index(int32_t actor_index);
}
}

extern "C" void * actor_get_threat_weapon_definition(int32_t actor_index);

/**
 * actor_get_threat_weapon_definition: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_threat_weapon_definition.c.txt.
 *
 * @address 0x40f970
 */
void * halo::ai::combat_ops::get_threat_weapon_definition()
{
    using namespace c_actor_get_threat_weapon_definition;
    int32_t actor_index = datum;
    datum_index weapon_object;

    weapon_object = actor_get_threat_weapon_object_index(actor_index);
    if (weapon_object != (datum_index)k_datum_index_none) {
        object_header *hdr = (object_header *)object_data->data + (weapon_object & halo::k_slot_mask);
        object *obj = hdr->data;
        return halo::cache::globals().tag_instances[obj->definition_tag & halo::k_slot_mask].data;
    }
    return (void *)0;
}

extern "C" void * actor_get_threat_weapon_definition(int32_t actor_index)
{
    return halo::ai::combat_ops(actor_index).get_threat_weapon_definition();
}

namespace c_actor_is_burst_pending {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" uint8_t actor_is_burst_pending(datum_index actor_index);

/**
 * actor_is_burst_pending: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_is_burst_pending.c.txt.
 *
 * @address 0x428180
 */
uint8_t halo::ai::combat_ops::is_burst_pending()
{
    using namespace c_actor_is_burst_pending;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    return self->awareness_level == 3 && self->minimum_combat_status < self->combat_status;
}

extern "C" uint8_t actor_is_burst_pending(datum_index actor_index)
{
    return halo::ai::combat_ops(actor_index).is_burst_pending();
}

namespace c_actor_is_target_within_engagement_range {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern Scenario *global_scenario;

extern float actor_compute_accuracy_scale(datum_index actor_index);
}
}

extern "C" uint8_t actor_is_target_within_engagement_range(uint32_t actor_index);

/**
 * actor_is_target_within_engagement_range: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_is_target_within_engagement_range.c.txt.
 *
 * @address 0x403dc0
 */
uint8_t halo::ai::combat_ops::is_target_within_engagement_range()
{
    using namespace c_actor_is_target_within_engagement_range;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    ScenarioFiringPosition *fp;
    float dx, dy, dz, range;
    datum_index leader_prop_index;

    if (a->encounter_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    if (a->firing_position_index == -1) {
        return 0;
    }

    {
        ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
        ScenarioFiringPosition *positions = (ScenarioFiringPosition *)encounters[a->encounter_index & halo::k_slot_mask].firing_positions.pointer;
        fp = &positions[a->firing_position_index];
    }

    if ((a->movement_action_complete == 0 || a->movement_completed != 0) &&
        a->active_movement.type == 3 &&
        *(int16_t *)((uint8_t *)&a->active_movement + 4) == a->firing_position_index) {
        return 1;
    }

    range = actor_compute_accuracy_scale(actor_index);
    dx = fp->position.x - a->body_position.x;
    dy = fp->position.y - a->body_position.y;
    dz = fp->position.z - a->body_position.z;
    if (range * range <= dx * dx + dy * dy + dz * dz) {
        return 0;
    }

    leader_prop_index = *(datum_index *)(a->mode_data.raw + (0xb8 - 0x9c));
    if (leader_prop_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[leader_prop_index & halo::k_slot_mask];
        if (p->obstruction != 0 && p->obstruction != 1) {
            return 1;
        }
        return 0;
    }
    return 1;
}

extern "C" uint8_t actor_is_target_within_engagement_range(uint32_t actor_index)
{
    return halo::ai::combat_ops(actor_index).is_target_within_engagement_range();
}

