#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

namespace halo::ai {

namespace actor_movement_action_cancel_local {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
}
}

/**
 * Actor AI behaviour: movement action cancel.
 *
 * @address 0x428650
 */
void ActorView::movement_action_cancel()
{
    using namespace actor_movement_action_cancel_local;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    int16_t movement_type = self->active_movement.type;

    self->firing_position_index = -1;
    if (movement_type == 3 || movement_type == 4) {
        self->active_movement.type = 0;
        self->active_movement.extra = 0xffffffff;
    }

    {
        uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0x24);
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }
}

namespace actor_movement_action_complete_local {
extern "C" {
extern data_array *actor_data;
}
}

/**
 * Actor AI behaviour: movement action complete.
 *
 * @address 0x41a430
 */
void ActorView::run_movement_action_complete()
{
    using namespace actor_movement_action_complete_local;
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->movement_action_complete = 0;
    self->movement_completed = 1;
    self->movement_timer = 0;
}

namespace actor_movement_action_in_progress_local {
extern "C" {
extern data_array *actor_data;
}
}

/**
 * Actor AI behaviour: movement action in progress.
 *
 * @address 0x41a980
 */
uint8_t ActorView::movement_action_in_progress()
{
    using namespace actor_movement_action_in_progress_local;
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->movement_action_complete != 0 && self->movement_completed == 0) {
        return 0;
    }
    return 1;
}

namespace actor_movement_action_is_complete_local {
extern "C" {
extern data_array *actor_data;
}
}

/**
 * Actor AI behaviour: movement action is complete.
 *
 * @address 0x41a960
 */
uint8_t ActorView::movement_action_is_complete()
{
    using namespace actor_movement_action_is_complete_local;
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    return self->movement_action_complete;
}

namespace actor_movement_action_resolve_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern Scenario *global_scenario;
extern ScenarioStructureBSP *global_structure_bsp;
extern uint8_t actor_movement_check_arrival(datum_index actor_index);
extern void actor_movement_action_complete(datum_index actor_index);
extern void actor_build_path_find_request(datum_index actor_index, path_find_request *request);
extern uint8_t actor_movement_flying_needs_steering(datum_index actor_index, const real_point3d *destination,
                                                   float *out_avoidance_distance);
extern void actor_target_get_relationship_object(datum_index target_prop_index);
extern void path_find_set_avoid_sphere(path_find_request *request, const real_point3d *center, float radius,
                         datum_index object_index, float weight);
extern uint8_t path_find_validate_and_record_goal(void *candidate, void *context, uint32_t unused_b,
    uint32_t unused_c, const real_point3d *position);
extern uint8_t path_find_reconstruct_path(path_find_context *context, uint8_t *out_reachable);
extern void path_find_context_init(path_find_context *context, const path_find_request *request,
                                   int32_t flags);
extern void path_find_set_goal(path_find_context *context, const real_point3d *position, uint32_t goal_vertex_id, float goal_cost);
extern uint8_t path_find_run(path_find_context *context);
}
}

/**
 * Actor AI behaviour: movement action resolve.
 *
 * @address 0x41a460
 */
