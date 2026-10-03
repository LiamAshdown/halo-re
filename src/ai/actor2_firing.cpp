#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace halo::ai {

namespace actor_reject_firing_position_by_perception_local {
static void reject(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    candidate->rejected = 1;
    if (query->collect_all == 0) {
        candidate->valid = 0;
    }
}
}

/**
 * Actor AI behaviour: reject firing position by perception.
 *
 * @address 0x4124c0
 */
uint8_t ActorView::reject_firing_position_by_perception(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    using namespace actor_reject_firing_position_by_perception_local;
    (void)actor_index;
    if (query->have_target != 0) {
        float value = 0.0f;

        if (candidate == (actor_firing_position_candidate *)0) {
            query->baseline_penalty = query->baseline_penalty + 12.0f;
            return 1;
        }
        switch (candidate->request_result) {
        case 0:
            reject(query, candidate);
            break;
        case 1:
            if (query->unknown_08[0] != 0) {
                value = 6.0f;
            } else {
                reject(query, candidate);
            }
            break;
        case 2:
            value = 12.0f;
            break;
        case 3:
            value = 4.0f;
            break;
        case 4:
            value = 10.0f;
            break;
        }
        candidate->score = value + candidate->score;
    }
    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}

namespace actor_reject_firing_position_by_pursuit_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * 0x436b90: EBX object, EAX min_last_tick, CX type, stack (encounter, out_count, out_last_tick); the
 * create_if_missing slot is the constant 0 the binary passes on to squad_recent_object_get_or_create The pursuit
 * rule. A candidate the actor can already reach in under six units and that the movement req
 *
 * @address 0x412350
 */
uint8_t ActorView::reject_firing_position_by_pursuit(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    using namespace actor_reject_firing_position_by_pursuit_local;
    actor *self;
    int32_t tick;
    int32_t last_tick;
    int32_t count_out;
    int16_t sighting_count;
    uint8_t missed;
    float bonus;

    tick = halo::game::globals().game_time->game_time;
    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    last_tick = -1;
    count_out = 0;

    if (candidate == (actor_firing_position_candidate *)0) {
        return 0;
    }

    if (candidate->request_result != 0 || candidate->distance_from_actor >= 6.0f) {
        int16_t count16 = 0;
        missed = (uint8_t)(halo::ai::ai_pursuit_check_object(actor_index, self->encounter_index, candidate->firing_position_index,
            *(int32_t *)((uint8_t *)query + 0xc), 0, &count16, (uint32_t *)&last_tick) == 0);
        sighting_count = count16;
    } else {
        halo::ai::ai_pursuit_note_object(actor_index, self->encounter_index, candidate->firing_position_index,
            *(int32_t *)((uint8_t *)query + 0xc));
        sighting_count = 7;
        missed = 0;
        last_tick = tick;
    }

    if (query->score_instead_of_reject == 0) {
        if (missed != 0) {
            candidate->rejected = 1;
            if (query->collect_all == 0) {
                candidate->valid = 0;
            }
        }
    } else if (missed != 0) {
        candidate->score = candidate->score + 15.0f;
    }

    if (candidate->valid != 0) {
        bonus = 0.0f;
        if (last_tick == -1 || last_tick + 300 < tick) {
            bonus = 10.0f;
        } else if (last_tick < tick) {
            bonus = (float)(tick - last_tick) * 0.033333335f;
        }
        candidate->score = candidate->score + bonus;

        bonus = 0.0f;
        if (sighting_count < 4) {
            bonus = (float)(4 - sighting_count) * 5.0f;
        }
        candidate->score = candidate->score + bonus;
    }

    return candidate->valid;
}

/**
 * Actor AI behaviour: reject firing position by request result.
 *
 * @address 0x412620
 */
uint8_t ActorView::reject_firing_position_by_request_result(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    float bonus;

    if (query->have_target != 0) {
        if (candidate == (actor_firing_position_candidate *)0) {
            if (query->target_is_large != 0) {
                query->baseline_penalty = query->baseline_penalty + 6.0f;
            } else {
                query->baseline_penalty = query->baseline_penalty + 15.0f;
            }
            return 1;
        }

        bonus = 0.0f;
        if (candidate->request_result == 0) {
            bonus = (query->target_is_large != 0) ? 6.0f : 15.0f;
        } else if (candidate->request_result == 1) {
            bonus = (query->target_is_large != 0) ? 2.5f : 5.0f;
        } else if (query->target_is_large == 0) {
            candidate->rejected = 1;
            if (query->collect_all == 0) {
                candidate->valid = 0;
            }
        }
        candidate->score = bonus + candidate->score;
    }

    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}

