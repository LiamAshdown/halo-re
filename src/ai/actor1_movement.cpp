#include "halo/core/flag_bits.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/ai/actor_movement.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
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
#include "halo/physics/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"

namespace c_actor_avoid_obstacle_and_project {
static auto &global_structure_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_structure_collision_bsp);
static auto &global_down3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_down3d_pointer);



}


/**
 * actor_avoid_obstacle_and_project: behaviour unchanged from the original routine.
 *
 * @address 0x4095c0
 */
uint8_t halo::ai::movement_ops::avoid_obstacle_and_project(datum_index vehicle_index, real_point3d *entry, real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index)
{
    using namespace c_actor_avoid_obstacle_and_project;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    object *vehicle = (object *)halo::ai::object_bytes(vehicle_index);
    Vehicle *vehicle_tag = halo::ai::tag_data<Vehicle>(vehicle->definition_tag);
    real_point3d point = *entry;
    uint8_t near_line = in_out_near_line != 0 ? *in_out_near_line : 0;
    collision_bsp_segment_result result;
    real_point3d start;
    real_vector3d delta;

    if ((static_cast<uint8_t>(vehicle_tag->base.unit_flags) & 0x10) == 0) {
        [&]() {
        real_point3d center = *(real_point3d *)&((vehicle_object *)vehicle)->base.bounding_center.x;
        float radius = ((vehicle_object *)vehicle)->base.bounding_radius;
        float ax = act->body_position.x;
        float ay = act->body_position.y;
        real_point3d *target_pointer;
        real_point3d target;
        real_vector2d to_center;
        real_vector2d to_target;
        real_vector2d from_target;
        real_vector2d away;

        if (vehicle_tag->base.ai_vehicle_radius > 0.0f) {
            radius = vehicle_tag->base.ai_vehicle_radius;
        }
        target_pointer = entry;
        if (!near_line) {
            float dx = hint->x - center.x;
            float dy = hint->y - center.y;
            float dz = hint->z - center.z;
            float d = (float)halo::libm::sqrt(dz * dz + dy * dy + dx * dx);

            if (d <= 0.5f) {
                near_line = 1;
            } else {
                d = d + 0.3f;
                if (!(radius > d)) {
                    radius = d;
                }
                target_pointer = hint;
            }
        }
        target = *target_pointer;
        to_center.i = center.x - ax;
        to_center.j = center.y - ay;
        to_target.i = target.x - ax;
        to_target.j = target.y - ay;
        from_target.i = center.x - target.x;
        from_target.j = center.y - target.y;
        if (!near_line) {

            float lx = hint->x - entry->x;
            float ly = hint->y - entry->y;
            float t = -(to_target.j * ly + lx * to_target.i);
            float px = to_target.i + lx * t;
            float py = to_target.j + ly * t;

            if (!(py * py + px * px > 0.1225f)) {
                near_line = 1;
            }
        }
        {
            float length_squared = to_target.j * to_target.j + to_target.i * to_target.i;
            float ratio;

            if (!(length_squared > 0.0f)) {
                return;
            }
            ratio = (to_target.j * to_center.j + to_target.i * to_center.i) / length_squared;
            if (ratio > 0.0f && ratio <= 1.2f) {
                away.i = -to_target.j;
                away.j = to_target.i;
                if (to_center.j * to_target.i + away.i * to_center.i > 0.0f) {
                    away.i = to_target.j;
                    away.j = -to_target.i;
                }
            } else {
                if (near_line) {
                    return;
                }
                away.i = -from_target.i;
                away.j = -from_target.j;
            }
        }
        if (!(halo::math::vector2d_normalize_with_length(away) > 0.0f)) {
            return;
        }
        point.x = away.i * (radius * 1.1f) + center.x;
        point.y = away.j * (radius * 1.1f) + center.y;
        {
            float dx = point.x - ax;
            float dy = point.y - ay;
            float dz = point.z - act->body_position.z;
            float distance_squared = dz * dz + dy * dy + dx * dx;
            float distance;
            real_vector3d side;

            if (!(distance_squared > 0.0001f) || !(distance_squared <= 4.0f)) {
                return;
            }
            distance = (float)halo::libm::sqrt(distance_squared);
            side.i = -from_target.j;
            side.j = from_target.i;
            if (!(dy * from_target.i + side.i * dx > 0.0f)) {
                side.i = from_target.j;
                side.j = -from_target.i;
            }
            side.k = 0.0f;
            if (!(halo::math::vector2d_normalize_with_length(*((real_vector2d *)&side)) > 0.0f)) {
                return;
            }
            distance = 2.0f - distance;
            point.x = side.i * distance + point.x;
            point.y = side.j * distance + point.y;
            point.z = side.k * distance + point.z;
        }
        }();
    }
    if (in_out_near_line != 0) {
        *in_out_near_line = near_line;
    }
    start.x = point.x + halo::math::globals().global_up3d_pointer->i;
    start.y = point.y + halo::math::globals().global_up3d_pointer->j;
    start.z = point.z + halo::math::globals().global_up3d_pointer->k;
    delta.i = global_down3d_pointer->i * 4.0f;
    delta.j = global_down3d_pointer->j * 4.0f;
    delta.k = global_down3d_pointer->k * 4.0f;
    if (!halo::physics::collision_bsp_query_segment_init(1, &result, global_structure_collision_bsp, 0, 0, &start, &delta,
                                          3.4028235e38f)) {
        return 0;
    }
    *out_surface_index = result.surface_index;
    out_point->x = delta.i * result.t + start.x;
    out_point->y = delta.j * result.t + start.y;
    out_point->z = delta.k * result.t + start.z;
    return 1;
}