uint8_t ActorView::movement_action_resolve(uint8_t record_distance, path_find_context *context)
{
    using namespace actor_movement_action_resolve_local;
    actor *self;
    Actor *actor_definition;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    ScenarioMovePosition *move_position;
    ScenarioFiringPosition *firing_position;
    prop *target;
    int16_t type;
    int16_t index;
    uint8_t result;
    uint8_t have_previous;
    real_point3d previous_destination;
    float distance;
    float avoidance_distance;
    path_find_request request;
    path_find_context local_context;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    type = self->active_movement.type;
    result = 1;
    have_previous = 0;
    previous_destination.x = 0.0f;
    previous_destination.y = 0.0f;
    previous_destination.z = 0.0f;
    if (type != 0 && type != 1) {
        previous_destination = self->destination;
        have_previous = 1;
    }

    if (self->order_committed != 0 || type == 0 || type == 1 ||
        (type == 3 && self->grenade_evasion_active != 0)) {
        self->movement_action_complete = 0;
        self->movement_timer = 0;
        self->movement_completed = 1;
        return 1;
    }

    self->movement_action_complete = 0;
    self->movement_completed = 0;
    self->movement_timer = 0;
    self->waypoint_reached = 0;

    switch (type) {
    case 2:
        self->destination = self->active_movement.destination;
        self->destination_surface_index = (uint32_t)self->active_movement.parameter;
        self->destination_radius = 0;
        break;

    case 3:
        if (self->encounter_index == (datum_index)k_datum_index_none) {
            result = 0;
            actor_movement_action_complete(actor_index);
            return result;
        }
        encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                   [self->encounter_index & 0xffff];
        firing_position = &((ScenarioFiringPosition *)encounter_definition->firing_positions.pointer)
                              [*(int16_t *)&self->active_movement.destination];
        self->destination = *(real_point3d *)&firing_position->position;
        self->destination_surface_index = firing_position->surface_index;
        self->destination_radius = 0;
        break;

    case 4:
        result = 0;
        if (self->encounter_index == (datum_index)k_datum_index_none) {
            actor_movement_action_complete(actor_index);
            return result;
        }
        encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                   [self->encounter_index & 0xffff];
        squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)
                               [self->squad_index];
        index = *(int16_t *)&self->active_movement.destination;
        if (index < 0 || (int32_t)index >= (int32_t)squad_definition->move_positions.count) {
            actor_movement_action_complete(actor_index);
            return result;
        }
        move_position = &((ScenarioMovePosition *)squad_definition->move_positions.pointer)[index];
        self->destination = *(real_point3d *)&move_position->position;
        self->destination_surface_index = move_position->surface_index;
        result = 1;
        self->destination_radius = 0;
        break;

    case 5:
        target = &((prop *)prop_data->data)[*(uint32_t *)&self->active_movement.destination & 0xffff];
        if (target->state < 4 || target->state > 5) {
            actor_target_get_relationship_object(*(datum_index *)&self->active_movement.destination);
        }
        if (self->flying != 0) {
            self->destination = *(real_point3d *)&((struct prop *)target)->center_of_mass.x;
        } else {
            self->destination = *(real_point3d *)&((struct prop *)target)->pathfinding_point.x;
        }
        self->destination_surface_index = *(uint32_t *)&((struct prop *)target)->pathfinding_surface_index;
        self->destination_radius = *(uint32_t *)&self->active_movement.destination.y;
        break;

    default:
        result = 0;
        actor_movement_action_complete(actor_index);
        return result;
    }

    if (self->flying == 0) {
        if (*(float *)&self->destination_radius == 0.0f &&
            self->destination_surface_index == (uint32_t)-1) {
            result = 0;
            actor_movement_action_complete(actor_index);
            return result;
        }
    } else {
        if (actor_movement_flying_needs_steering(actor_index, &self->destination,
                                                 &avoidance_distance) == 0) {
            result = 0;
            actor_movement_action_complete(actor_index);
            return result;
        }
    }

    if (actor_movement_check_arrival(actor_index) != 0) {
        if (have_previous == 0) {
            return result;
        }
        if (halo::math::vector3d_distance_squared(self->destination, previous_destination) <= 0.010000001f) {
            return result;
        }
    }

    actor_definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & 0xffff].data;
    distance = halo::math::vector3d_distance(self->destination, self->body_position);

    if (self->flying != 0) {
        result = path_find_validate_and_record_goal((uint8_t *)self + 0x4a8, (void *)global_structure_bsp,
            (uint32_t)&self->body_position, 0, &self->destination);
    } else if (context != (path_find_context *)0) {
        path_find_set_goal(context, &self->destination, self->destination_surface_index, self->destination_radius);
        result = path_find_reconstruct_path(context, &self->movement_action_complete);
    } else {
        actor_build_path_find_request(actor_index, &request);
        if (self->active_movement.extra != (uint32_t)-1) {
            request.exclude_object_index_b = (datum_index)self->active_movement.extra;
        }
        if (self->danger_type > 0 && self->danger_is_own == 0 &&
            (((uint8_t *)actor_definition)[4] & 0x10) == 0) {
            path_find_set_avoid_sphere(&request, &self->flee_from_point, self->danger_unknown_294,
                         self->danger_object_index, 10.0f);
        }
        path_find_context_init(&local_context, &request, 0);
        path_find_set_goal(&local_context, &self->destination, self->destination_surface_index, self->destination_radius);
        result = 0;
        if (path_find_run(&local_context) != 0 &&
            path_find_reconstruct_path(&local_context, &self->movement_action_complete) != 0) {
            result = 1;
        }
    }

    self->path_resolved_this_tick = 1;
    if (record_distance != 0) {
        *(float *)&self->movement_timer = distance;
    }

    if (result != 0) {
        if (self->path_remaining_distance <= 0.0f) {
            return result;
        }
        if (*(float *)&self->destination_radius <= distance) {
            return result;
        }
        if (distance - self->path_remaining_distance >= 0.5f) {
            return result;
        }
    }

    actor_movement_action_complete(actor_index);
    return result;
}

namespace actor_movement_action_stop_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index, int32_t parameter, uint32_t extra);
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context);
}
}

/**
 * Actor AI behaviour: movement action stop.
 *
 * @address 0x417570
 */
void ActorView::movement_action_stop()
{
    using namespace actor_movement_action_stop_local;
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->vehicle_driving_type == 4 && self->moving != 0) {
        actor_movement_set_destination_point(&self->body_position, actor_index, self->pathfinding_surface_index, (uint32_t)-1);
        return;
    }

    self->firing_position_index = -1;
    if (self->active_movement.type != 1) {
        self->queued_movement.type = 1;
        self->active_movement = self->queued_movement;
    }
    actor_movement_action_resolve(actor_index, 1, 0);
}

namespace actor_movement_actions_cancel_local {
extern "C" {
extern data_array *actor_data;
}
}

/**
 * Marks both the actor's queued and its currently active movement action as cancelled.
 *
 * @address 0x417a30
 */
void ActorView::movement_actions_cancel()
{
    using namespace actor_movement_actions_cancel_local;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    self->queued_movement.cancelled = 1;
    self->active_movement.cancelled = 1;
}

namespace actor_movement_advance_waypoint_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context);
extern uint8_t actor_movement_check_arrival(datum_index actor_index);
}
}

/**
 * Actor AI behaviour: movement advance waypoint.
 *
 * @address 0x4163e0
 */