/**
 * Actor AI behaviour: reject firing position by target approach.
 *
 * @address 0x412570
 */
uint8_t ActorView::reject_firing_position_by_target_approach(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    (void)actor_index;
    if (query->have_target != 0) {
        float value = 0.0f;

        if (candidate == (actor_firing_position_candidate *)0) {
            query->baseline_penalty = query->baseline_penalty + 20.0f;
            return 1;
        }
        if (candidate->request_result == 0) {
            value = 20.0f;
        } else if (candidate->request_result == 1) {
            value = 10.0f;
        } else {
            float d = query->target_distance - 7.5f;

            if (!(d >= 0.0f) || !(candidate->distance_squared_to_target <= d * d)) {
                candidate->rejected = 1;
                if (query->collect_all == 0) {
                    candidate->valid = 0;
                }
            }
        }
        candidate->score = value + candidate->score;
    }
    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}

namespace actor_reject_firing_position_unreachable_local {
}

/**
 * The always-on rejection rule. Ground actors pass unconditionally because their candidates were already filled
 * in by a real pathfind. A flying actor has to prove it can fly to the position: if both direct tests pass the
 * candidate keeps its place with a flat 15.0 penalty, otherwise it is rejected (or
 *
 * @address 0x412290
 */
uint8_t ActorView::reject_firing_position_unreachable(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    using namespace actor_reject_firing_position_unreachable_local;
    if (query->flying != 0) {
        if (candidate == (actor_firing_position_candidate *)0) {
            query->baseline_penalty = query->baseline_penalty + 15.0f;
            return 1;
        }
        float avoidance_distance = 0.0f;
        actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
        const real_point3d *position = (const real_point3d *)candidate->position;

        if (halo::ai::actor_movement_flying_needs_steering(actor_index, position, &avoidance_distance) != 0 &&
            halo::ai::path_find_test_direct_reachability(position, &((struct actor *)self)->body_position, 0,
                halo::scenario::globals().structure_bsp, 0) != 0) {
            candidate->score = candidate->score + 15.0f;
            return candidate->valid;
        }
        candidate->rejected = 1;
        if (query->collect_all == 0) {
            candidate->valid = 0;
        }
    }

    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}

namespace actor_report_firing_position_request_local {
}

/**
 * Submits the movement or aim the actor would make if it took this candidate and records the perception result
 * code on the candidate. Goal kind 5 (pursuing) short-circuits: a candidate more than six units away is written
 * off as result 4 without asking. Otherwise the request is mode 2 for goal kinds 1
 *
 * @address 0x4120f0
 */
void ActorView::report_firing_position_request(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    using namespace actor_report_firing_position_request_local;
    actor *self;
    real_vector3d facing;
    real_point3d *point;
    real_vector3d *direction;
    void *offset;
    real_point3d marker_point;
    uint32_t mode;
    uint32_t kind;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    if (query->goal_kind == 5) {
        if (candidate->distance_from_actor < 6.0f) {
            halo::units::unit_add_marker_relative_offset(self->unit_index, 1, (float *)candidate->position, 0, 0, &marker_point);
            candidate->request_result = (int16_t)halo::ai::actor_evaluate_engagement_reachability(
                *(int16_t *)((uint8_t *)self + 0x148), *(int16_t *)((uint8_t *)candidate->position + 0xe),
                &marker_point, &((struct actor *)self)->aim_origin, 0, 0, halo::k_dword_none,
                self->active_unit_index != (datum_index)halo::k_dword_none);
            return;
        }
        candidate->request_result = 4;
        return;
    }

    offset = (void *)0;
    direction = (real_vector3d *)0;

    if (query->goal_kind == 1 || query->goal_kind == 2) {
        mode = 2;
    } else if (query->have_standing_gun_offset != 0) {
        point = (real_point3d *)candidate->position;
        mode = 3;
        offset = &query->standing_gun_offset;
        facing.i = query->target_aim_position.x - point->x;
        facing.j = query->target_aim_position.y - point->y;
        facing.k = query->target_aim_position.z - point->z;
        if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&facing)) <= 0.0f) {
            direction = (real_vector3d *)&self->facing;
        } else {
            facing.k = 0.0f;
            direction = &facing;
        }
    } else {
        mode = 1;
    }

    halo::units::unit_add_marker_relative_offset(self->unit_index, mode, (float *)candidate->position, (uint32_t)direction,
        (uint32_t)offset, &marker_point);

    kind = (query->goal_kind >= 1 && query->goal_kind <= 3) ? 1 : 0;
    candidate->request_result = (int16_t)halo::ai::actor_evaluate_engagement_reachability(
        *(int16_t *)((uint8_t *)candidate->position + 0xe), ((struct actor_firing_position_query *)query)->target_cluster_index,
        (real_point3d *)((uint8_t *)query + 0x61c), &marker_point, (int16_t)kind, 1,
        (uint32_t)query->target_relationship_object, self->active_unit_index != (datum_index)halo::k_dword_none);
}

