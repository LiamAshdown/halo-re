#include "halo/ai/actor_firing.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/cseries/cseries.hpp"

namespace c_actor_claim_firing_position {
}


/**
 * actor_claim_firing_position: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_claim_firing_position.c.txt.
 *
 * @address 0x414060
 */
int16_t halo::ai::firing_position_ops::claim_firing_position(datum_index previous_owner, path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok)
{
    using namespace c_actor_claim_firing_position;
    datum_index actor_index = datum;
    actor *self;
    actor *other;

    self = halo::ai::actor_at(actor_index);

    if (firing_position_index == -1) {
        halo::ai::actor_movement_action_stop(actor_index);
    } else {

        if (self->firing_position_index != -1 && self->firing_position_index != firing_position_index) {
            halo::ai::actor_push_recognition_entry(actor_index, self->firing_position_index, 1);
        }

        if (previous_owner != (datum_index)halo::k_dword_none) {
            other = halo::ai::actor_at(previous_owner);
            halo::ai::actor_movement_action_stop(previous_owner);
            other->firing_position_index = -1;
        }

        if (self->firing_position_index == firing_position_index) {
            return self->firing_position_index;
        }

        self->firing_position_index = firing_position_index;
        self->firing_position_without_path = (uint8_t)(path_ok == 0);
        self->grenade_evasion_active = 0;

        if (halo::ai::actor_movement_set_destination_firing_position(actor_index, firing_position_index,
                path_ok ? path_context : (path_find_context *)0) != 0) {
            return self->firing_position_index;
        }
    }

    self->firing_position_index = -1;
    return self->firing_position_index;
}

namespace halo::ai {
int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner, path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok)
{
    return halo::ai::firing_position_ops(actor_index).claim_firing_position(previous_owner, path_context, firing_position_index, path_ok);
}
}

namespace c_actor_find_best_firing_position {
extern "C" {
extern const real_vector3d *global_origin3d_pointer;

extern int16_t qsort_candidate_count;
extern actor_firing_position_candidate *qsort_candidate_base;

extern double sqrt(double x);


}
}


/**
 * actor_find_best_firing_position: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_best_firing_position.c.txt.
 *
 * @address 0x412ba0
 */