void ActorView::movement_advance_waypoint()
{
    using namespace actor_movement_advance_waypoint_local;
    actor *self;
    uint8_t *waypoints;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->needs_new_path != 0 && self->path_resolved_this_tick == 0 && self->keep_unit_alive == 0) {
        actor_movement_action_resolve(actor_index, 0, 0);
    }
    actor_movement_check_arrival(actor_index);

    waypoints = &self->movement_action_complete;
    if (self->movement_action_complete != 0) {
        for (;;) {
            int cursor = (int)(int8_t)self->waypoint_cursor;
            float *cur;
            float *next;
            float dx, dy, ex, ey;
            uint8_t reject;

            if ((int)(int8_t)self->waypoint_count <= cursor + 1) break;

            cur = (float *)(waypoints + (cursor + 2) * 0x10);
            next = (float *)(waypoints + (cursor + 3) * 0x10);
            dx = cur[0] - self->body_position.x;
            dy = cur[1] - self->body_position.y;
            ex = next[0] - cur[0];
            ey = next[1] - cur[1];

            if (self->waypoint_reached == 0) {
                if (self->moving == 0 || self->movement_thwarted == 0) {
                    float dist2 = dx * dx + dy * dy;
                    reject = dist2 < 0.0225f;
                } else {
                    float along = ex * dx + ey * dy;
                    if (ex * self->facing.i + ey * self->facing.j <= 0.0f || 0.0f <= along) break;
                    along = -along;
                    dx = ex * along + dx;
                    dy = ey * along + dy;
                    {
                        float dist2 = dx * dx + dy * dy;
                        reject = dist2 < 0.0625f;
                    }
                }
                if (!reject) break;
            }

            self->waypoint_cursor = self->waypoint_cursor + 1;
            self->waypoint_reached = 0;
        }

        if (self->waypoint_reached != 0 && self->unknown_4c0 != 0) {
            self->movement_action_complete = 0;
            self->movement_completed = 1;
            self->movement_timer = 0;
        }

        if (*waypoints != 0 && (self->moving != 0 || self->movement_completed == 0)) {
            float *cur;
            real_point3d *target;
            self->moving = 1;
            cur = (float *)((uint8_t *)self + 0x4c8 + (int8_t)self->waypoint_cursor * 0x10);
            target = (real_point3d *)&self->current_waypoint;
            target->x = cur[0];
            target->y = cur[1];
            target->z = cur[2];
            self->desired_movement_vector.x = target->x - self->body_position.x;
            self->desired_movement_vector.y = target->y - self->body_position.y;
            self->desired_movement_vector.z = target->z - self->body_position.z;
            return;
        }
    }

    if (self->vehicle_driving_type == 4) {
        float sign = (self->avoidance_emergency <= 0.9f) ? 1.0f : -1.0f;
        float scale = sign * 3.0f;
        self->moving = 1;
        self->waypoint_reached = 0;
        self->desired_movement_vector.x = scale * self->facing.i;
        self->desired_movement_vector.y = scale * self->facing.j;
        self->desired_movement_vector.z = scale * self->facing.k;
        *(real_point3d *)&self->current_waypoint = self->body_position;
        ((real_point3d *)&self->current_waypoint)->x += self->desired_movement_vector.x;
        ((real_point3d *)&self->current_waypoint)->y += self->desired_movement_vector.y;
        ((real_point3d *)&self->current_waypoint)->z += self->desired_movement_vector.z;
        return;
    }

    self->moving = 0;
    self->waypoint_reached = 0;
    self->movement_completed = 1;
    self->movement_action_complete = 0;
    self->movement_completed = 1;
    self->movement_timer = 0;
}

namespace actor_movement_apply_steering_local {
extern "C" {
extern data_array *actor_data;
extern ScenarioStructureBSP *global_structure_bsp;
extern const real_vector3d *global_origin3d_pointer;
extern double acos(double x);
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);
extern void actor_update_target_lead_position(datum_index actor_index);
extern float actor_compute_accuracy_scale(datum_index actor_index);
extern void actor_movement_get_stopping_distances(datum_index actor_index, float *out_accelerate_stop_distance,
                                                  float *out_stop_distance);
extern void actor_movement_choose_strafe_axis(const real_vector3d *direction, uint8_t use_3d,
                                              const real_vector3d *facing, const real_vector3d *reference,
                                              real_vector3d *out_axis, int16_t *out_index);
extern void actor_movement_project_into_frame(uint8_t use_3d, const real_vector3d *frame_axis,
                                              const real_vector3d *v, real_vector3d *out);
extern uint8_t path_find_trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start, int32_t start_surface,
    real_point3d *end, int32_t target_surface, path_find_boundary_crossing *out_result);
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
}
}

/**
 * Actor AI behaviour: movement apply steering.
 *
 * @address 0x4180c0
 */