namespace actor_score_firing_positions_by_history_local {
}

/**
 * Two desirability terms. The first rewards staying close: candidates nearer the actor score higher, with the
 * whole effect faded out as the threat gets past the maximum firing distance. The second asks the per-hazard
 * rater about every kind 0 and kind 1 hazard the query collected; a strong kind 1 ratin
 *
 * @address 0x411ee0
 */
void ActorView::score_firing_positions_by_history(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    using namespace actor_score_firing_positions_by_history_local;
    ActorVariant *variant;
    actor_firing_position_candidate *c;
    float travel_weight;
    float fraction;
    float bonus;
    int16_t best_kind_0;
    int16_t best_kind_1;
    int16_t rating;
    int32_t i;
    int32_t j;

    variant = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);

    travel_weight = 8.0f;
    if (query->have_target == 0 ||
        (query->target_distance <= variant->maximum_firing_distance &&
         (travel_weight = (1.0f - query->target_distance / variant->maximum_firing_distance) * 8.0f,
          travel_weight > 0.0f))) {
        for (i = 0; i < (int16_t)count; i++) {
            c = &candidates[i];
            if (c->valid != 0 && c->distance_from_actor < query->maximum_distance) {
                fraction = 1.0f - c->distance_from_actor / query->maximum_distance;
                if (fraction < 0.0f) {
                    fraction = 0.0f;
                }
                c->score = fraction * travel_weight + c->score;
            }
        }
    }

    if (query->hazard_count_kind_01 <= 0) {
        return;
    }

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }

        best_kind_0 = 0;
        best_kind_1 = 0;

        if (query->hazard_count < 1) {
            bonus = 10.0f;
        } else {
            for (j = 0; j < query->hazard_count; j++) {
                int16_t kind = query->hazards[j].kind;
                if (kind != 0 && kind != 1) {
                    continue;
                }
                rating = halo::ai::actor_evaluate_flank_offset(&query->hazards[j].direction, 0,
                    (real_point3d *)c->position, &query->hazards[j].position);
                kind = query->hazards[j].kind;
                if (kind == 0) {
                    if (rating > best_kind_0) {
                        best_kind_0 = rating;
                    }
                } else if (kind == 1 && best_kind_1 <= rating) {
                    best_kind_1 = rating;
                }
            }

            if (best_kind_1 >= 2) {
                bonus = 0.0f;
            } else if (best_kind_1 >= 1) {
                bonus = 1.5f;
            } else if (best_kind_0 >= 2) {
                bonus = 6.0f;
            } else if (best_kind_0 >= 1) {
                bonus = 8.5f;
            } else {
                bonus = 10.0f;
            }
        }

        c->score = bonus + c->score;
    }
}

namespace actor_score_firing_positions_by_range_local {
extern "C" {
extern double sqrt(double x);
}
}

/**
 * Rates candidates on distance to the threat and on clearance from the nearest avoidance plane. Scores are
 * desirability, so a bigger term is a better candidate: the first term peaks inside 80 percent of the maximum
 * firing distance, the second peaks at the preferred combat range while respecting the th
 *
 * @address 0x411bf0
 */