uint32_t halo::ai::firing_position_ops::find_best_firing_position(actor_firing_position_query *query, actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context, uint8_t *out_path_ok)
{
    using namespace c_actor_find_best_firing_position;
    datum_index actor_index = datum;
    actor *self;
    Actor *actor_definition;
    ActorVariant *variant;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    prop *target;
    datum_index prop_index;

    path_find_request request;

    int32_t sort_index[512];
    uint32_t claims[512];
    actor_firing_position_candidate candidates[512];
    path_find_context target_context;

    int16_t candidate_count;
    uint32_t best_index;
    float best_score;
    uint8_t any_in_range;
    uint8_t truncated;
    real_point3d hazard_direction;
    real_vector3d delta;
    float distance_squared;
    float length;
    float owner_distance;
    float self_distance;
    uint32_t owner;
    uint32_t *clear;
    int32_t i;
    int32_t n;
    int16_t k;

    self = halo::ai::actor_at(actor_index);
    best_index = halo::k_dword_none;
    best_score = 0.0f;

    if (self->encounter_index == (datum_index)halo::k_dword_none) {
        return halo::k_dword_none;
    }

    actor_definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    variant = halo::ai::tag_data<ActorVariant>(self->actor_variant_tag);
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                                [self->encounter_index & halo::k_slot_mask];

    candidate_count = 0;
    any_in_range = 0;

    halo::ai::encounter_build_firing_position_claims(self->encounter_index, (datum_index *)claims);
    if (self->firing_position_index != -1) {
        claims[self->firing_position_index] = halo::k_dword_none;
    }

    query->maximum_distance = (self->vehicle_driving_type == 0) ? 15.0f : 80.0f;
    if (query->search_radius == 0.0f) {
        query->search_radius = query->maximum_distance;
    }
    query->unknown_42 = (uint8_t)(query->goal_kind == 5);
    query->have_target = 0;

    if (query->have_explicit_target == 0) {
        prop_index = (datum_index)halo::k_dword_none;
        if (self->mode == 4 && *(uint32_t *)&self->mode_data.raw[0x1c] != halo::k_dword_none) {
            prop_index = *(datum_index *)&self->mode_data.raw[0x1c];
        } else {
            prop_index = (datum_index)self->target_unit_index;
        }
        if (prop_index != (datum_index)halo::k_dword_none) {
            target = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
            if (query->unknown_42 != 0 && target->state > 1 && target->state < 4) {
                halo::ai::actor_target_get_relationship_object(prop_index);
            }
            query->have_target = 1;
            query->target_position = target->last_known_position;
            query->target_surface_index = *(uint32_t *)&target->pathfinding_surface_index;
            query->target_surface_point = *(real_point3d *)&target->pathfinding_point.x;
            query->target_cluster_index = target->cluster_index;
            query->target_distance = target->distance;
            query->target_prop_index = prop_index;
            query->target_aim_position = *(real_point3d *)&target->head_position_x;
            query->target_relationship_object = target->relationship_object_index;
            query->target_danger_radius = target->danger_radius;

            if (query->use_last_seen_position == 0 || target->last_seen_time == -1) {
                query->target_lead_position = *(real_point3d *)&target->head_position_x;
            } else {
                query->target_lead_position = *(real_point3d *)&target->last_seen_position_x;
            }

            if (target->state > 3 && target->state < 6) {
                query->have_target_vault_point = 1;
                query->target_vault_point = *(real_point3d *)&target->perceived_to_known_delta;
            }

            query->target_is_large = (uint8_t)(query->goal_kind == 4 || query->goal_kind == 6);
        }
    } else {
        query->target_position = query->explicit_target_position;
        query->target_surface_point = query->explicit_target_position;
        query->target_cluster_index = (int16_t)query->explicit_target_cluster_index;
        query->have_target = 1;
        query->target_surface_index = query->explicit_target_object;

        delta.i = query->target_position.x - self->body_position.x;
        delta.j = query->target_position.y - self->body_position.y;
        delta.k = query->target_position.z - self->body_position.z;
        query->target_prop_index = (datum_index)halo::k_dword_none;
        query->target_relationship_object = -1;
        query->target_danger_radius = 0.0f;
        query->target_distance = (float)sqrt((double)(delta.i * delta.i + delta.j * delta.j +
                                                      delta.k * delta.k));
        halo::units::unit_add_marker_relative_offset(self->unit_index, 1, (float *)&query->target_position, 0, 0,
            &query->target_aim_position);
        query->target_lead_position = query->target_aim_position;
    }

    if (variant->custom_stand_gun_offset.i * variant->custom_stand_gun_offset.i +
        variant->custom_stand_gun_offset.j * variant->custom_stand_gun_offset.j +
        variant->custom_stand_gun_offset.k * variant->custom_stand_gun_offset.k <= 0.0001f) {
        if (actor_definition->standing_gun_offset.i * actor_definition->standing_gun_offset.i +
            actor_definition->standing_gun_offset.j * actor_definition->standing_gun_offset.j +
            actor_definition->standing_gun_offset.k * actor_definition->standing_gun_offset.k <=
                0.0001f) {
            query->have_standing_gun_offset = 0;
        } else {
            query->have_standing_gun_offset = 1;
            query->standing_gun_offset = *(real_vector3d *)&actor_definition->standing_gun_offset;
        }
    } else {
        query->have_standing_gun_offset = 1;
        query->standing_gun_offset = *(real_vector3d *)&variant->custom_stand_gun_offset;
    }

    if (variant->custom_crouch_gun_offset.i * variant->custom_crouch_gun_offset.i +
        variant->custom_crouch_gun_offset.j * variant->custom_crouch_gun_offset.j +
        variant->custom_crouch_gun_offset.k * variant->custom_crouch_gun_offset.k <= 0.0001f) {
        if (actor_definition->crouching_gun_offset.i * actor_definition->crouching_gun_offset.i +
            actor_definition->crouching_gun_offset.j * actor_definition->crouching_gun_offset.j +
            actor_definition->crouching_gun_offset.k * actor_definition->crouching_gun_offset.k <=
                0.0001f) {
            query->have_crouching_gun_offset = 0;
        } else {
            query->have_crouching_gun_offset = 1;
            query->crouching_gun_offset = *(real_vector3d *)&actor_definition->crouching_gun_offset;
        }
    } else {
        query->have_crouching_gun_offset = 1;
        query->crouching_gun_offset = *(real_vector3d *)&variant->custom_crouch_gun_offset;
    }

    if (self->danger_type > 0 && self->danger_reacting != 0 &&
        self->danger_distance < self->danger_radius + 3.0f) {
        query->danger_active = 1;
    }
    query->flying = self->flying;
    if (self->vehicle_driving_type == 4) {
        query->check_vehicle_aim_cone = 1;
        query->vehicle_ignore_velocity = 1;
    }

    query->danger_sphere_count = 0;
    if (self->recognition_valid != 0) {
        query->danger_spheres[0].position = self->recognition_position;
        query->danger_spheres[query->danger_sphere_count].radius =
            actor_definition->old_position_avoid_dist;
        query->danger_sphere_count = query->danger_sphere_count + 1;
    }

    if (actor_definition->friend_avoid_dist > 0.0f &&
        (query->goal_kind == 0 || query->goal_kind == 3 || query->goal_kind == 6)) {
        datum_index p = self->first_prop;
        while (query->danger_sphere_count < 0x20 && p != (datum_index)halo::k_dword_none) {
            prop *pr = &((prop *)halo::ai::globals().prop_data->data)[p & halo::k_slot_mask];
            p = pr->next_in_actor;
            if (pr->state > 1 && pr->state < 4 && pr->enemy == 0 && pr->dead == 0 &&
                pr->is_parented == 0) {
                query->danger_spheres[query->danger_sphere_count].position =
                    pr->last_known_position;
                query->danger_spheres[query->danger_sphere_count].radius =
                    actor_definition->friend_avoid_dist;
                query->danger_sphere_count = query->danger_sphere_count + 1;
            }
        }
    }

    query->hazard_count = 0;
    query->hazard_count_kind_01 = 0;
    query->hazard_count_kind_2 = 0;
    {
        uint8_t gather_hazards = 0;
        if (halo::has(static_cast<halo::tags::actor_tag_flag>(actor_definition->flags), halo::tags::actor_tag_flag::avoid_friends_line_of_fire)  && self->combat_status > 2 &&
            (int8_t)self->tally.group_a_total > 0) {
            gather_hazards = 1;
        }
        if ((actor_definition->more_flags & 1) != 0  && self->combat_status > 2) {
            gather_hazards = 1;
        }

        if (gather_hazards != 0 &&
            (query->goal_kind == 0 || query->goal_kind == 2 || query->goal_kind == 3 ||
             query->goal_kind == 6)) {
            datum_index p = self->first_prop;
            do {
                prop *pr;
                int16_t kind;
                datum_index current = p;
                if (p == (datum_index)halo::k_dword_none) {
                    break;
                }
                pr = &((prop *)halo::ai::globals().prop_data->data)[p & halo::k_slot_mask];
                kind = pr->state;
                p = pr->next_in_actor;
                if (kind > 1 && kind < 4 && pr->dead == 0) {
                    if (pr->enemy == 0 &&
                        (pr->is_parented != 0 || pr->relationship_object_index == -1) &&
                        halo::ai::actor_get_ranged_attack_vector(current, actor_index, (real_vector3d *)&hazard_direction) != 0) {
                        actor_firing_position_hazard *h = &query->hazards[query->hazard_count];
                        h->kind = (int16_t)(pr->is_parented != 0);
                        h->position = pr->last_known_position;
                        h->direction.i = hazard_direction.x;
                        h->direction.j = hazard_direction.y;
                        h->direction.k = hazard_direction.z;
                        query->hazard_count = query->hazard_count + 1;
                        query->hazard_count_kind_01 = query->hazard_count_kind_01 + 1;
                    }
                    if (pr->enemy != 0 &&
                        ((pr->engaged != 0 && (pr->is_parented != 0 || pr->shooting != 0)) ||
                         (actor_definition->more_flags & 1) != 0 )) {
                        actor_firing_position_hazard *h = &query->hazards[query->hazard_count];
                        h->kind = 2;
                        h->position = pr->last_known_position;

                        halo::units::unit_get_aiming_vector(pr->relationship_object_index != -1 ? (uint32_t)pr->relationship_object_index
                                                                                    : (uint32_t)pr->object_index,
                                               &h->direction);
                        query->hazard_count = query->hazard_count + 1;
                        query->hazard_count_kind_2 = query->hazard_count_kind_2 + 1;
                    }
                }
            } while (query->hazard_count < 0x20);
        }
    }

    *out_path_ok = 0;
    truncated = 0;
    firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;

    for (i = 0; i < (int32_t)encounter_definition->firing_positions.count; i++) {
        ScenarioFiringPosition *fp = &firing_positions[i];
        actor_firing_position_candidate *c;

        if ((query->group_mask & (1u << (((uint8_t)fp->group_index) & 0x1f))) == 0) {
            continue;
        }
        if (query->flying == 0 && fp->surface_index == halo::k_dword_none) {
            continue;
        }
        if (query->goal_kind == 5 &&
            halo::ai::actor_firing_position_near_point(actor_index, (real_point3d *)fp,
                                             (int32_t)fp->surface_index, 1) == 0) {
            continue;
        }

        owner = claims[i];
        if (owner != halo::k_dword_none) {
            actor *owner_actor = (actor *)((uint8_t *)halo::ai::globals().actor_data->data +
                                           (owner & halo::k_slot_mask) * sizeof(actor));
            if (query->goal_kind == 4 && query->danger_sphere_count < 0x20) {
                query->danger_spheres[query->danger_sphere_count].position =
                    *(real_point3d *)fp;
                query->danger_spheres[query->danger_sphere_count].radius = 4.0f;
                query->danger_sphere_count = query->danger_sphere_count + 1;
            }
            owner_distance = (float)sqrt((double)(
                (fp->position.x - owner_actor->body_position.x) *
                    (fp->position.x - owner_actor->body_position.x) +
                (fp->position.y - owner_actor->body_position.y) *
                    (fp->position.y - owner_actor->body_position.y) +
                (fp->position.z - owner_actor->body_position.z) *
                    (fp->position.z - owner_actor->body_position.z)));
            if (owner_distance < 1.0f) {
                continue;
            }
            self_distance = (float)sqrt((double)(
                (fp->position.x - self->body_position.x) *
                    (fp->position.x - self->body_position.x) +
                (fp->position.y - self->body_position.y) *
                    (fp->position.y - self->body_position.y) +
                (fp->position.z - self->body_position.z) *
                    (fp->position.z - self->body_position.z)));
            if (owner_distance < self_distance + self_distance) {
                continue;
            }
        }

        if (candidate_count >= 0x200) {
            if (truncated == 0) {
                truncated = 1;
            }
            continue;
        }

        c = &candidates[candidate_count];
        c->position = (uint32_t)(uint8_t *)fp;
        c->firing_position_index = (int16_t)i;
        c->request_result = 0;
        c->distance_from_actor = 3.4028235e+38f;
        c->direction_from_actor.i = global_origin3d_pointer->i;
        c->direction_from_actor.j = global_origin3d_pointer->j;
        c->direction_from_actor.k = global_origin3d_pointer->k;
        c->distance_from_target = 3.4028235e+38f;
        c->segment_distance = 3.4028235e+38f;
        c->direction_from_target.i = global_origin3d_pointer->i;
        c->direction_from_target.j = global_origin3d_pointer->j;
        c->direction_from_target.k = global_origin3d_pointer->k;
        c->distance_squared_to_target = 0.0f;
        c->score_before_rejects = 0.0f;
        c->score = 0.0f;
        c->valid = 1;
        c->rejected = 0;
        candidate_count = candidate_count + 1;
    }

    if (candidate_count == 0) {

        self->recognition_cursor = 0;
        for (k = 0; k < 4; k++) {
            self->recognition[k].firing_position_index = -1;
        }
        if (self->recognition_valid != 0) {
            self->recognition_valid = 0;
        }
        return best_index & halo::k_slot_mask;
    }

    if (query->have_target != 0 && query->unknown_42 != 0) {
        if (query->flying == 0) {
            if (query->target_surface_index != halo::k_dword_none) {
                clear = (uint32_t *)&request;
                for (n = 0; n < 0x12; n++) {
                    clear[n] = 0;
                }
                request.pathfinding_radius = actor_definition->pathfinding_radius;
                request.ignores_glass = self->ignores_glass;
                request.exclude_object_index_a = (datum_index)halo::k_dword_none;
                request.exclude_object_index_b = (datum_index)halo::k_dword_none;
                request.start_position = query->target_surface_point;
                request.start_surface_index = query->target_surface_index;
                request.have_start = 1;
                request.have_limit = 1;
                request.limit_distance = 20.0f;

                clear = (uint32_t *)&target_context;
                for (n = 0; n < 0x4023; n++) {
                    clear[n] = 0;
                }
                target_context.structure_bsp = (uint32_t)halo::scenario::globals().structure_bsp;
                for (n = 0; n < 0x12; n++) {
                    ((uint32_t *)&target_context)[n] = ((uint32_t *)&request)[n];
                }
                target_context.obstacle_cache = 0;
                halo::ai::path_find_run(&target_context);

                for (i = 0; i < candidate_count; i++) {
                    actor_firing_position_candidate *c = &candidates[i];

                    halo::ai::path_find_compute_heuristic(&target_context, *(uint32_t *)((uint8_t *)c->position + 0x14),
                                 (real_point3d *)c->position, &c->distance_from_target, (float *)0,
                                 (query->want_direction_from_target != 0) ? &c->direction_from_target : 0);
                }
            }
        } else {
            for (i = 0; i < candidate_count; i++) {
                actor_firing_position_candidate *c = &candidates[i];
                real_point3d *p = (real_point3d *)c->position;
                delta.i = p->x - query->target_position.x;
                delta.j = p->y - query->target_position.y;
                delta.k = p->z - query->target_position.z;
                if (delta.j * delta.j + delta.k * delta.k + delta.i * delta.i < 400.0f &&
                    halo::ai::path_find_test_direct_reachability(p, &query->target_position, 0, halo::scenario::globals().structure_bsp, 0) != 0) {
                    c->distance_from_target = halo::math::vector3d_normalize_with_length(delta);
                    if (query->want_direction_from_target != 0) {
                        c->direction_from_target = delta;
                    }
                }
            }
        }
    }

    if (query->flying == 0) {
        halo::ai::actor_build_path_find_request(actor_index, &request);
        request.have_limit = 1;
        request.limit_distance = query->search_radius;

        if (query->unknown_36 != 0 && query->have_target != 0) {
            request.avoid_object_index = (datum_index)halo::k_dword_none;
            if (query->target_prop_index != (datum_index)halo::k_dword_none) {
                request.avoid_object_index =
                    ((prop *)halo::ai::globals().prop_data->data)[query->target_prop_index & halo::k_slot_mask].object_index;
            }
            request.avoid_position = query->target_position;
            request.avoid_radius = query->avoid_radius;
            request.avoid_weight = query->avoid_weight;
            request.have_avoid_sphere = 1;
        } else if (self->danger_type > 0 && !halo::has(static_cast<halo::tags::actor_more_tag_flag>(actor_definition->more_flags), halo::tags::actor_more_tag_flag::pathfinding_ignores_danger) ) {
            request.avoid_position = self->flee_from_point;
            request.avoid_radius = self->danger_object_radius;
            request.avoid_object_index = self->danger_object_index;
            request.avoid_weight = 10.0f;
            request.have_avoid_sphere = 1;
        }

        clear = (uint32_t *)path_context;
        for (n = 0; n < 0x4023; n++) {
            clear[n] = 0;
        }
        path_context->structure_bsp = (uint32_t)halo::scenario::globals().structure_bsp;
        for (n = 0; n < 0x12; n++) {
            ((uint32_t *)path_context)[n] = ((uint32_t *)&request)[n];
        }
        path_context->obstacle_cache = 0;
        if (halo::ai::path_find_run(path_context) != 0) {
            *out_path_ok = 1;
        }
    }

    for (i = 0; i < candidate_count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        real_point3d *p = (real_point3d *)c->position;

        if (query->have_target != 0) {
            c->distance_squared_to_target =
                (p->y - query->target_position.y) * (p->y - query->target_position.y) +
                (p->z - query->target_position.z) * (p->z - query->target_position.z) +
                (p->x - query->target_position.x) * (p->x - query->target_position.x);
        }

        delta.i = p->x - self->body_position.x;
        delta.j = p->y - self->body_position.y;
        delta.k = p->z - self->body_position.z;
        distance_squared = delta.j * delta.j + delta.k * delta.k + delta.i * delta.i;

        if (distance_squared < query->search_radius * query->search_radius) {
            if (query->flying == 0) {

                halo::ai::path_find_compute_heuristic(path_context, *(uint32_t *)((uint8_t *)p + 0x14), p,
                             &c->distance_from_actor, &c->segment_distance,
                             (query->danger_active != 0) ? &c->direction_from_actor : 0);
            } else {

                c->segment_distance = (float)sqrt((double)halo::math::point3d_distance_squared_to_segment(
                    self->body_position, delta, query->target_position));
                length = (float)sqrt((double)distance_squared);
                if (length < 0.0001f && length > -0.0001f) {
                    length = 0.0f;
                } else {
                    float inverse = 1.0f / length;
                    delta.i = delta.i * inverse;
                    delta.j = delta.j * inverse;
                    delta.k = delta.k * inverse;
                }
                c->distance_from_actor = length;
                if (query->danger_active != 0) {
                    c->direction_from_actor = delta;
                }
            }
        }

        if (query->search_radius <= c->distance_from_actor) {
            c->valid = 0;
        } else {
            any_in_range = 1;
        }
    }

    if (query->allow_random_fallback != 0 && any_in_range == 0) {
        actor_firing_position_candidate *c;
        uint32_t roll;

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        roll = ((halo::math::globals().random_seed_global >> 0x10) * (uint32_t)(int32_t)candidate_count) >> 0x10;
        *out_path_ok = 0;
        best_index = roll;

        self->recognition_cursor = 0;
        for (k = 0; k < 4; k++) {
            self->recognition[k].firing_position_index = -1;
        }
        if (self->recognition_valid != 0) {
            self->recognition_valid = 0;
        }

        c = &candidates[(int16_t)roll];
        c->score = 0.0f;
        c->score_before_rejects = 0.0f;
        c->rejected = 0;
        c->valid = 1;
        halo::ai::actor_firing_position_run_score_rules(actor_index, 1, query, c);
        if (c->valid == 0) {
            return halo::k_dword_none;
        }
        if (query->have_target != 0) {
            halo::ai::actor_report_firing_position_request(actor_index, query, c);
        }
        c->score_before_rejects = c->score;
        c->valid = halo::ai::actor_firing_position_run_reject_rules(actor_index, query, c);
        if (c->valid == 0) {
            return halo::k_dword_none;
        }
    } else {
        halo::ai::actor_firing_position_run_score_rules(actor_index, (uint16_t)candidate_count, query,
                                              candidates);
        for (i = 0; i < candidate_count; i++) {
            sort_index[i] = i;
        }
        qsort_candidate_base = candidates;
        qsort_candidate_count = candidate_count;
        halo::cseries::dword_sort::sort((uint32_t)(int32_t)candidate_count, sort_index, halo::ai::actor_firing_position_compare);

        query->baseline_accept = halo::ai::actor_firing_position_probe_reject_rules(query, actor_index);

        for (n = 0; n < candidate_count; n++) {
            actor_firing_position_candidate *c;
            i = (int32_t)(int16_t)sort_index[n];
            c = &candidates[i];
            if (c->valid == 0) {
                break;
            }
            if (query->baseline_accept != 0 &&
                c->score + query->baseline_penalty <= best_score) {
                break;
            }
            if (query->have_target != 0) {
                halo::ai::actor_report_firing_position_request(actor_index, query, c);
            }
            c->score_before_rejects = c->score;
            if (halo::ai::actor_firing_position_run_reject_rules(actor_index, query, c) != 0 &&
                best_score < c->score) {
                best_index = (uint32_t)i;
                best_score = c->score;
            }
        }
    }

    if ((int16_t)best_index == -1) {
        return best_index;
    }
    if (out_candidate != (actor_firing_position_candidate *)0) {
        *out_candidate = candidates[(int16_t)best_index];
    }
    i = candidates[(int16_t)best_index].firing_position_index;
    if (out_previous_owner != (uint32_t *)0) {
        *out_previous_owner = claims[i];
    }
    return (uint32_t)(uint16_t)(int16_t)i;
}