void ActorOps::movement_apply_steering(int16_t cached_axis, uint8_t keep_z, datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed, float steering_maximum, float oversteer_min, float oversteer_max, float avoidance_scale, float throttle_maximum, real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis, real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506)
{
    using namespace actor_movement_apply_steering_local;
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    real_vector3d *facing = (real_vector3d *)(act + 0x174);
    float max_turn_cos = 0.8660254f;
    int16_t chosen_axis = -1;
    real_vector3d aim;
    real_vector3d heading;
    real_vector3d desired;
    real_vector3d rotated;
    float dot_facing;
    uint8_t take_step;
    float stop_distance;
    float desired_length_squared;
    float turn_limit = throttle_maximum;

    if (act[0x42a]) {
        act[0x591] = 1;
    }
    if (cached_axis >= 0 && cached_axis <= 3) {
        chosen_axis = cached_axis;
        heading = *desired_direction;
        if (!keep_z) {
            heading.k = 0.0f;
        }
        if (halo::math::vector3d_normalize_with_length(heading) == 0.0f) {
            heading = *facing;
        }
        switch (cached_axis) {
        case 0: aim = heading; break;
        case 1: aim.i = -heading.i; aim.j = -heading.j; aim.k = heading.k; break;
        case 2: aim.i = -heading.j; aim.j = heading.i; aim.k = heading.k; break;
        default: aim.i = heading.j; aim.j = -heading.i; aim.k = heading.k; break;
        }
        if (want_avoid_check) {
            actor_movement_project_into_frame(keep_z, &aim, &heading, &rotated);
            chosen_axis = 4;
        }
    } else {
        desired_length_squared = desired_direction->i * desired_direction->i +
                                 desired_direction->j * desired_direction->j +
                                 desired_direction->k * desired_direction->k;
        if (desired_length_squared > 0.64000005f) {
            max_turn_cos = ((Actor *)actor_tag)->cosine_begin_moving_angle;
        }
        if (want_avoid_check && desired_length_squared <= avoid_threshold) {
            uint8_t use_scratch = 0;
            real_vector3d fallback;

            desired = *desired_direction;
            if (act[0x505]) {
                aim = *(real_vector3d *)&((struct actor *)act)->forced_aim_direction.i;
                if (((struct actor *)act)->vehicle_driving_type > 0) {
                    use_scratch = 1;
                }
            } else {
                aim = *facing;
            }
            if (!keep_z) {
                desired.k = 0.0f;
                aim.k = 0.0f;
            }
            if (halo::math::vector3d_normalize_with_length(aim) == 0.0f) {
                aim = *facing;
            }
            fallback = aim;
            if (halo::math::vector3d_normalize_with_length(desired) == 0.0f) {
                desired = fallback;
            }
            if (use_scratch) {
                heading = *facing;
                if (!keep_z) {
                    heading.k = 0.0f;
                }
                if (halo::math::vector3d_normalize_with_length(heading) == 0.0f) {
                    heading = fallback;
                }
                actor_movement_project_into_frame(keep_z, &heading, &desired, &rotated);
            } else {
                actor_movement_project_into_frame(keep_z, &aim, &desired, &rotated);
            }
            chosen_axis = 4;
        } else if (act[0x505]) {
            actor_movement_choose_strafe_axis(desired_direction, keep_z, facing, (real_vector3d *)(act + 0x524), &aim,
                                              &chosen_axis);
        } else {
            aim = *desired_direction;
            if (!keep_z) {
                aim.k = 0.0f;
            }
            if (halo::math::vector3d_normalize_with_length(aim) == 0.0f) {
                aim = *facing;
            }
            chosen_axis = 0;
        }
    }

    dot_facing = aim.j * facing->j + aim.k * facing->k + aim.i * facing->i;
    if (order_failed || ((struct actor *)act)->control_animation_mode == 4) {
        take_step = 1;
    } else {
        if (!act[0x99]) {
            int32_t surface;

            actor_update_target_lead_position(actor_index);
            surface = ((struct actor *)act)->pathfinding_surface_index;
            if (surface != -1 && chosen_axis >= 0 && chosen_axis <= 3) {
                real_vector3d probe;

                switch (chosen_axis) {
                case 0: probe = *facing; break;
                case 1: probe.i = -facing->i; probe.j = -facing->j; probe.k = facing->k; break;
                case 2: probe.i = facing->j; probe.j = -facing->i; probe.k = facing->k; break;
                default: probe.i = -facing->j; probe.j = facing->i; probe.k = facing->k; break;
                }
                if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&probe)) > 0.0f) {
                    real_point3d point;
                    path_find_boundary_crossing crossing;

                    point.x = probe.i * 0.4f + ((actor *)act)->body_position.x;
                    point.y = probe.j * 0.4f + ((actor *)act)->body_position.y;
                    point.z = ((actor *)act)->body_position.z;
                    if (path_find_trace_bsp_boundary(global_structure_bsp, act[0x376], (real_point3d *)(act + 0x12c),
                                                     surface, &point, -1, &crossing) &&
                        !(max_turn_cos > 0.95f)) {
                        max_turn_cos = 0.95f;
                    }
                }
            }
        }
        take_step = (uint8_t)(dot_facing > max_turn_cos);
    }

    {
        float accuracy = actor_compute_accuracy_scale(actor_index);

        desired_length_squared = desired_direction->i * desired_direction->i +
                                 desired_direction->j * desired_direction->j +
                                 desired_direction->k * desired_direction->k;
        *out_flag_506 = (uint8_t)(desired_length_squared <= accuracy * accuracy);
    }
    actor_movement_get_stopping_distances(actor_index, &max_turn_cos, &stop_distance);
    if (!act[0x46e] && stop_distance * stop_distance > desired_length_squared) {
        float distance = (float)sqrt(desired_length_squared);

        if (!(max_turn_cos + 0.05f < distance) || !(stop_distance > max_turn_cos)) {
            turn_limit = 0.0f;
        } else {
            float t = (distance - max_turn_cos) / (stop_distance - max_turn_cos);

            if (turn_limit > t) {
                turn_limit = t;
            }
        }
    }

    heading = *global_origin3d_pointer;
    if (take_step) {
        switch (chosen_axis) {
        case 0: heading.i = 1.0f; break;
        case 1: heading.i = -1.0f; break;
        case 2: heading.j = -1.0f; break;
        case 3: heading.j = 1.0f; break;
        case 4: heading = rotated; break;
        default: break;
        }
        heading.i *= turn_limit;
        heading.j *= turn_limit;
        heading.k *= turn_limit;
        *out_flag_507 = 0;
    } else {
        act[0x591] = 1;
        *out_flag_507 = 1;
    }

    if (steering_maximum > 0.0f || oversteer_max > 0.0f) {
        float target_angle;
        float angle;
        float *held = (float *)(act + 0x594);

        if (dot_facing >= 1.0f) {
            target_angle = 0.0f;
        } else if (dot_facing <= -1.0f) {
            target_angle = 3.1415927f;
        } else {
            target_angle = (float)acos(dot_facing);
        }
        angle = target_angle;
        if (steering_maximum > 0.0f) {
            float limit = steering_maximum * avoidance_scale;
            float cap = steering_maximum;

            if (avoidance_scale > 1.0f) {
                cap = (avoidance_scale > 1.5f ? 1.5f : avoidance_scale) * steering_maximum;
            }
            if (target_angle * 3.0f <= limit) {
                limit = target_angle * 3.0f;
            }
            if (target_angle < limit) {
                angle = limit;
            } else if (target_angle > cap) {
                angle = cap;
            }
        }
        if (angle > *held) {
            if (act[0x591] && angle > oversteer_min) {
                *held = angle <= oversteer_max ? angle : oversteer_max;
            }
        } else if (*held > 0.0f) {
            if (angle < oversteer_min) {
                *held = 0.0f;
            } else {
                angle = *held;
            }
        }
        {
            float step = angle - target_angle;

            if (fabs(step) > 9.999999747378752e-05) {
                real_vector3d axis;

                halo::math::vector3d_cross_product(axis, aim, *facing);
                if (halo::math::vector3d_normalize_with_length(axis) > 0.0f) {
                    halo::math::vector3d_rotate_about_axis(aim, axis, (real)sin(step), (real)cos(step));
                }
            }
        }
    }

    *out_axis = chosen_axis;
    *out_direction = aim;
    *out_heading = heading;
}