void ActorView::score_firing_positions_by_range(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    using namespace actor_score_firing_positions_by_range_local;
    actor *self;
    ActorVariant *variant;
    actor_firing_position_candidate *c;
    void *weapon_definition;
    real_point3d *p;
    float distance;
    float threshold;
    float preferred_range;
    float minimum_range;
    float margin;
    float bonus;
    float nearest_plane;
    float dot;
    float ax, ay, az;
    int32_t i;
    int32_t j;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    variant = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }

        if (query->have_target != 0) {
            distance = (float)sqrt((double)c->distance_squared_to_target);

            if (variant->maximum_firing_distance > 0.0f) {
                threshold = variant->maximum_firing_distance * 0.8f;
                bonus = (threshold <= distance) ? (threshold / distance) * 10.0f : 10.0f;
                c->score = bonus + c->score;
            }

            if (variant->desired_combat_range[1] > 0.0f &&
                distance < variant->desired_combat_range[1]) {
                preferred_range = (self->berserking != 0) ? variant->berserk_firing_ranges[1]
                                                           : variant->desired_combat_range[1];
                weapon_definition = halo::ai::actor_get_threat_weapon_definition(actor_index);
                minimum_range = 0.0f;
                if (weapon_definition != (void *)0 &&
                    *(float *)((uint8_t *)weapon_definition + 0x40c) > 0.0f &&
                    minimum_range <= *(float *)((uint8_t *)weapon_definition + 0x40c)) {
                    minimum_range = *(float *)((uint8_t *)weapon_definition + 0x40c);
                }
                margin = preferred_range - distance;
                if (minimum_range > 0.0f && distance - minimum_range < margin) {
                    margin = distance - minimum_range;
                }
                if (margin <= 2.0f) {
                    bonus = (margin > 0.0f) ? margin * 0.5f * 20.0f : 0.0f;
                } else {
                    bonus = 20.0f;
                }
                c->score = bonus + c->score;
            }
        }

        if (query->hazard_count_kind_2 > 0) {
            nearest_plane = 3.4028235e+38f;
            for (j = 0; j < query->hazard_count; j++) {
                actor_firing_position_hazard *h = &query->hazards[j];
                if (h->kind != 2) {
                    continue;
                }
                p = (real_point3d *)c->position;
                dot = (p->x - h->position.x) * h->direction.i +
                      (p->z - h->position.z) * h->direction.k +
                      (p->y - h->position.y) * h->direction.j;
                if (dot <= 0.0f) {
                    continue;
                }
                dot = -dot;
                ax = dot * h->direction.i + (p->x - h->position.x);
                ay = dot * h->direction.j + (p->y - h->position.y);
                az = dot * h->direction.k + (p->z - h->position.z);
                dot = ax * ax + ay * ay + az * az;
                if (dot < nearest_plane) {
                    nearest_plane = dot;
                }
            }
            bonus = (nearest_plane < 12.25f) ? (float)sqrt((double)nearest_plane) * 0.25f : 6.0f;
            c->score = bonus + c->score;
        }
    }
}

namespace actor_score_firing_positions_by_standoff_local {
extern "C" {
extern double sqrt(double x);
}
}

/**
 * Actor AI behaviour: score firing positions by standoff.
 *
 * @address 0x411980
 */
void ActorView::score_firing_positions_by_standoff(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    using namespace actor_score_firing_positions_by_standoff_local;
    int32_t i;

    (void)actor_index;
    if ((int16_t)count <= 0) {
        return;
    }
    for (i = 0; i < (int32_t)count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        float base;
        float r;

        if (c->valid == 0) {
            continue;
        }
        if (!(c->distance_from_actor >= 4.0f)) {
            c->rejected = 1;
            if (query->collect_all == 0) {
                c->valid = 0;
                continue;
            }
        } else {
            float value = 0.0f;

            if (!(c->distance_from_actor >= 8.0f)) {
                value = c->distance_from_actor - 4.0f;
                value = value + value;
            } else if (!(c->distance_from_actor >= query->maximum_distance)) {
                value = (query->maximum_distance - c->distance_from_actor) * 8.0f / (query->maximum_distance - 8.0f);
            }
            c->score = value + c->score;
        }

        if (query->have_target == 0) {
            continue;
        }
        if (!(c->distance_squared_to_target >= 16.0f)) {
            c->rejected = 1;
            if (query->collect_all == 0) {
                c->valid = 0;
                continue;
            }
        }
        {
            float value;

            if (!(c->distance_squared_to_target >= 16.0f)) {
                value = 0.0f;
            } else if (!(c->distance_squared_to_target >= 49.0f)) {
                value = ((float)sqrt((double)c->distance_squared_to_target) - 4.0f) * 3.3333333f;
            } else {
                value = 10.0f;
            }
            base = value + c->score;
            c->score = base;
        }

        if (c->segment_distance >= 3.4028235e+38f) {
            continue;
        }
        if (query->target_distance <= 0.0f || query->target_distance >= 3.4028235e+38f) {
            continue;
        }
        r = 1.0f - c->segment_distance / (query->target_distance * 0.8f);
        if (r > 0.5f) {
            c->rejected = 1;
            if (query->collect_all == 0) {
                c->valid = 0;
                continue;
            }
        }
        if (!(r >= 0.0f)) {
            r = 0.0f;
        } else if (r > 1.0f) {
            r = 1.0f;
        }
        c->score = (1.0f - r) * 8.0f + base;
    }
}