namespace halo::ai {
uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry, real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index)
{
    return halo::ai::movement_ops(actor_index).avoid_obstacle_and_project(vehicle_index, entry, hint, in_out_near_line, out_point, out_surface_index);
}
}


namespace c_actor_avoidance_build_direction_tables {
static auto &actor_avoidance_samples_a = halo::link::ref<float [16][7]>(halo::ai::vars().actor_avoidance_samples_a);
static auto &actor_avoidance_circle = halo::link::ref<float [8][3]>(halo::ai::vars().actor_avoidance_circle);
static auto &actor_avoidance_samples_b = halo::link::ref<float [9][7]>(halo::ai::vars().actor_avoidance_samples_b);
static auto &actor_avoidance_b_radius = halo::link::ref<const float [9]>(halo::ai::vars().actor_avoidance_b_radius);
static auto &actor_avoidance_b_elevation = halo::link::ref<const float [9]>(halo::ai::vars().actor_avoidance_b_elevation);
static auto &actor_avoidance_b_bearing = halo::link::ref<const float [9]>(halo::ai::vars().actor_avoidance_b_bearing);
static auto &actor_avoidance_a_bearing = halo::link::ref<const float [8]>(halo::ai::vars().actor_avoidance_a_bearing);
static auto &actor_avoidance_a_radius = halo::link::ref<const float [2]>(halo::ai::vars().actor_avoidance_a_radius);
static auto &actor_avoidance_a_elevation = halo::link::ref<const float [2]>(halo::ai::vars().actor_avoidance_a_elevation);
}


/**
 * actor_avoidance_build_direction_tables: behaviour unchanged from the original routine.
 *
 * @address 0x41a2d0
 */
void halo::ai::movement_ops::avoidance_build_direction_tables()
{
    using namespace c_actor_avoidance_build_direction_tables;
    int32_t i;
    int32_t j;
    float bearing;
    float elevation;
    float radius;
    float sin_bearing;
    float cos_bearing;
    float sin_elevation;
    float cos_elevation;

    for (i = 0; i < 9; i++) {
        bearing = actor_avoidance_b_bearing[i];
        sin_bearing = (float)halo::libm::sin((double)bearing);
        cos_bearing = (float)halo::libm::cos((double)bearing);
        elevation = actor_avoidance_b_elevation[i] * 0.05235988f;
        sin_elevation = (float)halo::libm::sin((double)elevation);
        cos_elevation = (float)halo::libm::cos((double)elevation);
        radius = actor_avoidance_b_radius[i];

        actor_avoidance_samples_b[i][0] = 1.0f;
        actor_avoidance_samples_b[i][1] = 0.0f;
        actor_avoidance_samples_b[i][2] = cos_bearing * radius * 0.7f;
        actor_avoidance_samples_b[i][3] = sin_bearing * radius * 0.7f;
        actor_avoidance_samples_b[i][4] = cos_elevation;
        actor_avoidance_samples_b[i][5] = sin_elevation * cos_bearing;
        actor_avoidance_samples_b[i][6] = sin_elevation * sin_bearing;
    }

    for (i = 0; i < 2; i++) {
        sin_elevation = (float)halo::libm::sin((double)actor_avoidance_a_elevation[i]);
        cos_elevation = (float)halo::libm::cos((double)actor_avoidance_a_elevation[i]);
        radius = actor_avoidance_a_radius[i];

        for (j = 0; j < 8; j++) {
            float *row = actor_avoidance_samples_a[j * 2 + i];

            bearing = actor_avoidance_a_bearing[j];
            actor_avoidance_circle[j][0] = 0.0f;
            actor_avoidance_circle[j][1] = (float)halo::libm::cos((double)bearing);
            actor_avoidance_circle[j][2] = (float)halo::libm::sin((double)bearing);

            row[0] = 0.7f;
            row[1] = radius * actor_avoidance_circle[j][0];
            row[2] = radius * actor_avoidance_circle[j][1];
            row[3] = radius * actor_avoidance_circle[j][2];
            row[4] = sin_elevation * actor_avoidance_circle[j][0];
            row[5] = sin_elevation * actor_avoidance_circle[j][1];
            row[6] = sin_elevation * actor_avoidance_circle[j][2];
            row[4] = cos_elevation;
        }
    }
}