#undef TAG_DATA

namespace actor_movement_check_arrival_local {
extern "C" {
extern data_array *actor_data;
extern float actor_compute_accuracy_scale(datum_index actor_index);
}
}

/**
 * Actor AI behaviour: movement check arrival.
 *
 * @address 0x416700
 */
uint8_t ActorView::movement_check_arrival()
{
    using namespace actor_movement_check_arrival_local;
    actor *self;
    float radius;
    float dx, dy, dz;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->active_movement.type != 0 && self->active_movement.type != 1) {
        radius = actor_compute_accuracy_scale(actor_index);
        dx = self->destination.x - self->body_position.x;
        dy = self->destination.y - self->body_position.y;
        dz = self->destination.z - self->body_position.z;
        if (radius * radius <= dy * dy + dx * dx + dz * dz) {
            return self->movement_completed;
        }
    }
    self->movement_completed = 1;
    return self->movement_completed;
}

namespace actor_movement_choose_strafe_axis_local {
extern "C" {
extern const real_vector3d *global_origin3d_pointer;
}
}

/**
 * Actor AI behaviour: movement choose strafe axis.
 *
 * @address 0x418a40
 */
void ActorOps::movement_choose_strafe_axis(const real_vector3d *direction, uint8_t use_3d, const real_vector3d *facing, const real_vector3d *reference, real_vector3d *out_axis, int16_t *out_index)
{
    using namespace actor_movement_choose_strafe_axis_local;
    real_vector3d candidates[4];
    float best_reference_dot;
    float best_facing_dot;
    float reference_dot;
    float facing_dot;
    int16_t best_index;
    int16_t chosen;
    int16_t i;

    candidates[0] = *direction;
    if (use_3d == 0) {
        candidates[0].k = 0.0f;
        if (halo::math::vector3d_normalize_with_length(candidates[0]) == 0.0f) {
            candidates[0] = *facing;
        }
        candidates[2].i = -candidates[0].j;
        candidates[2].j = candidates[0].i;
        candidates[2].k = 0.0f;
    } else {
        if (halo::math::vector3d_normalize_with_length(candidates[0]) == 0.0f) {
            candidates[0] = *facing;
        }
        candidates[2] = *global_origin3d_pointer;
    }

    candidates[1].i = -candidates[0].i;
    candidates[1].j = -candidates[0].j;
    candidates[1].k = -candidates[0].k;
    candidates[3].i = -candidates[2].i;
    candidates[3].j = -candidates[2].j;
    candidates[3].k = -candidates[2].k;

    best_index = -1;
    best_reference_dot = 0.0f;
    best_facing_dot = 0.0f;

    for (i = 0; i < 4; i++) {
        if (use_3d == 0) {
            reference_dot = reference->j * candidates[i].j + candidates[i].i * reference->i;
            facing_dot = candidates[i].i * facing->i;
        } else {
            reference_dot = reference->j * candidates[i].j + candidates[i].i * reference->i +
                            reference->k * candidates[i].k;
            facing_dot = candidates[i].i * facing->i + facing->k * candidates[i].k;
        }
        facing_dot = candidates[i].j * facing->j + facing_dot;

        chosen = i;
        if (best_index != -1) {
            if (reference_dot <= best_reference_dot) {
                if (facing_dot <= best_facing_dot || reference_dot <= 0.5f) {
                    chosen = best_index;
                    facing_dot = best_facing_dot;
                    reference_dot = best_reference_dot;
                }
            } else if (facing_dot <= best_facing_dot && best_facing_dot >= 0.5f) {
                chosen = best_index;
                facing_dot = best_facing_dot;
                reference_dot = best_reference_dot;
            }
        }
        best_reference_dot = reference_dot;
        best_facing_dot = facing_dot;
        best_index = chosen;
    }

    *out_index = best_index;
    *out_axis = candidates[best_index];
}

namespace actor_movement_collect_obstacle_candidates_local {
extern "C" {
extern data_array *object_data;
extern double sqrt(double x);
extern int16_t object_find_in_sphere(int32_t kind, int32_t type_mask, const void *from,
                                     const real_point3d *center, float radius,
                                     datum_index *out_objects, int32_t maximum_count);
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out);
}
}

/**
 * Actor AI behaviour: movement collect obstacle candidates.
 *
 * @address 0x418ce0
 */