namespace actor_score_firing_positions_by_threat_local {
extern "C" {
extern double sqrt(double x);
}
}

/**
 * The always-on scoring rule. Scores are desirability, so every term below RAISES the score of a good candidate.
 * It throws out candidates the actor has already recognized, rewards candidates that keep their distance from
 * the registered danger (rejecting the ones inside its inner radius outright), appl
 *
 * @address 0x4112b0
 */
void ActorView::score_firing_positions_by_threat(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    using namespace actor_score_firing_positions_by_threat_local;
    actor *self;
    actor_firing_position_candidate *c;
    real_point3d *p;
    real_vector3d segment;
    real_point3d vehicle_position;
    object *vehicle;
    float dx, dy, dz;
    float distance_squared;
    float radius;
    float bonus;
    float best_ratio;
    float ratio;
    float cosine;
    int16_t k;
    int32_t i;
    int32_t j;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }

        if (c->firing_position_index != -1) {
            for (k = 0; k < 4; k++) {
                if (c->firing_position_index == self->recognition[k].firing_position_index) {
                    c->rejected = 1;
                    if (query->collect_all == 0) {
                        c->valid = 0;
                        goto next_candidate;
                    }
                    break;
                }
            }
        }

        if (query->danger_active != 0) {
            p = (real_point3d *)c->position;
            segment.i = self->danger_segment_end.x - self->flee_from_point.x;
            segment.j = self->danger_segment_end.y - self->flee_from_point.y;
            segment.k = self->danger_segment_end.z - self->flee_from_point.z;

            dx = p->x - self->danger_center.x;
            dy = p->y - self->danger_center.y;
            dz = p->z - self->danger_center.z;
            radius = self->danger_radius + 2.5f;
            if (dx * dx + dy * dy + dz * dz < radius * radius) {
                distance_squared = halo::math::point3d_distance_squared_to_segment(self->flee_from_point, segment, *p);
                bonus = 0.0f;
                if (self->danger_object_radius * self->danger_object_radius <= distance_squared) {
                    radius = self->danger_object_radius + 2.5f;
                    if (radius * radius <= distance_squared) {
                        bonus = 20.0f;
                    } else {
                        bonus = ((float)sqrt((double)distance_squared) - self->danger_object_radius) * 8.0f;
                    }
                } else {
                    c->rejected = 1;
                    if (query->collect_all == 0) {
                        c->valid = 0;
                        goto next_candidate;
                    }
                }
                c->score = bonus + c->score;
            }

            real_vector3d scaled_direction;

            scaled_direction.i = c->direction_from_actor.i * 3.0f;
            scaled_direction.j = c->direction_from_actor.j * 3.0f;
            scaled_direction.k = c->direction_from_actor.k * 3.0f;
            dx = self->body_position.x - self->danger_center.x;
            dy = self->body_position.y - self->danger_center.y;
            dz = self->body_position.z - self->danger_center.z;
            radius = self->danger_radius + 3.0f;
            if (dx * dx + dy * dy + dz * dz < radius * radius &&
                self->danger_object_radius < self->danger_distance &&
                self->danger_object_radius * self->danger_object_radius <
                    halo::math::point3d_distance_squared_to_segment(self->flee_from_point, segment, self->body_position) &&
                0.0001f < (c->direction_from_actor.i * 3.0f) * (c->direction_from_actor.i * 3.0f) +
                          (c->direction_from_actor.j * 3.0f) * (c->direction_from_actor.j * 3.0f) +
                          (c->direction_from_actor.k * 3.0f) * (c->direction_from_actor.k * 3.0f) &&
                halo::math::segment3d_distance_squared_to_segment(&self->flee_from_point, &self->body_position, &scaled_direction,
                    &segment) <
                    self->danger_object_radius * self->danger_object_radius) {
                c->rejected = 1;
                if (query->collect_all == 0) {
                    c->valid = 0;
                    goto next_candidate;
                }
            }
        }

        p = (real_point3d *)c->position;
        if ((query->marked_group_mask & (1u << (((uint8_t *)p)[12] & 0x1f))) != 0) {
            c->score = query->marked_group_penalty + c->score;
        }

        if (query->danger_sphere_count > 0) {
            best_ratio = 1.0f;
            for (j = 0; j < query->danger_sphere_count; j++) {
                actor_firing_position_danger_sphere *sphere = &query->danger_spheres[j];
                dx = sphere->position.x - p->x;
                dy = sphere->position.y - p->y;
                dz = sphere->position.z - p->z;
                ratio = (dx * dx + dy * dy + dz * dz) / (sphere->radius * sphere->radius);
                if (ratio < best_ratio) {
                    best_ratio = ratio;
                }
            }
            bonus = (best_ratio < 1.0f) ? (float)sqrt((double)best_ratio) * 10.0f : 10.0f;
            c->score = bonus + c->score;
        }