namespace halo::ai {
void actor_avoidance_build_direction_tables(void)
{
    halo::ai::movement_ops::avoidance_build_direction_tables();
}
}

namespace c_actor_avoidance_interpolate_sample {
}


/**
 * actor_avoidance_interpolate_sample: behaviour unchanged from the original routine.
 *
 * @address 0x419240
 */
uint8_t halo::ai::movement_ops::avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples, int16_t count, const float *values, float *out_index, float *out_value)
{
    using namespace c_actor_avoidance_interpolate_sample;
    const real_vector3d *sample;
    float previous_cross;
    float cross;
    float dot;
    int16_t previous_index;
    int16_t wrapped_index;
    int16_t i;

    previous_index = (int16_t)(count - 1);
    sample = &samples[previous_index];
    previous_cross = direction->k * sample->j - sample->k * direction->j;

    if (count < 1) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        sample = &samples[i];
        cross = sample->j * direction->k - sample->k * direction->j;
        if (cross * previous_cross <= 0.0f) {
            dot = direction->i * sample->i + sample->j * direction->j + sample->k * direction->k;
            if (dot > 0.0f) {
                wrapped_index = i;
                if (i == 0) {
                    wrapped_index = count;
                }
                *out_index = ((float)previous_index * cross - (float)wrapped_index * previous_cross) /
                             (cross - previous_cross);
                *out_value = (cross * values[previous_index] - previous_cross * values[i]) /
                             (cross - previous_cross);
                return 1;
            }
        }
        previous_index = i;
        previous_cross = cross;
    }

    return 0;
}

namespace halo::ai {
uint8_t actor_avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples, int16_t count, const float *values, float *out_index, float *out_value)
{
    return halo::ai::movement_ops::avoidance_interpolate_sample(direction, samples, count, values, out_index, out_value);
}
}

namespace c_actor_check_step_obstruction {
static auto &global_down3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &global_structure_collision_bsp = halo::link::ref<int32_t>(halo::physics::vars().global_structure_collision_bsp);
}


/**
 * actor_check_step_obstruction: behaviour unchanged from the original routine.
 *
 * @address 0x417bb0
 */