void ActorOps::movement_collect_obstacle_candidates(actor_movement_context *context)
{
    using namespace actor_movement_collect_obstacle_candidates_local;
    datum_index candidates[2048];
    real_matrix4x3 world_matrix;
    real_point3d transformed;
    object *unit_object;
    object *candidate_object;
    Object *candidate_definition;
    ModelCollisionGeometry *collision_model;
    ModelCollisionGeometrySphere *spheres;
    ModelCollisionGeometrySphere *sphere;
    const real_matrix4x3 *node_matrix;
    float origin_x, origin_y, origin_z, origin_top;
    float extent;
    float reach;
    float dx, dy;
    int16_t found;
    int16_t i;
    int16_t obstacle_index;
    int32_t sphere_count;
    int32_t remaining;
    datum_index *cursor;

    unit_object = ((object_header *)object_data->data)[context->unit_index & 0xffff].data;
    found = object_find_in_sphere(1, 0xc2, (uint8_t *)unit_object + 0x98, &context->position,
                                  context->search_radius, candidates, 0x800);
    context->obstacle_count = 0;
    if (found <= 0) {
        return;
    }

    cursor = candidates;
    remaining = (int32_t)(uint16_t)found;
    do {
        if (*cursor != (datum_index)k_datum_index_none && *cursor != context->unit_index) {
            candidate_object = ((object_header *)object_data->data)[*cursor & 0xffff].data;
            candidate_definition = (Object *)halo::cache::globals().tag_instances[candidate_object->definition_tag & 0xffff].data;
            collision_model = (ModelCollisionGeometry *)
                halo::cache::globals().tag_instances[candidate_definition->collision_model.tag_id.index].data;
            if ((int32_t)collision_model->pathfinding_spheres.count > 0) {
                candidate_object = ((object_header *)object_data->data)[*cursor & 0xffff].data;
                origin_x = ((struct object *)candidate_object)->bounding_center.x;
                origin_y = ((struct object *)candidate_object)->bounding_center.y;
                origin_z = ((struct object *)candidate_object)->bounding_center.z;
                origin_top = ((struct object *)candidate_object)->bounding_radius;
                extent = 0.0f;
                object_get_world_matrix(*cursor, &world_matrix);

                sphere_count = (int32_t)collision_model->pathfinding_spheres.count;
                spheres = (ModelCollisionGeometrySphere *)collision_model->pathfinding_spheres.pointer;
                for (i = 0; (int32_t)i < sphere_count; i++) {
                    sphere = &spheres[i];
                    if ((int16_t)sphere->node == -1) {
                        halo::math::matrix4x3_transform_point(transformed, *((const real_point3d *)&sphere->center),
                                                  world_matrix);
                        reach = world_matrix.scale * sphere->radius;
                    } else {
                        candidate_object = ((object_header *)object_data->data)[*cursor & 0xffff].data;
                        node_matrix = (const real_matrix4x3 *)
                            ((uint8_t *)candidate_object +
                             (int32_t)((struct object *)candidate_object)->nodes.offset +
                             (int16_t)sphere->node * 0x34);
                        halo::math::matrix4x3_transform_point(transformed, *((const real_point3d *)&sphere->center),
                                                  *node_matrix);
                        reach = sphere->radius * node_matrix->scale;
                    }
                    dx = transformed.x - origin_x;
                    dy = transformed.y - origin_y;
                    reach = (float)sqrt((double)(dy * dy + dx * dx)) + reach;
                    if (extent <= reach) {
                        extent = reach;
                    }
                }

                obstacle_index = context->obstacle_count;
                if (obstacle_index < 0x400) {
                    actor_movement_obstacle *obstacle = &context->obstacles[obstacle_index];

                    context->obstacle_count = (int16_t)(obstacle_index + 1);
                    obstacle->object_index = *cursor;
                    obstacle->radius = extent;
                    obstacle->position.x = origin_x;
                    obstacle->position.y = origin_y;
                    obstacle->bottom = origin_z - (origin_top - extent);
                    obstacle->height = (origin_top + origin_top) - (extent + extent);
                    if (obstacle->height < 0.0f) {
                        obstacle->height = 0.0f;
                    }
                }
            }
        }
        cursor++;
        remaining--;
    } while (remaining != 0);
}

namespace actor_movement_flying_needs_steering_local {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
}
}

/**
 * Actor AI behaviour: movement flying needs steering.
 *
 * @address 0x41aab0
 */
uint8_t ActorView::movement_flying_needs_steering(const real_point3d *destination, float *out_avoidance_distance)
{
    using namespace actor_movement_flying_needs_steering_local;
    actor *self;
    object *unit_object;
    Vehicle *vehicle_definition;
    real_vector3d delta;
    float avoidance_distance;
    float length;
    float facing_dot;
    uint8_t needs_steering;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    avoidance_distance = 0.0f;
    needs_steering = 1;

    if (self->vehicle_driving_type == 4) {
        unit_object = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        vehicle_definition = (Vehicle *)halo::cache::globals().tag_instances[unit_object->definition_tag & 0xffff].data;
        avoidance_distance = vehicle_definition->ai_avoidance_distance;
        if (avoidance_distance > 0.0f && self->avoidance_emergency > 0.9f) {
            delta.i = destination->x - self->body_position.x;
            delta.j = destination->y - self->body_position.y;
            delta.k = destination->z - self->body_position.z;
            length = halo::math::vector3d_normalize_with_length(delta);
            if (length > 0.0f) {
                facing_dot = delta.i * self->facing.i + delta.j * self->facing.j +
                             delta.k * self->facing.k;
                if (facing_dot > 0.984f) {
                    needs_steering = 0;
                }
            }
        }
    }

    if (out_avoidance_distance != (float *)0) {
        *out_avoidance_distance = avoidance_distance;
    }
    return needs_steering;
}

namespace actor_movement_get_stopping_distances_local {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
}
}

/**
 * Actor AI behaviour: movement get stopping distances.
 *
 * @address 0x4173a0
 */