next_candidate:
        ;
    }

    if (query->check_vehicle_aim_cone == 0 || self->active_unit_index == (datum_index)halo::k_dword_none) {
        return;
    }

    vehicle = (object *)((object_header *)halo::objects::globals().object_data->data)[self->active_unit_index & halo::k_slot_mask].data;
    halo::objects::object_get_position(&vehicle_position, self->active_unit_index);

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }
        p = (real_point3d *)c->position;
        dx = p->x - vehicle_position.x;
        dy = p->y - vehicle_position.y;
        dz = p->z - vehicle_position.z;
        distance_squared = dx * dx + dy * dy + dz * dz;
        if (distance_squared < 0.0001f || distance_squared >= 900.0f) {
            continue;
        }
        cosine = (dx * ((vehicle_object *)vehicle)->base.forward.i +
                  dy * ((vehicle_object *)vehicle)->base.forward.j +
                  dz * ((vehicle_object *)vehicle)->base.forward.k) / (float)sqrt((double)distance_squared);

        if ((query->vehicle_ignore_velocity == 0 &&
             ((vehicle_object *)vehicle)->base.velocity.k * ((vehicle_object *)vehicle)->base.velocity.k +
             ((vehicle_object *)vehicle)->base.velocity.j * ((vehicle_object *)vehicle)->base.velocity.j +
             ((vehicle_object *)vehicle)->base.velocity.i * ((vehicle_object *)vehicle)->base.velocity.i <=
                 0.0069444445f) ||
            distance_squared >= 64.0f || cosine >= 0.70710677f ||
            (c->rejected = 1, query->collect_all != 0)) {
            if (cosine >= 0.0f) {
                bonus = 15.0f;
                if (cosine > 0.8660254f) {
                    bonus = (cosine - 0.8660254f) * 111.96151f + 15.0f;
                }
            } else {
                bonus = 15.0f + cosine * 15.0f;
            }
            c->score = bonus + c->score;
        } else {
            c->valid = 0;
        }
    }
}

/**
 * Actor AI behaviour: score firing positions close range.
 *
 * @address 0x411b60
 */
void ActorView::score_firing_positions_close_range(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    int32_t i;

    (void)actor_index;
    if ((int16_t)count <= 0) {
        return;
    }
    for (i = 0; i < (int32_t)count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        float value = 0.0f;

        if (c->valid == 0) {
            continue;
        }
        if (query->maximum_distance * 0.5f > c->distance_from_actor) {
            value = 8.0f;
        } else if (!(c->distance_from_actor >= query->maximum_distance)) {
            value = (1.0f / (query->maximum_distance * 0.5f)) * (query->maximum_distance - c->distance_from_actor) * 8.0f;
        }
        c->score = value + c->score;
    }
}

/**
 * Actor AI behaviour: score firing positions near target.
 *
 * @address 0x411840
 */