uint8_t halo::ai::movement_ops::check_step_obstruction(real_vector2d *direction, float step_distance, float step_up, uint8_t *out_flag, void *extra_param)
{
    using namespace c_actor_check_step_obstruction;
    datum_index actor_index = datum;
    actor *self;
    Actor *definition;
    uint8_t obstructed = 0;
    uint8_t used_point_check = 0;
    real_point3d step_point;

    self = halo::ai::actor_at(actor_index);
    definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if (self->flying == 0) {
        uint8_t trace_ok;
        bool trace_done = false;

        step_point.x = step_distance * direction->i + self->body_position.x;
        step_point.y = step_distance * direction->j + self->body_position.y;
        halo::ai::actor_update_target_lead_position(actor_index);

        trace_ok = halo::ai::path_find_test_segment_unobstructed(halo::scenario::globals().structure_bsp, &self->pathfinding_point,
            self->ignores_glass, (int32_t)self->pathfinding_surface_index, &step_point, -1, definition->pathfinding_radius, 0,
            (path_find_boundary_crossing *)extra_param);
        if (!trace_ok) {
            obstructed = 1;
            {

                float dz = ((path_find_boundary_crossing *)extra_param)->position.z - self->body_position.z;
                if (dz <= step_distance * 0.5f && (step_up != 0.0f || step_distance * -0.5f <= dz)) {
                    trace_done = true;
                }
            }
        }
        if (!trace_done) {
        obstructed = 0;
        if (0.0f < step_up) {
            real_point3d mid;
            real_vector3d scaled_dir;
            uint8_t clear1;
            collision_bsp_segment_result probe;

            mid.x = (self->aim_origin.x + self->body_position.x) * 0.5f;
            mid.y = (self->aim_origin.y + self->body_position.y) * 0.5f;
            mid.z = (self->aim_origin.z + self->body_position.z) * 0.5f;
            scaled_dir.i = step_distance * direction->i;
            scaled_dir.j = step_distance * direction->j;
            scaled_dir.k = 0.0f;

            clear1 = halo::physics::collision_bsp_query_segment_init(3, &probe, (ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0, 0, &mid, &scaled_dir, 3.4028235e+38f);
            if (!clear1) {
                obstructed = 1;
                used_point_check = 1;
                if (step_up < 3.4028235e+38f) {
                    real_point3d far_point;
                    real_vector3d down_step;
                    uint8_t clear2;

                    far_point.x = scaled_dir.i + mid.x;
                    far_point.y = scaled_dir.j + mid.y;
                    far_point.z = scaled_dir.k + mid.z;
                    down_step.i = step_up * global_down3d_pointer->i;
                    down_step.j = step_up * global_down3d_pointer->j;
                    down_step.k = step_up * global_down3d_pointer->k;

                    clear2 = halo::physics::collision_bsp_query_segment_init(3, &probe, (ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0, 0, &far_point, &down_step, 3.4028235e+38f);

                    if (!clear2) {
                        obstructed = 0;
                    }
                }
            }
        }
        }
    }
    if (out_flag != 0) {
        *out_flag = used_point_check;
    }
    return obstructed;
}

namespace halo::ai {
uint8_t actor_check_step_obstruction(datum_index actor_index, real_vector2d *direction, float step_distance, float step_up, uint8_t *out_flag, void *extra_param)
{
    return halo::ai::movement_ops(actor_index).check_step_obstruction(direction, step_distance, step_up, out_flag, extra_param);
}
}

namespace c_actor_check_vehicle_mode_timeout {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_check_vehicle_mode_timeout: behaviour unchanged from the original routine.
 *
 * @address 0x428270
 */
uint8_t halo::ai::movement_ops::check_vehicle_mode_timeout()
{
    using namespace c_actor_check_vehicle_mode_timeout;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->mode == halo::ai::actor_mode::charge &&
        self->mode_data.charge.stage == 3 &&
        self->mode_data.charge.jump_started != 0) {
        int32_t deadline = self->mode_data.charge.stage_start_time + 0x1e;
        return (int32_t)halo::game::globals().game_time->game_time <= deadline;
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_check_vehicle_mode_timeout(datum_index actor_index)
{
    return halo::ai::movement_ops(actor_index).check_vehicle_mode_timeout();
}
}

namespace c_actor_compute_swarm_avoidance_offset {
static auto &global_forward2d_pointer = halo::link::ref<const real_vector2d *>(halo::ai::vars().global_forward2d_pointer);
}


/**
 * actor_compute_swarm_avoidance_offset: behaviour unchanged from the original routine.
 *
 * @address 0x425c70
 */
void halo::ai::movement_ops::compute_swarm_avoidance_offset(datum_index unit_index, float radius, float *out_offset)
{
    using namespace c_actor_compute_swarm_avoidance_offset;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->swarm_index == (datum_index)k_datum_index_none) {
        return;
    }

    {
        swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
        int16_t i;

        for (i = 0; i < s->component_count; i++) {
            if (s->unit_index[i] == unit_index) {
                object *unit_object = halo::ai::object_at(unit_index);
                swarm_component *component = &((swarm_component *)halo::ai::globals().swarm_component_data->data)[s->component_index[i] & halo::k_slot_mask];
                uint16_t flags = component->flags;
                datum_index target = component->leap_target_index;

                if ((flags & 1) == 0 || target == (datum_index)k_datum_index_none) {
                    if ((flags & 8) != 0 && (flags & 0x10) != 0) {
                        uint8_t byte21 = *((uint8_t *)component + 0x21);
                        if ((byte21 & 4) != 0 && (byte21 & 0x10) != 0) {
                            real_vector2d dir;
                            dir.i = unit_object->forward.i;
                            dir.j = unit_object->forward.j;
                            if (halo::math::vector2d_normalize_with_length(dir) == 0.0f) {
                                dir.i = unit_object->up.i;
                                dir.j = unit_object->up.j;
                                if (halo::math::vector2d_normalize_with_length(dir) == 0.0f) {
                                    dir.i = global_forward2d_pointer->i;
                                    dir.j = global_forward2d_pointer->j;
                                }
                            }
                            {
                                float scale = component->infection.heading.k;
                                float z = component->infection.turn_rate;
                                out_offset[0] = dir.i * scale;
                                out_offset[1] = dir.j * scale;
                                out_offset[2] = z;
                            }
                        }
                        component->flags &= 0xef;
                    }
                } else {

                    prop *target_prop = halo::ai::prop_at(target);
                    real max_time = 0.7f;
                    real half_gravity;
                    real horizontal_speed;
                    real_vector3d leap;

                    if (!(radius > 0.12f)) {
                        radius = 0.12f;
                    }
                    if (halo::ai::projectile_solve_ballistic_arc(&target_prop->center_of_mass,
                            &component->position, radius, 1.0f, &max_time, 0,
                            &leap, 0, 0, 0, 0, &half_gravity, &horizontal_speed)) {
                        float x, y, sum_sq;

                        if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                            leap = *(real_vector3d *)&self->facing.i;
                            if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                                leap = *halo::math::globals().global_forward3d_pointer;
                            }
                        }
                        if (target_prop->flying == 0 && !(half_gravity <= 0.075f)) {
                            half_gravity = 0.075f;
                        }
                        x = leap.i * horizontal_speed;
                        y = leap.j * horizontal_speed;
                        out_offset[2] = half_gravity;
                        out_offset[0] = x;
                        out_offset[1] = y;
                        sum_sq = y * y + x * x + half_gravity * half_gravity;
                        if (!(sum_sq <= radius * radius)) {
                            float k = radius / (float)halo::libm::sqrt((double)sum_sq);
                            out_offset[0] = x * k;
                            out_offset[1] = y * k;
                            out_offset[2] = half_gravity * k;
                        }
                    }
                }
            }

        }
    }
}