void ActorView::movement_get_stopping_distances(float *out_accelerate_stop_distance, float *out_stop_distance)
{
    using namespace actor_movement_get_stopping_distances_local;
    actor *self;
    object *unit_object;
    Biped *biped_definition;
    Vehicle *vehicle_definition;
    float speed;
    float top_speed;
    float acceleration;
    float deceleration;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    speed = 0.0f;
    top_speed = 0.083333336f;
    acceleration = 0.016666668f;
    deceleration = 0.026666667f;

    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if (self->unit_index != (datum_index)k_datum_index_none) {
            unit_object = (object *)object_try_and_get(self->unit_index, 1);
            if (unit_object != (object *)0) {
                biped_definition = (Biped *)halo::cache::globals().tag_instances[unit_object->definition_tag & 0xffff].data;
                speed = unit_object->velocity.i * unit_object->forward.i +
                        unit_object->velocity.j * unit_object->forward.j +
                        unit_object->velocity.k * unit_object->forward.k;
                if ((biped_definition->biped_flags & 4) != 0) {
                    top_speed = biped_definition->max_velocity * 0.033333335f;
                    acceleration = biped_definition->acceleration * 0.033333335f;
                    deceleration = biped_definition->deceleration * 0.033333335f;
                    if (self->crouching != 0 && biped_definition->crouch_velocity_modifier > 0.0f) {
                        top_speed = top_speed * biped_definition->crouch_velocity_modifier;
                        acceleration = acceleration * biped_definition->crouch_velocity_modifier;
                        deceleration = deceleration * biped_definition->crouch_velocity_modifier;
                    }
                }
            }
        }
    } else if (self->vehicle_driving_type > 1 && self->vehicle_driving_type < 4) {
        unit_object = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        vehicle_definition = (Vehicle *)halo::cache::globals().tag_instances[unit_object->definition_tag & 0xffff].data;
        speed = unit_object->velocity.i * unit_object->forward.i +
                unit_object->velocity.j * unit_object->forward.j +
                unit_object->velocity.k * unit_object->forward.k;
        top_speed = vehicle_definition->maximum_forward_speed;
        deceleration = vehicle_definition->speed_acceleration;
        acceleration = deceleration;
    }

    if (out_stop_distance != (float *)0) {
        *out_stop_distance = (speed * speed) / (deceleration + deceleration);
    }
    if (out_accelerate_stop_distance != (float *)0) {
        if (top_speed < speed) {
            top_speed = speed;
        }
        *out_accelerate_stop_distance = (top_speed * top_speed) / (deceleration + deceleration) +
                                        (top_speed * top_speed - speed * speed) /
                                        (acceleration + acceleration);
    }
}

namespace actor_movement_project_into_frame_local {
}

/**
 * Actor AI behaviour: movement project into frame.
 *
 * @address 0x418c20
 */
void ActorOps::movement_project_into_frame(uint8_t use_3d, const real_vector3d *frame_axis, const real_vector3d *v, real_vector3d *out)
{
    using namespace actor_movement_project_into_frame_local;
    real_vector3d axis2;
    real_vector3d axis3;

    if (use_3d != 0) {
        halo::math::real_matrix4x3_rotation_from_forward(frame_axis, &axis2, &axis3);
        out->i = frame_axis->i * v->i + frame_axis->j * v->j + frame_axis->k * v->k;
        out->j = axis2.i * v->i + axis2.j * v->j + axis2.k * v->k;
        out->k = axis3.i * v->i + axis3.j * v->j + axis3.k * v->k;
        halo::math::vector3d_normalize_with_length(*out);
        return;
    }

    out->i = frame_axis->i * v->i + frame_axis->j * v->j;
    out->k = 0.0f;
    out->j = frame_axis->i * v->j - frame_axis->j * v->i;
    halo::math::vector3d_normalize_with_length(*out);
}

namespace actor_movement_set_destination_firing_position_local {
extern "C" {
extern data_array *actor_data;
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context);
}
}

/**
 * Actor AI behaviour: movement set destination firing position.
 *
 * @address 0x417830
 */
uint8_t ActorView::movement_set_destination_firing_position(int16_t formation_slot, path_find_context *path_context)
{
    using namespace actor_movement_set_destination_firing_position_local;
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_set_units_active(actor_index, 0);

    if (self->active_movement.type != 3 || *(int16_t *)&self->active_movement.destination != formation_slot) {
        self->queued_movement.type = 3;
        self->queued_movement.cancelled = 0;
        *(int16_t *)&self->queued_movement.destination = formation_slot;
        self->queued_movement.extra = (uint32_t)-1;
        self->active_movement = self->queued_movement;
        self->grenade_evasion_active = 0;
        return actor_movement_action_resolve(actor_index, 1, path_context);
    }
    if (self->needs_new_path != 0 && self->path_resolved_this_tick == 0) {
        return actor_movement_action_resolve(actor_index, 0, path_context);
    }
    return 1;
}

namespace actor_movement_set_destination_move_position_local {
extern "C" {
extern data_array *actor_data;
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context);
}
}

/**
 * Actor AI behaviour: movement set destination move position.
 *
 * @address 0x417750
 */
uint8_t ActorView::movement_set_destination_move_position(int16_t move_position_index)
{
    using namespace actor_movement_set_destination_move_position_local;
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->firing_position_index = -1;
    actor_set_units_active(actor_index, 0);

    if (self->active_movement.type != 4 || *(int16_t *)&self->active_movement.destination != move_position_index) {
        self->queued_movement.type = 4;
        self->queued_movement.cancelled = 0;
        *(int16_t *)&self->queued_movement.destination = move_position_index;
        self->queued_movement.extra = (uint32_t)-1;
        self->active_movement = self->queued_movement;
        return actor_movement_action_resolve(actor_index, 1, 0);
    }
    if (self->needs_new_path != 0 && self->path_resolved_this_tick == 0) {
        return actor_movement_action_resolve(actor_index, 0, 0);
    }
    return 1;
}

namespace actor_movement_set_destination_near_target_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context);
}
}

/**
 * Actor AI behaviour: movement set destination near target.
 *
 * @address 0x417910
 */