namespace halo::ai {
uint32_t actor_find_best_firing_position(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context, uint8_t *out_path_ok)
{
    return halo::ai::firing_position_ops(actor_index).find_best_firing_position(query, out_candidate, out_previous_owner, path_context, out_path_ok);
}
}

namespace c_actor_firing_position_compare {
extern "C" {
extern actor_firing_position_candidate *qsort_candidate_base;
}
}


/**
 * actor_firing_position_compare: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_firing_position_compare.c.txt.
 *
 * @address 0x4127b0
 */
uint8_t halo::ai::firing_position_ops::firing_position_compare(int32_t element, int32_t other)
{
    using namespace c_actor_firing_position_compare;
    actor_firing_position_candidate *a = &qsort_candidate_base[element];
    actor_firing_position_candidate *b = &qsort_candidate_base[other];

    if (a->valid != b->valid) {
        return a->valid == 0;
    }
    if (a->rejected != b->rejected) {
        return a->rejected != 0;
    }
    return a->score < b->score;
}

namespace halo::ai {
uint8_t actor_firing_position_compare(int32_t element, int32_t other)
{
    return halo::ai::firing_position_ops::firing_position_compare(element, other);
}
}

namespace c_actor_firing_position_evaluate {
}


/**
 * actor_firing_position_evaluate: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_firing_position_evaluate.c.txt.
 *
 * @address 0x412820
 */