namespace halo::ai {
void actor_compute_swarm_avoidance_offset(datum_index actor_index, datum_index unit_index, float radius, float *out_offset)
{
    halo::ai::movement_ops(actor_index).compute_swarm_avoidance_offset(unit_index, radius, out_offset);
}
}

namespace c_actor_create_swarm {
}


/**
 * actor_create_swarm: behaviour unchanged from the original routine.
 *
 * @address 0x427f40
 */
datum_index halo::ai::movement_ops::create_swarm()
{
    using namespace c_actor_create_swarm;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->swarm_index == (datum_index)k_datum_index_none) {
        self->swarm_index = halo::memory::datum_new(halo::ai::globals().swarm_data);
        if (self->swarm_index != (datum_index)k_datum_index_none) {
            datum_index unit_index = self->cluster_unit_index;
            swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];

            s->actor_index = actor_index;
            s->component_count = 0;

            while (unit_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
                object *unit_object = header->data;
                datum_index component_index = halo::memory::datum_new(halo::ai::globals().swarm_component_data);

                if (component_index == (datum_index)k_datum_index_none) {
                    return self->swarm_index;
                }

                {
                    swarm *s2 = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
                    swarm_component *component = &((swarm_component *)halo::ai::globals().swarm_component_data->data)[component_index & halo::k_slot_mask];
                    datum_index marker;

                    component->leap_target_index = halo::k_dword_none;
                    s2->unit_index[s2->component_count] = unit_index;
                    s2->component_index[s2->component_count] = component_index;
                    s2->component_count = s2->component_count + 1;

                    marker = (unit_object->type == _object_type_biped) ? ((biped_object *)unit_object)->biped.ground_surface_index
                                                       : (datum_index)k_datum_index_none;
                    halo::objects::object_get_position(&component->position, unit_index);
                    component->marker_index = marker;
                }

                unit_index = halo::units::unit_data_of(unit_object)->swarm_next_unit_index;
            }
        }
    }
    return self->swarm_index;
}

namespace halo::ai {
datum_index actor_create_swarm(datum_index actor_index)
{
    return halo::ai::movement_ops(actor_index).create_swarm();
}
}

namespace c_actor_delete_swarm {
}


/**
 * actor_delete_swarm: behaviour unchanged from the original routine.
 *
 * @address 0x4280b0
 */