uint8_t TargetView::movement_set_destination_near_target(datum_index actor_index, float radius)
{
    using namespace actor_movement_set_destination_near_target_local;
    actor *self;
    prop *target;
    int32_t extra;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->firing_position_index = -1;
    actor_set_units_active(actor_index, 0);

    if (self->active_movement.type == 5 && *(uint32_t *)&self->active_movement.destination.x == (uint32_t)target_prop_index) {
        if (self->active_movement.destination.y == radius) {
            if (self->needs_new_path != 0 && self->path_resolved_this_tick == 0) {
                return actor_movement_action_resolve(actor_index, 0, 0);
            }
            return 1;
        }
    }

    *(uint32_t *)&self->queued_movement.destination.x = (uint32_t)target_prop_index;
    target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    self->queued_movement.type = 5;
    self->queued_movement.cancelled = 0;
    self->queued_movement.destination.y = radius;
    extra = target->relationship_object_index;
    if (extra == -1) {
        extra = (int32_t)target->object_index;
    }
    self->queued_movement.extra = (uint32_t)extra;
    self->active_movement = self->queued_movement;
    return actor_movement_action_resolve(actor_index, 1, 0);
}

namespace actor_movement_set_destination_point_local {
extern "C" {
extern data_array *actor_data;
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context);
}
}

/**
 * Actor AI behaviour: movement set destination point.
 *
 * @address 0x417610
 */
uint8_t ActorOps::movement_set_destination_point(real_point3d *destination, datum_index actor_index, int32_t parameter, uint32_t extra)
{
    using namespace actor_movement_set_destination_point_local;
    actor *self;
    float dx, dy, dz;
    float dist2;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->firing_position_index = -1;
    actor_set_units_active(actor_index, 0);

    if (self->active_movement.type == 2 && self->active_movement.parameter == parameter) {
        dx = self->active_movement.destination.x - destination->x;
        dy = self->active_movement.destination.y - destination->y;
        dz = self->active_movement.destination.z - destination->z;
        dist2 = dz * dz + dy * dy + dx * dx;
        if (dist2 <= 0.010000001f) {
            if (self->needs_new_path != 0 && self->path_resolved_this_tick == 0) {
                return actor_movement_action_resolve(actor_index, 0, 0);
            }
            return 1;
        }
    }

    self->queued_movement.type = 2;
    self->queued_movement.cancelled = 0;
    self->queued_movement.destination = *destination;
    self->queued_movement.parameter = parameter;
    self->queued_movement.extra = extra;
    self->active_movement = self->queued_movement;
    return actor_movement_action_resolve(actor_index, 1, 0);
}

namespace actor_movement_test_obstacle_ray_local {
extern "C" {
extern const real_vector3d *global_origin3d_pointer;
extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                                ModelCollisionGeometryBSP *bsp,
                                                int16_t breakable_surface_count,
                                                uint32_t *breakable_surfaces, real_point3d *origin,
                                                real_vector3d *delta, float max_fraction);
}
}

/**
 * stack -> out_distance, out_clear_counter
 *
 * @address 0x418f70
 */
int16_t ActorOps::movement_test_obstacle_ray(real_vector3d *out_elevation, const float *sample, real_point3d *out_end_point, actor_movement_context *context, float *out_distance, uint8_t *out_clear_counter)
{
    using namespace actor_movement_test_obstacle_ray_local;
    real_vector3d elevation;
    real_vector3d segment;
    collision_bsp_segment_result bsp_result;
    float hit_fraction;
    float scale;
    int16_t result;
    int16_t i;

    *out_distance = 3.4028235e+38f;
    result = 0;

    elevation.i = sample[6] * context->up.i + sample[5] * context->left.i +
                  sample[4] * context->forward.i + global_origin3d_pointer->i;
    elevation.j = sample[6] * context->up.j + sample[5] * context->left.j +
                  sample[4] * context->forward.j + global_origin3d_pointer->j;
    elevation.k = sample[6] * context->up.k + sample[5] * context->left.k +
                  sample[4] * context->forward.k + global_origin3d_pointer->k;

    out_end_point->x = (sample[3] * context->up.i + sample[2] * context->left.i +
                        sample[1] * context->forward.i + global_origin3d_pointer->i) *
                       context->ray_scale + context->position.x;
    out_end_point->y = (sample[3] * context->up.j + sample[2] * context->left.j +
                        sample[1] * context->forward.j + global_origin3d_pointer->j) *
                       context->ray_scale + context->position.y;
    out_end_point->z = (sample[3] * context->up.k + sample[2] * context->left.k +
                        sample[1] * context->forward.k + global_origin3d_pointer->k) *
                       context->ray_scale + context->position.z;

    scale = context->search_radius * sample[0];
    out_elevation->i = elevation.i * scale;
    out_elevation->j = elevation.j * scale;
    out_elevation->k = elevation.k * scale;

    segment.i = out_end_point->x - context->position.x;
    segment.j = out_end_point->y - context->position.y;
    segment.k = out_end_point->z - context->position.z;

    if (halo::physics::collision_bsp_query_segment_init(3, &bsp_result,
                                         (ModelCollisionGeometryBSP *)context->collision_bsp,
                                         0, 0, &context->position, &segment, 1.0f) != 0) {
        result = 2;
        *out_distance = 0.0f;
    } else if (halo::physics::collision_bsp_query_segment_init(3, &bsp_result,
                                                (ModelCollisionGeometryBSP *)context->collision_bsp,
                                                0, 0, out_end_point, out_elevation, 1.0f) != 0) {
        result = 2;
        *out_distance = bsp_result.t;
    }

    for (i = 0; i < context->obstacle_count; i++) {
        actor_movement_obstacle *obstacle = &context->obstacles[i];

        if (halo::math::ray_intersects_cylinder(obstacle->height, obstacle->radius, &segment, &hit_fraction,
                                    (real_point3d *)&obstacle->position, out_end_point, out_elevation) != 0 &&
            hit_fraction < *out_distance) {
            result = 1;
            *out_distance = hit_fraction;
        }
    }

    if (out_clear_counter != (uint8_t *)0) {
        if (result > 0) {
            *out_clear_counter = 0;
            return result;
        }
        if (*out_clear_counter != 0xff) {
            *out_clear_counter = (uint8_t)(*out_clear_counter + 1);
        }
    }
    return result;
}

}