uint8_t halo::ai::firing_position_ops::firing_position_evaluate(actor_firing_position_candidate *candidate, actor_firing_position_query *query, datum_index actor_index)
{
    using namespace c_actor_firing_position_evaluate;
    candidate->score = 0.0f;
    candidate->score_before_rejects = 0.0f;
    candidate->valid = 1;
    candidate->rejected = 0;

    halo::ai::actor_firing_position_run_score_rules(actor_index, 1, query, candidate);

    if (candidate->valid != 0) {
        if (query->have_target != 0) {
            halo::ai::actor_report_firing_position_request(actor_index, query, candidate);
        }
        candidate->score_before_rejects = candidate->score;
        candidate->valid = halo::ai::actor_firing_position_run_reject_rules(actor_index, query, candidate);
    }

    return candidate->valid;
}

namespace halo::ai {
uint8_t actor_firing_position_evaluate(actor_firing_position_candidate *candidate, actor_firing_position_query *query, datum_index actor_index)
{
    return halo::ai::firing_position_ops::firing_position_evaluate(candidate, query, actor_index);
}
}

namespace c_actor_firing_position_near_point {
}


/**
 * actor_firing_position_near_point: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_firing_position_near_point.c.txt.
 *
 * @address 0x412960
 */
uint8_t halo::ai::firing_position_ops::firing_position_near_point(real_point3d *point, int32_t start_surface_index, int16_t kind)
{
    using namespace c_actor_firing_position_near_point;
    datum_index actor_index = datum;
    actor *self;
    Actor *actor_definition;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    path_find_request request;
    path_find_context context;
    uint32_t group_mask;
    float path_distance;
    float dx, dy, dz;
    int32_t i;
    uint32_t *clear;
    int32_t n;

    self = halo::ai::actor_at(actor_index);
    actor_definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if ((self->flying == 0 && start_surface_index == -1) ||
        self->encounter_index == (datum_index)halo::k_dword_none) {
        return 0;
    }

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                                [self->encounter_index & halo::k_slot_mask];
    group_mask = halo::ai::actor_get_firing_position_group_mask(actor_index, kind, 0);

    if (self->flying == 0) {
        clear = (uint32_t *)&request;
        for (n = 0; n < 0x12; n++) {
            clear[n] = 0;
        }
        request.pathfinding_radius = actor_definition->pathfinding_radius;
        request.ignores_glass = 1;
        request.exclude_object_index_a = (datum_index)halo::k_dword_none;
        request.exclude_object_index_b = (datum_index)halo::k_dword_none;
        request.have_start = 1;
        request.start_position.x = point->x;
        request.start_position.y = point->y;
        request.start_position.z = point->z;
        request.start_surface_index = (uint32_t)start_surface_index;
        request.have_limit = 1;
        request.limit_distance = 4.0f;

        clear = (uint32_t *)&context;
        for (n = 0; n < 0x4023; n++) {
            clear[n] = 0;
        }
        context.structure_bsp = (uint32_t)halo::scenario::globals().structure_bsp;
        for (n = 0; n < 0x12; n++) {
            ((uint32_t *)&context)[n] = ((uint32_t *)&request)[n];
        }
        context.obstacle_cache = 0;
        halo::ai::path_find_run(&context);
    }

    firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;
    for (i = 0; i < (int32_t)encounter_definition->firing_positions.count; i++) {
        ScenarioFiringPosition *fp = &firing_positions[i];
        if ((group_mask & (1u << (((uint8_t)fp->group_index) & 0x1f))) == 0) {
            continue;
        }
        dx = fp->position.x - point->x;
        dy = fp->position.y - point->y;
        dz = fp->position.z - point->z;
        if (dx * dx + dy * dy + dz * dz >= 16.0f) {
            continue;
        }
        if (self->flying == 0) {

            halo::ai::path_find_compute_heuristic(&context, fp->surface_index, (real_point3d *)fp, &path_distance, 0, 0);
            if (path_distance < 4.0f) {
                return 1;
            }
        } else {

            if (halo::ai::path_find_test_direct_reachability((real_point3d *)fp, point, 0, halo::scenario::globals().structure_bsp, 0) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point, int32_t start_surface_index, int16_t kind)
{
    return halo::ai::firing_position_ops(actor_index).firing_position_near_point(point, start_surface_index, kind);
}
}

namespace c_actor_firing_position_probe_reject_rules {
extern "C" {
extern actor_firing_position_rule actor_firing_position_reject_rules[6];
}
}


/**
 * actor_firing_position_probe_reject_rules: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_firing_position_probe_reject_rules.c.txt.
 *
 * @address 0x412770
 */
uint8_t halo::ai::firing_position_ops::firing_position_probe_reject_rules(actor_firing_position_query *query, datum_index actor_index)
{
    using namespace c_actor_firing_position_probe_reject_rules;
    actor_firing_position_rule *row;
    uint8_t (*proc)(datum_index actor_index, actor_firing_position_query *query,
                    actor_firing_position_candidate *candidate);
    uint8_t accepted;

    accepted = 1;
    query->baseline_penalty = 0.0f;

    row = actor_firing_position_reject_rules;
    do {
        if (row->proc == 0) {
            return accepted;
        }
        if ((((int32_t)row->kinds) & (1 << (((uint8_t)query->goal_kind) & 0x1f))) != 0) {
            proc = (uint8_t (*)(datum_index, actor_firing_position_query *,
                                actor_firing_position_candidate *))row->proc;
            accepted = proc(actor_index, query, (actor_firing_position_candidate *)0);
        }
        row++;
    } while (accepted != 0);

    return accepted;
}

namespace halo::ai {
uint8_t actor_firing_position_probe_reject_rules(actor_firing_position_query *query, datum_index actor_index)
{
    return halo::ai::firing_position_ops::firing_position_probe_reject_rules(query, actor_index);
}
}

namespace c_actor_firing_position_run_reject_rules {
extern "C" {
extern actor_firing_position_rule actor_firing_position_reject_rules[6];
}
}


/**
 * actor_firing_position_run_reject_rules: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_firing_position_run_reject_rules.c.txt.
 *
 * @address 0x412730
 */
uint8_t halo::ai::firing_position_ops::firing_position_run_reject_rules(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    using namespace c_actor_firing_position_run_reject_rules;
    datum_index actor_index = datum;
    actor_firing_position_rule *row;
    uint8_t (*proc)(datum_index actor_index, actor_firing_position_query *query,
                    actor_firing_position_candidate *candidate);
    uint8_t accepted;

    accepted = 1;
    row = actor_firing_position_reject_rules;
    do {
        if (row->proc == 0) {
            return accepted;
        }
        if ((((int32_t)row->kinds) & (1 << (((uint8_t)query->goal_kind) & 0x1f))) != 0) {
            proc = (uint8_t (*)(datum_index, actor_firing_position_query *,
                                actor_firing_position_candidate *))row->proc;
            accepted = proc(actor_index, query, candidate);
        }
        row++;
    } while (accepted != 0);

    return accepted;
}

namespace halo::ai {
uint8_t actor_firing_position_run_reject_rules(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    return halo::ai::firing_position_ops(actor_index).firing_position_run_reject_rules(query, candidate);
}
}

namespace c_actor_firing_position_run_score_rules {
extern "C" {
extern actor_firing_position_rule actor_firing_position_score_rules[7];
}
}


/**
 * actor_firing_position_run_score_rules: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_firing_position_run_score_rules.c.txt.
 *
 * @address 0x4126f0
 */
void halo::ai::firing_position_ops::firing_position_run_score_rules(uint16_t count, actor_firing_position_query *query, actor_firing_position_candidate *candidates)
{
    using namespace c_actor_firing_position_run_score_rules;
    datum_index actor_index = datum;
    actor_firing_position_rule *row;
    void (*proc)(datum_index actor_index, actor_firing_position_query *query, uint16_t count,
                 actor_firing_position_candidate *candidates);

    row = actor_firing_position_score_rules;
    do {
        if ((((int32_t)row->kinds) & (1 << (((uint8_t)query->goal_kind) & 0x1f))) != 0) {
            proc = (void (*)(datum_index, actor_firing_position_query *, uint16_t,
                             actor_firing_position_candidate *))row->proc;
            proc(actor_index, query, count, candidates);
        }
        row++;
    } while (row->proc != 0);
}

namespace halo::ai {
void actor_firing_position_run_score_rules(datum_index actor_index, uint16_t count, actor_firing_position_query *query, actor_firing_position_candidate *candidates)
{
    halo::ai::firing_position_ops(actor_index).firing_position_run_score_rules(count, query, candidates);
}
}

namespace c_actor_get_firing_position_group_mask {
}


/**
 * actor_get_firing_position_group_mask: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_firing_position_group_mask.c.txt.
 *
 * @address 0x412880
 */
uint32_t halo::ai::firing_position_ops::get_firing_position_group_mask(int16_t kind, int16_t search_override)
{
    using namespace c_actor_get_firing_position_group_mask;
    datum_index actor_index = datum;
    actor *self;
    ScenarioEncounter *encounters;
    ScenarioSquad *squad;
    uint32_t *groups;
    uint8_t searching;

    self = halo::ai::actor_at(actor_index);
    if (self->encounter_index == (datum_index)halo::k_dword_none) {
        return 0;
    }

    encounters = (ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer;
    squad = &((ScenarioSquad *)encounters[self->encounter_index & halo::k_slot_mask].squads.pointer)
                 [self->squad_index];
    groups = &squad->attacking;

    searching = self->search_firing_positions;
    if (search_override == 1) {
        searching = 1;
    } else if (search_override == 2) {
        searching = 0;
    }

    if (kind == 1) {
        return squad->defending_guard;
    }
    if (kind == 4) {
        return groups[(self->defending != 0 ? 3 : 0) + 2];
    }
    if (kind == 5) {
        return squad->pursuing;
    }
    if (self->defending != 0) {
        return groups[(searching != 0 ? 1 : 0) + 3];
    }
    return groups[searching != 0 ? 1 : 0];
}

namespace halo::ai {
uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override)
{
    return halo::ai::firing_position_ops(actor_index).get_firing_position_group_mask(kind, search_override);
}
}

namespace c_actor_get_firing_positions {
}


/**
 * actor_get_firing_positions: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_firing_positions.c.txt.
 *
 * @address 0x41c1e0
 */
void halo::ai::firing_position_ops::get_firing_positions(uint32_t *out_block, real_point3d *query_point)
{
    using namespace c_actor_get_firing_positions;
    datum_index actor_index = datum;
    actor *self;
    uint32_t *src;
    int32_t i;
    swarm *group;
    int16_t component_count;
    swarm_component *component;
    float dx, dy, dz;
    float distance_squared;
    float min_distance_squared;

    self = halo::ai::actor_at(actor_index);

    if (self->swarm == 0) {
        src = (uint32_t *)&self->aim_origin;
        for (i = 0xe; i != 0; i--) {
            *out_block = *src;
            src++;
            out_block++;
        }
        return;
    }

    group = halo::ai::swarm_at(self->swarm_index);
    component_count = group->component_count;
    min_distance_squared = 3.4028235e+38f;
    {
    datum_index nearest_unit = k_datum_index_none;

    if (0 < component_count) {
        for (i = 0; i < component_count; i++) {
            component = (swarm_component *)((uint8_t *)halo::ai::globals().swarm_component_data->data +
                                            (group->component_index[i] & halo::k_slot_mask) * sizeof(swarm_component));
            dx = query_point->x - component->position.x;
            dy = query_point->y - component->position.y;
            dz = query_point->z - component->position.z;
            distance_squared = dz * dz + dy * dy + dx * dx;
            if (distance_squared < min_distance_squared) {
                min_distance_squared = distance_squared;
                nearest_unit = group->unit_index[i];
            }
        }
    }

    halo::ai::actor_fill_unit_position_context(nearest_unit, (actor_unit_position_context *)out_block);
    }
}

namespace halo::ai {
void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point)
{
    halo::ai::firing_position_ops(actor_index).get_firing_positions(out_block, query_point);
}
}