void halo::ai::movement_ops::delete_swarm()
{
    using namespace c_actor_delete_swarm;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    datum_index swarm_index = self->swarm_index;

    if (swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[swarm_index & halo::k_slot_mask];
        int16_t i;

        for (i = 0; i < s->component_count; i++) {
            halo::memory::datum_delete(halo::ai::globals().swarm_component_data, s->component_index[i]);
        }
        halo::memory::datum_delete(halo::ai::globals().swarm_data, swarm_index);
        self->swarm_index = (datum_index)k_datum_index_none;
    }
}

namespace halo::ai {
void actor_delete_swarm(datum_index actor_index)
{
    halo::ai::movement_ops(actor_index).delete_swarm();
}
}

namespace c_actor_evaluate_search_node {



static actor *actor_try_get(datum_index handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    actor *record;

    if (index < 0 || index >= halo::ai::globals().actor_data->maximum_count) {
        return 0;
    }
    record = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + halo::ai::globals().actor_data->size * index);
    if (record->identifier == 0 || (salt != 0 && record->identifier != salt)) {
        return 0;
    }
    return record;
}
}


/**
 * actor_evaluate_search_node: behaviour unchanged from the original routine.
 *
 * @address 0x4091d0
 */
uint8_t halo::ai::movement_ops::evaluate_search_node(datum_index vehicle_index, int16_t seat_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front)
{
    using namespace c_actor_evaluate_search_node;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    ActorVariant *variant = halo::ai::tag_data<ActorVariant>(act->actor_variant_tag);
    real_point3d entry;
    real_point3d seat;
    real_point3d hint;
    real_vector3d direction;
    real_vector2d to_seat;
    float ax;
    float ay;
    float distance;
    float dot;
    float score;
    uint8_t close;
    uint8_t facing;
    uint8_t in_front;
    datum_index prop_index;

    if (halo::units::unit_is_seat_occupied((int32_t)vehicle_index, seat_index)) {
        return 0;
    }
    if ((halo::ai::tag_data<Actor>(act->actor_definition_tag)->more_flags & 8) && !halo::units::unit_seat_flag_bit10(vehicle_index, seat_index)) {
        return 0;
    }
    if (!halo::units::unit_find_weapon_marker_transform(act->unit_index, vehicle_index, seat_index, &entry, &seat,
                                           &hint)) {
        return 0;
    }
    halo::objects::object_get_position((real_point3d *)&direction, vehicle_index);
    direction.i = seat.x - entry.x;
    direction.k = 0.0f;
    direction.j = seat.y - entry.y;
    if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&direction)) == 0.0f) {
        direction = *(real_vector3d *)&act->facing.i;
    }
    ax = act->body_position.x;
    ay = act->body_position.y;
    if (halo::libm::sqrt((seat.y - ay) * (seat.y - ay) + (seat.x - ax) * (seat.x - ax)) <
        halo::libm::sqrt((entry.y - ay) * (entry.y - ay) + (entry.x - ax) * (entry.x - ax))) {
        distance = (float)halo::libm::sqrt((seat.y - ay) * (seat.y - ay) + (seat.x - ax) * (seat.x - ax));
    } else {
        distance = (float)halo::libm::sqrt((entry.y - ay) * (entry.y - ay) + (entry.x - ax) * (entry.x - ax));
    }
    for (prop_index = act->first_prop; prop_index != k_datum_index_none;) {
        struct prop *prop = halo::ai::prop_at(prop_index);
        datum_index other_index = prop->owner_actor_index;

        prop_index = prop->next_in_actor;
        if (prop->enemy == 0 && other_index != k_datum_index_none) {
            actor *other = actor_try_get(other_index);

            if (other != 0 && other->mode == halo::ai::actor_mode::vehicle && other->mode_data.vehicle.vehicle_index == vehicle_index &&
                other->mode_data.vehicle.seat_index == seat_index) {
                float dx = other->mode_data.vehicle.path_destination.x - other->body_position.x;
                float dy = other->mode_data.vehicle.path_destination.y - other->body_position.y;

                if (distance * distance > dy * dy + dx * dx) {
                    return 0;
                }
            }
        }
    }
    to_seat.i = seat.x - ax;
    to_seat.j = seat.y - ay;
    halo::math::vector2d_normalize_with_length(to_seat);
    dot = to_seat.j * act->facing.j + to_seat.i * act->facing.i;
    close = (uint8_t)(distance < 0.7f);
    facing = (uint8_t)(dot > 0.6f);
    in_front = (uint8_t)(distance < 1.1f && dot > 0.0f);
    score = 10.0f / (distance + 1.0f);
    if ((halo::units::unit_seat_flag_bit3(vehicle_index, seat_index) != 0) != (test_flag(variant->flags, halo::tags::actor_variant_tag_flag::prefer_passenger_seat))) {
        score = score + 3.5f;
    }
    if (out_entry != 0) {
        *out_entry = entry;
    }
    if (out_direction != 0) {
        *out_direction = direction;
    }
    if (out_hint != 0) {
        *out_hint = hint;
    }
    if (out_score != 0) {
        *out_score = score;
    }
    if (out_close != 0) {
        *out_close = close;
    }
    if (out_facing != 0) {
        *out_facing = facing;
    }
    if (out_in_front != 0) {
        *out_in_front = in_front;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front)
{
    return halo::ai::movement_ops(actor_index).evaluate_search_node(vehicle_index, seat_index, out_entry, out_direction, out_hint, out_score, out_close, out_facing, out_in_front);
}
}