void ActorView::score_firing_positions_near_target(actor_firing_position_query *query, uint16_t count, actor_firing_position_candidate *candidates)
{
    int32_t i;

    (void)actor_index;
    if ((int16_t)count <= 0) {
        return;
    }
    for (i = 0; i < (int32_t)count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        float value = 0.0f;
        float base;

        if (c->valid == 0) {
            continue;
        }
        if (query->maximum_distance * 0.5f > c->distance_from_actor) {
            value = 5.0f;
        } else if (!(c->distance_from_actor >= query->maximum_distance)) {
            value = (1.0f / (query->maximum_distance * 0.5f)) * (query->maximum_distance - c->distance_from_actor) * 5.0f;
        }
        base = value + c->score;
        c->score = base;
        if (query->have_target == 0) {
            continue;
        }
        if (!(c->distance_from_target >= 20.0f)) {
            c->score = (20.0f - c->distance_from_target) * 0.5f + base;
        }
        if (query->want_direction_from_target == 0 || query->have_target_vault_point == 0) {
            continue;
        }
        {
            float dot = c->direction_from_target.k * query->target_vault_point.z +
                        c->direction_from_target.i * query->target_vault_point.x +
                        query->target_vault_point.y * c->direction_from_target.j;

            if (dot > 0.70710677f) {
                value = 10.0f;
            } else {
                value = dot * 1.4142135f;
                if (0.0f > value) {
                    value = 0.0f;
                }
                value = value * 10.0f;
            }
            c->score = value + c->score;
        }
    }
}

namespace actor_select_firing_position_local {
static auto &global_origin3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
}

/**
 * path_context, out_path_ok (the goal kind is query +0x04) Picks a firing position for the actor. When the mask
 * for the actor current searching state is a strict subset of the union of both states, the wider union becomes
 * the hard filter and the narrower own-state mask becomes a soft preference worth
 *
 * @address 0x413e50
 */