namespace c_actor_fill_unit_position_context {
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);


static uint8_t *object_get(datum_index object_index)
{
    return reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index));
}
}


/**
 * actor_fill_unit_position_context: behaviour unchanged from the original routine.
 *
 * @address 0x4296c0
 */
void halo::ai::movement_ops::fill_unit_position_context(datum_index unit_index, actor_firing_positions *out_context)
{
    using namespace c_actor_fill_unit_position_context;
    object *unit = (object *)object_get(unit_index);
    object_marker marker;
    datum_index root = k_datum_index_none;
    object *root_object;

    halo::objects::object_get_position(&out_context->body_position, unit_index);
    out_context->forward = *(real_vector3d *)&unit->forward.i;
    halo::objects::object_get_node_local_transform(unit_index, ai_marker_name_a, &marker, 1);
    out_context->aim_origin = marker.node_transform.position;
    halo::objects::object_get_root_object_velocities(unit_index, &out_context->velocity, 0);
    if (unit_index != k_datum_index_none) {
        datum_index cursor = unit_index;

        do {
            root = cursor;
            cursor = ((object *)object_get(cursor))->parent_object;
        } while (cursor != k_datum_index_none);
    }
    root_object = (object *)object_get(root);
    out_context->location = *halo::ai::object_location(root_object);
}

namespace halo::ai {
void actor_fill_unit_position_context(datum_index unit_index, actor_firing_positions *out_context)
{
    halo::ai::movement_ops::fill_unit_position_context(unit_index, out_context);
}
}

namespace c_actor_find_best_search_node {
}


/**
 * actor_find_best_search_node: behaviour unchanged from the original routine.
 *
 * @address 0x409070
 */
int16_t halo::ai::movement_ops::find_best_search_node(datum_index vehicle_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint)
{
    using namespace c_actor_find_best_search_node;
    datum_index actor_index = datum;
    Unit *vehicle_tag = halo::ai::tag_data<Unit>(((object_header *)halo::objects::globals().object_data->data)[vehicle_index & halo::k_slot_mask].data->definition_tag);
    int16_t best_seat = -1;
    float best_score = 0.0f;
    real_point3d best_entry = {0.0f, 0.0f, 0.0f};
    real_vector3d best_direction = {0.0f, 0.0f, 0.0f};
    real_point3d best_hint = {0.0f, 0.0f, 0.0f};
    int16_t i;

    for (i = 0; i < (int32_t)vehicle_tag->seats.count; i++) {
        real_point3d entry;
        real_vector3d direction;
        real_point3d hint;
        float score;

        if (halo::ai::actor_evaluate_search_node(actor_index, vehicle_index, i, &entry, &direction, &hint, &score, 0, 0, 0) &&
            score > best_score) {
            best_score = score;
            best_entry = entry;
            best_direction = direction;
            best_hint = hint;
            best_seat = i;
        }
    }
    if (out_entry != 0) {
        *out_entry = best_entry;
    }
    if (out_direction != 0) {
        *out_direction = best_direction;
    }
    if (out_hint != 0) {
        *out_hint = best_hint;
    }
    return best_seat;
}

namespace halo::ai {
int16_t actor_find_best_search_node(datum_index actor_index, datum_index vehicle_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint)
{
    return halo::ai::movement_ops(actor_index).find_best_search_node(vehicle_index, out_entry, out_direction, out_hint);
}
}

namespace c_actor_gate_jump_traversal {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_gate_jump_traversal: behaviour unchanged from the original routine.
 *
 * @address 0x40a700
 */
uint8_t halo::ai::movement_ops::gate_jump_traversal(int16_t threshold, char allow_broadcast, int16_t broadcast_threshold)
{
    using namespace c_actor_gate_jump_traversal;
    uint32_t actor_index = datum;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t result = 0;

    if (threshold > a->pending_panic_type || a->order_committed != 0) {
        a->pending_panic_type = 0;
        return result;
    }

    if (a->mode == halo::ai::actor_mode::flee) {
        int16_t climb = a->mode_data.flee.panic;
        if (climb > 0) {
            if (climb <= a->pending_panic_type) {
                climb = a->pending_panic_type;
            }
            a->mode_data.flee.panic = climb;
            a->pending_panic_type = 0;
            return 0;
        }
    }

    if (a->last_flee_abort_time == -1 || halo::game::globals().game_time->game_time > a->last_flee_abort_time + 7) {
        if (allow_broadcast != 0 && a->pending_panic_type < broadcast_threshold) {
            halo::ai::ai_communication_broadcast(0x22, a->unit_index, halo::k_dword_none, halo::k_dword_none, halo::k_dword_none, halo::k_dword_none, 0);
            a->pending_panic_type = 0;
            return 0;
        }

        result = halo::ai::actor_check_pain_reaction(a->pending_panic_prop_index, (uint8_t)(a->pending_panic_type >= broadcast_threshold),
                                           (uint16_t)a->pending_panic_type, actor_index);
    }

    a->pending_panic_type = 0;
    return result;
}

namespace halo::ai {
uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast, int16_t broadcast_threshold)
{
    return halo::ai::movement_ops(actor_index).gate_jump_traversal(threshold, allow_broadcast, broadcast_threshold);
}
}

namespace c_actor_get_cached_wander_position {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}


/**
 * actor_get_cached_wander_position: behaviour unchanged from the original routine.
 *
 * @address 0x4281f0
 */
uint8_t halo::ai::movement_ops::get_cached_wander_position(real_vector3d *out_position)
{
    using namespace c_actor_get_cached_wander_position;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->target_combat_status > 8 && actor_mode_definitions[self->mode].combat_grade == 4) {
        if (self->grenade_throw_pending != 0) {
            *out_position = self->unit_aiming_vector;
            return 1;
        }
        if (self->firing_target_type > 0) {
            *out_position = self->target_aim_vector;
            return 1;
        }
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_get_cached_wander_position(datum_index actor_index, real_vector3d *out_position)
{
    return halo::ai::movement_ops(actor_index).get_cached_wander_position(out_position);
}
}

namespace c_actor_get_requested_velocity {
}


/**
 * actor_get_requested_velocity: behaviour unchanged from the original routine.
 *
 * @address 0x417fa0
 */
uint8_t halo::ai::movement_ops::get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit)
{
    using namespace c_actor_get_requested_velocity;
    actor *self;
    float length;
    float scale;

    self = halo::ai::actor_at(actor_index);

    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if (self->swarm != 0) {

            halo::ai::actor_dispatch_type_vtable_0x1c(actor_index, object_index, *(uint32_t *)&speed_limit, (uint32_t)out_velocity);
            self->jump_velocity_request.valid = 0;
            return 1;
        }
        if (self->jump_velocity_request.valid != 0) {
            if (self->mode == halo::ai::actor_mode::charge && self->mode_data.charge.stage == 3) {
                skip_clamp = 1;
            }
            out_velocity->j = self->jump_velocity_request.direction.j *
                              self->jump_velocity_request.horizontal_speed;
            out_velocity->k = self->jump_velocity_request.vertical_speed;
            out_velocity->i = self->jump_velocity_request.direction.i *
                              self->jump_velocity_request.horizontal_speed;
            length = (float)halo::libm::sqrt((double)(out_velocity->i * out_velocity->i +
                                          out_velocity->j * out_velocity->j +
                                          out_velocity->k * out_velocity->k));
            if (skip_clamp == 0 && speed_limit < length) {
                scale = speed_limit / length;
                out_velocity->i = scale * out_velocity->i;
                out_velocity->j = scale * out_velocity->j;
                out_velocity->k = scale * out_velocity->k;
            }
        }
    }

    self->jump_velocity_request.valid = 0;
    return 1;
}

namespace halo::ai {
uint8_t actor_get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit)
{
    return halo::ai::movement_ops::get_requested_velocity(skip_clamp, actor_index, out_velocity, object_index, speed_limit);
}
}