int16_t ActorView::select_firing_position(actor_firing_position_query *query, actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context, uint8_t *out_path_ok)
{
    using namespace actor_select_firing_position_local;
    int16_t kind = query->goal_kind;
    actor *self;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    uint32_t own_mask;
    uint32_t not_searching_mask;
    uint32_t searching_mask;
    int16_t result;
    int16_t held;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    if (self->encounter_index == (datum_index)halo::k_dword_none) {
        return -1;
    }

    own_mask = halo::ai::actor_get_firing_position_group_mask(actor_index, kind, 0);
    not_searching_mask = halo::ai::actor_get_firing_position_group_mask(actor_index, kind, 2);
    searching_mask = halo::ai::actor_get_firing_position_group_mask(actor_index, kind, 1);

    if (own_mask < (not_searching_mask | searching_mask)) {
        query->marked_group_mask = own_mask;
        query->marked_group_penalty = 8.0f;
        query->group_mask = not_searching_mask | searching_mask;
    } else {
        query->group_mask = own_mask;
    }

    query->allow_random_fallback =
        (uint8_t)(self->firing_position_index == -1 || self->firing_position_without_path == 0);
    query->collect_all = 1;

    result = (int16_t)halo::ai::actor_find_best_firing_position(actor_index, query, out_candidate,
                                                      out_previous_owner, path_context, out_path_ok);

    if (result == -1) {
        held = self->firing_position_index;
        if (held == -1 || self->firing_position_without_path == 0) {
            return result;
        }

        encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                                    [self->encounter_index & halo::k_slot_mask];
        firing_positions =
            (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;

        out_candidate->position = (uint32_t)(uint8_t *)&firing_positions[held];
        out_candidate->distance_from_actor = 3.4028235e+38f;
        out_candidate->distance_from_target = 3.4028235e+38f;
        out_candidate->segment_distance = 3.4028235e+38f;
        out_candidate->firing_position_index = held;
        out_candidate->request_result = 0;
        out_candidate->direction_from_target.i = global_origin3d_pointer->i;
        out_candidate->direction_from_target.j = global_origin3d_pointer->j;
        out_candidate->direction_from_target.k = global_origin3d_pointer->k;
        out_candidate->direction_from_actor.i = global_origin3d_pointer->i;
        out_candidate->direction_from_actor.j = global_origin3d_pointer->j;
        out_candidate->direction_from_actor.k = global_origin3d_pointer->k;
        if (query->have_target == 0) {
            out_candidate->distance_squared_to_target = 0.0f;
        } else {
            out_candidate->distance_squared_to_target = halo::math::vector3d_distance_squared(
                *((real_point3d *)&firing_positions[held]), query->target_position);
        }

        if (halo::ai::actor_firing_position_evaluate(out_candidate, query, actor_index) == 0) {
            held = -1;
            self->firing_position_index = -1;
        }
        *out_previous_owner = halo::k_dword_none;
        *out_path_ok = 0;
        return held;
    }

    if ((own_mask & (1u << (((uint8_t *)out_candidate->position)[0xc] & 0x1f))) == 0) {
        self->search_firing_positions = (uint8_t)(self->search_firing_positions == 0);
    }
    return result;
}

namespace actor_select_move_position_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * REWRITTEN from objdump 0x4014c0..0x4017a7. EAX: actor; stack: (mode, current index, direction byte *). Picks
 * one of the actor's squad move positions (ScenarioSquad +0xc4 count / +0xc8 block, 0x50 each). A position is
 * taken out (mask bit) when it is the current one, within 0.5 of the actor, of anothe
 *
 * @address 0x4014c0
 */
int32_t ActorView::select_move_position(int16_t select_mode, int32_t position_index, uint8_t *direction_flag)
{
    using namespace actor_select_move_position_local;
    struct actor *a = halo::ai::actor_at(actor_index);
    uint8_t *squad;
    uint8_t *positions;
    int32_t count;
    uint32_t mask = 0;
    uint8_t found = 0;
    int16_t current = (int16_t)position_index;
    int16_t i;
    int16_t index;

    if (a->order_committed || select_mode == 0) {
        return -1;
    }
    if (a->encounter_index == k_datum_index_none) {
        return -1;
    }
    squad = *(uint8_t **)(*(uint8_t **)((uint8_t *)halo::scenario::globals().scenario + 0x430) +
        (a->encounter_index & halo::k_slot_mask) * 0xb0 + 0x84) + a->squad_index * 0xe8;
    if (select_mode == 1 && current != -1) {
        return position_index;
    }
    count = *(int32_t *)(squad + 0xc4);
    if (count <= 0) {
        return -1;
    }
    positions = *(uint8_t **)(squad + 0xc8);
    for (i = 0; (int32_t)i < count; i++) {
        float *pos = (float *)(positions + i * 0x50);
        uint8_t eligible = (i != current);
        uint8_t occupied = 0;
        datum_index prop_index;

        if (current != -1) {
            float dx = pos[0] - a->body_position.x;
            float dy = pos[1] - a->body_position.y;
            float dz = pos[2] - a->body_position.z;

            if (!(dz * dz + dy * dy + dx * dx >= 0.25f)) {
                eligible = 0;
            }
        }
        if (((uint8_t *)pos)[0x1e] && ((uint8_t *)pos)[0x1e] != a->sequence_id) {
            eligible = 0;
        }
        for (prop_index = a->first_prop; prop_index != k_datum_index_none;) {
            prop *pr = halo::ai::prop_at(prop_index);
            int16_t kind = pr->state;

            prop_index = pr->next_in_actor;
            if (kind >= 2 && kind <= 3) {
                float dx = pos[0] - pr->last_known_position.x;
                float dy = pos[1] - pr->last_known_position.y;
                float dz = pos[2] - pr->last_known_position.z;

                if (!(dz * dz + dy * dy + dx * dx >= 0.25f)) {
                    occupied = 1;
                    break;
                }
            }
        }
        if (occupied || !eligible) {
            (&mask)[i >> 5] |= 1u << (i & 0x1f);
        } else {
            found = 1;
        }
    }
    if (!found) {
        return -1;
    }
    if (select_mode == 5) {
        return halo::ai::ai_weighted_random_index(0x10, positions, 0x50, (uint16_t)*(int32_t *)(squad + 0xc4), &mask);
    }
    index = current;
    if (index < 0 || (int32_t)index >= count) {
        index = 0;
    }
    do {
        uint8_t forward = 1;
        uint8_t store = 1;

        if (select_mode == 2) {
            forward = 1;
        } else if (select_mode == 3) {
            if (index == 0) {
                forward = 1;
            } else if ((int32_t)index == *(int32_t *)(squad + 0xc4) - 1) {
                forward = 0;
            } else if (direction_flag != 0) {
                forward = *direction_flag;
            } else {
                store = 0;
            }
        } else if (select_mode == 4) {
            forward = (uint8_t)(halo::game::globals().game_time->game_time & 1);
        }
        if (store && direction_flag != 0) {
            *direction_flag = forward;
        }
        if (forward) {
            index++;
            if ((int32_t)index >= *(int32_t *)(squad + 0xc4)) {
                index = 0;
            }
        } else {
            index--;
            if (index < 0) {
                index = (int16_t)(*(int32_t *)(squad + 0xc4) - 1);
            }
        }
    } while ((&mask)[index >> 5] & (1u << (index & 0x1f)));
    return index;
}

}
