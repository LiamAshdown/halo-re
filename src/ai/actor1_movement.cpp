#include "halo/ai/actor_movement.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

namespace c_actor_avoid_obstacle_and_project {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern ModelCollisionGeometryBSP *global_structure_collision_bsp;
extern const real_vector3d *global_down3d_pointer;

extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                                ModelCollisionGeometryBSP *bsp,
                                                int16_t breakable_surface_count,
                                                uint32_t *breakable_surfaces, real_point3d *origin,
                                                real_vector3d *delta, float max_fraction);

extern double sqrt(double x);

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
}
}

extern "C" uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry, real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index);

/**
 * actor_avoid_obstacle_and_project: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_avoid_obstacle_and_project.c.txt.
 *
 * @address 0x4095c0
 */
uint8_t halo::ai::movement_ops::avoid_obstacle_and_project(datum_index vehicle_index, real_point3d *entry, real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index)
{
    using namespace c_actor_avoid_obstacle_and_project;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *vehicle_tag = TAG_DATA(*(datum_index *)vehicle);
    real_point3d point = *entry;
    uint8_t near_line = in_out_near_line != 0 ? *in_out_near_line : 0;
    collision_bsp_segment_result result;
    real_point3d start;
    real_vector3d delta;

    if ((vehicle_tag[0x17c] & 0x10) == 0) {
        real_point3d center = *(real_point3d *)&((vehicle_object *)vehicle)->base.bounding_center.x;
        float radius = ((vehicle_object *)vehicle)->base.bounding_radius;
        float ax = ((actor *)act)->body_position.x;
        float ay = ((actor *)act)->body_position.y;
        real_point3d *target_pointer;
        real_point3d target;
        real_vector2d to_center;
        real_vector2d to_target;
        real_vector2d from_target;
        real_vector2d away;

        if (*(float *)(vehicle_tag + 0x280) > 0.0f) {
            radius = *(float *)(vehicle_tag + 0x280);
        }
        target_pointer = entry;
        if (!near_line) {
            float dx = hint->x - center.x;
            float dy = hint->y - center.y;
            float dz = hint->z - center.z;
            float d = (float)sqrt(dz * dz + dy * dy + dx * dx);

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
                goto project;
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
                    goto project;
                }
                away.i = -from_target.i;
                away.j = -from_target.j;
            }
        }
        if (!(halo::math::vector2d_normalize_with_length(away) > 0.0f)) {
            goto project;
        }
        point.x = away.i * (radius * 1.1f) + center.x;
        point.y = away.j * (radius * 1.1f) + center.y;
        {
            float dx = point.x - ax;
            float dy = point.y - ay;
            float dz = point.z - ((actor *)act)->body_position.z;
            float distance_squared = dz * dz + dy * dy + dx * dx;
            float distance;
            real_vector3d side;

            if (!(distance_squared > 0.0001f) || !(distance_squared <= 4.0f)) {
                goto project;
            }
            distance = (float)sqrt(distance_squared);
            side.i = -from_target.j;
            side.j = from_target.i;
            if (!(dy * from_target.i + side.i * dx > 0.0f)) {
                side.i = from_target.j;
                side.j = -from_target.i;
            }
            side.k = 0.0f;
            if (!(halo::math::vector2d_normalize_with_length(*((real_vector2d *)&side)) > 0.0f)) {
                goto project;
            }
            distance = 2.0f - distance;
            point.x = side.i * distance + point.x;
            point.y = side.j * distance + point.y;
            point.z = side.k * distance + point.z;
        }
    }
project:
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

extern "C" uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry, real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index)
{
    return halo::ai::movement_ops(actor_index).avoid_obstacle_and_project(vehicle_index, entry, hint, in_out_near_line, out_point, out_surface_index);
}

#undef ACTOR
#undef OBJECT_DATA
#undef TAG_DATA

namespace c_actor_avoidance_build_direction_tables {
extern "C" {
extern double cos(double x);
extern double sin(double x);

extern float actor_avoidance_samples_a[16][7];
extern float actor_avoidance_circle[8][3];
extern float actor_avoidance_samples_b[9][7];

extern const float actor_avoidance_b_radius[9];
extern const float actor_avoidance_b_elevation[9];
extern const float actor_avoidance_b_bearing[9];
extern const float actor_avoidance_a_bearing[8];
extern const float actor_avoidance_a_radius[2];
extern const float actor_avoidance_a_elevation[2];
}
}

extern "C" void actor_avoidance_build_direction_tables(void);

/**
 * actor_avoidance_build_direction_tables: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_avoidance_build_direction_tables.c.txt.
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
        sin_bearing = (float)sin((double)bearing);
        cos_bearing = (float)cos((double)bearing);
        elevation = actor_avoidance_b_elevation[i] * 0.05235988f;
        sin_elevation = (float)sin((double)elevation);
        cos_elevation = (float)cos((double)elevation);
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
        sin_elevation = (float)sin((double)actor_avoidance_a_elevation[i]);
        cos_elevation = (float)cos((double)actor_avoidance_a_elevation[i]);
        radius = actor_avoidance_a_radius[i];

        for (j = 0; j < 8; j++) {
            float *row = actor_avoidance_samples_a[j * 2 + i];

            bearing = actor_avoidance_a_bearing[j];
            actor_avoidance_circle[j][0] = 0.0f;
            actor_avoidance_circle[j][1] = (float)cos((double)bearing);
            actor_avoidance_circle[j][2] = (float)sin((double)bearing);

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

extern "C" void actor_avoidance_build_direction_tables(void)
{
    halo::ai::movement_ops::avoidance_build_direction_tables();
}

namespace c_actor_avoidance_interpolate_sample {
}

extern "C" uint8_t actor_avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples, int16_t count, const float *values, float *out_index, float *out_value);

/**
 * actor_avoidance_interpolate_sample: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_avoidance_interpolate_sample.c.txt.
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

extern "C" uint8_t actor_avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples, int16_t count, const float *values, float *out_index, float *out_value)
{
    return halo::ai::movement_ops::avoidance_interpolate_sample(direction, samples, count, values, out_index, out_value);
}

namespace c_actor_check_step_obstruction {
extern "C" {
extern data_array *actor_data;
extern const real_vector3d *global_down3d_pointer;

extern void actor_update_target_lead_position(datum_index actor_index);
extern uint8_t path_find_test_segment_unobstructed(void *map, real_point3d *point_a, uint8_t ignore_permission,
    int32_t surface_a, real_point3d *point_b, int32_t surface_b, float radius, uint8_t flags,
    path_find_boundary_crossing *out_result);
extern ScenarioStructureBSP *global_structure_bsp;

extern int32_t global_structure_collision_bsp;
}
}

extern "C" uint8_t actor_check_step_obstruction(datum_index actor_index, real_vector2d *direction, float step_distance, float step_up, uint8_t *out_flag, void *extra_param);

/**
 * actor_check_step_obstruction: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_step_obstruction.c.txt.
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & 0xffff].data;

    if (self->flying == 0) {
        uint8_t trace_ok;

        step_point.x = step_distance * direction->i + self->body_position.x;
        step_point.y = step_distance * direction->j + self->body_position.y;
        actor_update_target_lead_position(actor_index);

        trace_ok = path_find_test_segment_unobstructed(global_structure_bsp, (real_point3d *)((uint8_t *)self + 0x168),
            self->ignores_glass, (int32_t)self->pathfinding_surface_index, &step_point, -1, definition->pathfinding_radius, 0,
            (path_find_boundary_crossing *)extra_param);
        if (!trace_ok) {
            obstructed = 1;
            {

                float dz = *(float *)((uint8_t *)extra_param + 0xc) - self->body_position.z;
                if (dz <= step_distance * 0.5f && (step_up != 0.0f || step_distance * -0.5f <= dz)) {
                    goto done;
                }
            }
        }
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
done:
    if (out_flag != 0) {
        *out_flag = used_point_check;
    }
    return obstructed;
}

extern "C" uint8_t actor_check_step_obstruction(datum_index actor_index, real_vector2d *direction, float step_distance, float step_up, uint8_t *out_flag, void *extra_param)
{
    return halo::ai::movement_ops(actor_index).check_step_obstruction(direction, step_distance, step_up, out_flag, extra_param);
}

namespace c_actor_check_vehicle_mode_timeout {
extern "C" {
extern data_array *actor_data;
extern game_time_globals *game_time;
}
}

extern "C" uint8_t actor_check_vehicle_mode_timeout(datum_index actor_index);

/**
 * actor_check_vehicle_mode_timeout: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_vehicle_mode_timeout.c.txt.
 *
 * @address 0x428270
 */
uint8_t halo::ai::movement_ops::check_vehicle_mode_timeout()
{
    using namespace c_actor_check_vehicle_mode_timeout;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->mode == _actor_mode_vehicle &&
        *(int16_t *)&self->mode_data.raw[4] == 3 &&
        self->mode_data.raw[0xb] != 0) {
        int32_t deadline = *(int32_t *)&self->mode_data.raw[0x10] + 0x1e;
        return (int32_t)game_time->game_time <= deadline;
    }
    return 0;
}

extern "C" uint8_t actor_check_vehicle_mode_timeout(datum_index actor_index)
{
    return halo::ai::movement_ops(actor_index).check_vehicle_mode_timeout();
}

namespace c_actor_compute_swarm_avoidance_offset {
extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern data_array *object_data;
extern data_array *prop_data;
extern const real_vector2d *global_forward2d_pointer;

extern double sqrt(double x);
extern uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin,
    real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc,
    real_vector3d *out_direction, real *max_speed_override, real *out_speed,
    real *out_time_of_flight, real *out_range, real *out_half_gravity_term,
    real *out_horizontal_speed);
}
}

extern "C" void actor_compute_swarm_avoidance_offset(datum_index actor_index, datum_index unit_index, float radius, float *out_offset);

/**
 * actor_compute_swarm_avoidance_offset: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_compute_swarm_avoidance_offset.c.txt.
 *
 * @address 0x425c70
 */
void halo::ai::movement_ops::compute_swarm_avoidance_offset(datum_index unit_index, float radius, float *out_offset)
{
    using namespace c_actor_compute_swarm_avoidance_offset;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm_index == (datum_index)k_datum_index_none) {
        return;
    }

    {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;

        for (i = 0; i < s->component_count; i++) {
            if (s->unit_index[i] == unit_index) {
                object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
                swarm_component *component = &((swarm_component *)swarm_component_data->data)[s->component_index[i] & 0xffff];
                uint16_t flags = *(uint16_t *)&((struct swarm_component *)component)->flags;
                datum_index target = component->leap_target_index;

                if ((flags & 1) == 0 || target == (datum_index)k_datum_index_none) {
                    if ((flags & 8) != 0 && (flags & 0x10) != 0) {
                        uint8_t byte21 = *((uint8_t *)component + 0x21);
                        if ((byte21 & 4) != 0 && (byte21 & 0x10) != 0) {
                            real_vector2d dir;
                            dir.i = unit_object->forward.i;
                            dir.j = unit_object->forward.j;
                            if (halo::math::vector2d_normalize_with_length(dir) == 0.0f) {
                                dir.i = ((struct object *)unit_object)->up.i;
                                dir.j = ((struct object *)unit_object)->up.j;
                                if (halo::math::vector2d_normalize_with_length(dir) == 0.0f) {
                                    dir.i = global_forward2d_pointer->i;
                                    dir.j = global_forward2d_pointer->j;
                                }
                            }
                            {
                                float scale = *((float *)((uint8_t *)component + 0x28));
                                float z = *((float *)((uint8_t *)component + 0x2c));
                                out_offset[0] = dir.i * scale;
                                out_offset[1] = dir.j * scale;
                                out_offset[2] = z;
                            }
                        }
                        *((uint8_t *)component + 2) &= 0xef;
                    }
                } else {

                    const uint8_t *target_prop = (const uint8_t *)prop_data->data + (target & 0xffff) * 0x138;
                    real max_time = 0.7f;
                    real half_gravity;
                    real horizontal_speed;
                    real_vector3d leap;

                    if (!(radius > 0.12f)) {
                        radius = 0.12f;
                    }
                    if (projectile_solve_ballistic_arc((real_point3d *)(target_prop + 0xc8),
                            (real_point3d *)((uint8_t *)component + 0x4), radius, 1.0f, &max_time, 0,
                            &leap, 0, 0, 0, 0, &half_gravity, &horizontal_speed)) {
                        float x, y, sum_sq;

                        if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                            leap = *(real_vector3d *)&((struct actor *)self)->facing.i;
                            if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&leap)) == 0.0f) {
                                leap = *halo::math::globals().global_forward3d_pointer;
                            }
                        }
                        if (target_prop[0x130] == 0 && !(half_gravity <= 0.075f)) {
                            half_gravity = 0.075f;
                        }
                        x = leap.i * horizontal_speed;
                        y = leap.j * horizontal_speed;
                        out_offset[2] = half_gravity;
                        out_offset[0] = x;
                        out_offset[1] = y;
                        sum_sq = y * y + x * x + half_gravity * half_gravity;
                        if (!(sum_sq <= radius * radius)) {
                            float k = radius / (float)sqrt((double)sum_sq);
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

extern "C" void actor_compute_swarm_avoidance_offset(datum_index actor_index, datum_index unit_index, float radius, float *out_offset)
{
    halo::ai::movement_ops(actor_index).compute_swarm_avoidance_offset(unit_index, radius, out_offset);
}

namespace c_actor_create_swarm {
extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern data_array *object_data;

extern void object_get_position(real_point3d *out_position, datum_index object_index);
}
}

extern "C" datum_index actor_create_swarm(datum_index actor_index);

/**
 * actor_create_swarm: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_create_swarm.c.txt.
 *
 * @address 0x427f40
 */
datum_index halo::ai::movement_ops::create_swarm()
{
    using namespace c_actor_create_swarm;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm_index == (datum_index)k_datum_index_none) {
        self->swarm_index = halo::memory::datum_new(swarm_data);
        if (self->swarm_index != (datum_index)k_datum_index_none) {
            datum_index unit_index = self->cluster_unit_index;
            swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];

            s->actor_index = actor_index;
            s->component_count = 0;

            while (unit_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
                object *unit_object = header->data;
                datum_index component_index = halo::memory::datum_new(swarm_component_data);

                if (component_index == (datum_index)k_datum_index_none) {
                    return self->swarm_index;
                }

                {
                    swarm *s2 = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
                    swarm_component *component = &((swarm_component *)swarm_component_data->data)[component_index & 0xffff];
                    datum_index marker;

                    component->leap_target_index = 0xffffffff;
                    s2->unit_index[s2->component_count] = unit_index;
                    s2->component_index[s2->component_count] = component_index;
                    s2->component_count = s2->component_count + 1;

                    marker = (unit_object->type == 0) ? *(datum_index *)((uint8_t *)unit_object + 0x4d8)
                                                       : (datum_index)k_datum_index_none;
                    object_get_position(&component->position, unit_index);
                    component->marker_index = marker;
                }

                unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc);
            }
        }
    }
    return self->swarm_index;
}

extern "C" datum_index actor_create_swarm(datum_index actor_index)
{
    return halo::ai::movement_ops(actor_index).create_swarm();
}

namespace c_actor_delete_swarm {
extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;

}
}

extern "C" void actor_delete_swarm(datum_index actor_index);

/**
 * actor_delete_swarm: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_delete_swarm.c.txt.
 *
 * @address 0x4280b0
 */
void halo::ai::movement_ops::delete_swarm()
{
    using namespace c_actor_delete_swarm;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index swarm_index = self->swarm_index;

    if (swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[swarm_index & 0xffff];
        int16_t i;

        for (i = 0; i < s->component_count; i++) {
            halo::memory::datum_delete(swarm_component_data, s->component_index[i]);
        }
        halo::memory::datum_delete(swarm_data, swarm_index);
        self->swarm_index = (datum_index)k_datum_index_none;
    }
}

extern "C" void actor_delete_swarm(datum_index actor_index)
{
    halo::ai::movement_ops(actor_index).delete_swarm();
}

namespace c_actor_evaluate_search_node {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;

extern double sqrt(double x);
extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index);
extern uint8_t unit_seat_flag_bit10(uint32_t unit_index, int16_t seat_index);
extern uint8_t unit_seat_flag_bit3(uint32_t unit_index, int16_t seat_index);
extern uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint);
extern void object_get_position(real_point3d *out_position, datum_index object_index);

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)

static uint8_t *actor_try_get(datum_index handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *record;

    if (index < 0 || index >= actor_data->maximum_count) {
        return 0;
    }
    record = (uint8_t *)actor_data->data + actor_data->size * index;
    if (*(int16_t *)record == 0 || (salt != 0 && *(int16_t *)record != salt)) {
        return 0;
    }
    return record;
}
}
}

extern "C" uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front);

/**
 * actor_evaluate_search_node: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_evaluate_search_node.c.txt.
 *
 * @address 0x4091d0
 */
uint8_t halo::ai::movement_ops::evaluate_search_node(datum_index vehicle_index, int16_t seat_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front)
{
    using namespace c_actor_evaluate_search_node;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *variant = TAG_DATA(((actor *)act)->actor_variant_tag);
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

    if (unit_is_seat_occupied((int32_t)vehicle_index, seat_index)) {
        return 0;
    }
    if ((TAG_DATA(((actor *)act)->actor_definition_tag)[0x4] & 8) && !unit_seat_flag_bit10(vehicle_index, seat_index)) {
        return 0;
    }
    if (!unit_find_weapon_marker_transform(((actor *)act)->unit_index, vehicle_index, seat_index, &entry, &seat,
                                           &hint)) {
        return 0;
    }
    object_get_position((real_point3d *)&direction, vehicle_index);
    direction.i = seat.x - entry.x;
    direction.k = 0.0f;
    direction.j = seat.y - entry.y;
    if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&direction)) == 0.0f) {
        direction = *(real_vector3d *)&((actor *)act)->facing.i;
    }
    ax = ((actor *)act)->body_position.x;
    ay = ((actor *)act)->body_position.y;
    if (sqrt((seat.y - ay) * (seat.y - ay) + (seat.x - ax) * (seat.x - ax)) <
        sqrt((entry.y - ay) * (entry.y - ay) + (entry.x - ax) * (entry.x - ax))) {
        distance = (float)sqrt((seat.y - ay) * (seat.y - ay) + (seat.x - ax) * (seat.x - ax));
    } else {
        distance = (float)sqrt((entry.y - ay) * (entry.y - ay) + (entry.x - ax) * (entry.x - ax));
    }
    for (prop_index = ((actor *)act)->first_prop; prop_index != k_datum_index_none;) {
        uint8_t *prop = (uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138;
        datum_index other_index = ((struct prop *)prop)->owner_actor_index;

        prop_index = ((struct prop *)prop)->next_in_actor;
        if (prop[0x60] == 0 && other_index != k_datum_index_none) {
            uint8_t *other = actor_try_get(other_index);

            if (other != 0 && *(int16_t *)(other + 0x6c) == 9 && *(datum_index *)(other + 0x9c) == vehicle_index &&
                *(int16_t *)(other + 0xa0) == seat_index) {
                float dx = *(float *)(other + 0xcc) - *(float *)(other + 0x12c);
                float dy = *(float *)(other + 0xd0) - *(float *)(other + 0x130);

                if (distance * distance > dy * dy + dx * dx) {
                    return 0;
                }
            }
        }
    }
    to_seat.i = seat.x - ax;
    to_seat.j = seat.y - ay;
    halo::math::vector2d_normalize_with_length(to_seat);
    dot = to_seat.j * ((actor *)act)->facing.j + to_seat.i * ((actor *)act)->facing.i;
    close = (uint8_t)(distance < 0.7f);
    facing = (uint8_t)(dot > 0.6f);
    in_front = (uint8_t)(distance < 1.1f && dot > 0.0f);
    score = 10.0f / (distance + 1.0f);
    if ((unit_seat_flag_bit3(vehicle_index, seat_index) != 0) != ((variant[0] & 0x80) != 0)) {
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

extern "C" uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front)
{
    return halo::ai::movement_ops(actor_index).evaluate_search_node(vehicle_index, seat_index, out_entry, out_direction, out_hint, out_score, out_close, out_facing, out_in_front);
}

#undef ACTOR
#undef TAG_DATA

namespace c_actor_fill_unit_position_context {
extern "C" {
extern data_array *object_data;
extern char ai_marker_name_a[];

extern void object_get_position(real_point3d *out_position, datum_index object_index);
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker,
    uint32_t flags);
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity);

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}
}
}

extern "C" void actor_fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context);

/**
 * actor_fill_unit_position_context: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_fill_unit_position_context.c.txt.
 *
 * @address 0x4296c0
 */
void halo::ai::movement_ops::fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context)
{
    using namespace c_actor_fill_unit_position_context;
    uint8_t *context = (uint8_t *)out_context;
    uint8_t *unit = object_get(unit_index);
    object_marker marker;
    datum_index root = k_datum_index_none;
    uint8_t *root_object;

    object_get_position((real_point3d *)(context + 0xc), unit_index);
    *(real_vector3d *)&((struct actor_unit_position_context *)context)->forward.i = *(real_vector3d *)&((unit_object *)unit)->base.forward.i;
    object_get_node_local_transform(unit_index, ai_marker_name_a, &marker, 1);
    *(real_point3d *)context = marker.node_transform.position;
    object_get_root_object_velocities(unit_index, (real_vector3d *)(context + 0x2c), 0);
    if (unit_index != k_datum_index_none) {
        datum_index cursor = unit_index;

        do {
            root = cursor;
            cursor = *(datum_index *)(object_get(cursor) + 0x11c);
        } while (cursor != k_datum_index_none);
    }
    root_object = object_get(root);
    *(uint32_t *)&((struct actor_unit_position_context *)context)->root_position_x = *(uint32_t *)(root_object + 0x98);
    *(uint32_t *)&((struct actor_unit_position_context *)context)->root_position_y = *(uint32_t *)(root_object + 0x9c);
}

extern "C" void actor_fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context)
{
    halo::ai::movement_ops::fill_unit_position_context(unit_index, out_context);
}

namespace c_actor_find_best_search_node {
extern "C" {
extern data_array *object_data;

extern uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score,
    uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front);
}
}

extern "C" int16_t actor_find_best_search_node(datum_index actor_index, datum_index vehicle_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint);

/**
 * actor_find_best_search_node: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_best_search_node.c.txt.
 *
 * @address 0x409070
 */
int16_t halo::ai::movement_ops::find_best_search_node(datum_index vehicle_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint)
{
    using namespace c_actor_find_best_search_node;
    datum_index actor_index = datum;
    uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)((object_header *)object_data->data)
                                                        [vehicle_index & 0xffff].data & 0xffff].data;
    int16_t best_seat = -1;
    float best_score = 0.0f;
    real_point3d best_entry = {0.0f, 0.0f, 0.0f};
    real_vector3d best_direction = {0.0f, 0.0f, 0.0f};
    real_point3d best_hint = {0.0f, 0.0f, 0.0f};
    int16_t i;

    for (i = 0; i < *(int32_t *)(vehicle_tag + 0x2e4); i++) {
        real_point3d entry;
        real_vector3d direction;
        real_point3d hint;
        float score;

        if (actor_evaluate_search_node(actor_index, vehicle_index, i, &entry, &direction, &hint, &score, 0, 0, 0) &&
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

extern "C" int16_t actor_find_best_search_node(datum_index actor_index, datum_index vehicle_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint)
{
    return halo::ai::movement_ops(actor_index).find_best_search_node(vehicle_index, out_entry, out_direction, out_hint);
}

namespace c_actor_gate_jump_traversal {
extern "C" {
extern data_array *actor_data;
extern game_time_globals *game_time;

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);

extern uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base,
    uint16_t order_code, datum_index actor_index);
}
}

extern "C" uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast, int16_t broadcast_threshold);

/**
 * actor_gate_jump_traversal: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_gate_jump_traversal.c.txt.
 *
 * @address 0x40a700
 */
uint8_t halo::ai::movement_ops::gate_jump_traversal(int16_t threshold, char allow_broadcast, int16_t broadcast_threshold)
{
    using namespace c_actor_gate_jump_traversal;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t result = 0;

    if (threshold > a->pending_panic_type || a->order_committed != 0) {
        a->pending_panic_type = 0;
        return result;
    }

    if (a->mode == 4) {
        int16_t climb = *(int16_t *)(a->mode_data.raw + (0xa8 - 0x9c));
        if (climb > 0) {
            if (climb <= a->pending_panic_type) {
                climb = a->pending_panic_type;
            }
            *(int16_t *)(a->mode_data.raw + (0xa8 - 0x9c)) = climb;
            a->pending_panic_type = 0;
            return 0;
        }
    }

    if (a->last_flee_abort_time == -1 || game_time->game_time > a->last_flee_abort_time + 7) {
        if (allow_broadcast != 0 && a->pending_panic_type < broadcast_threshold) {
            ai_communication_broadcast(0x22, a->unit_index, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0);
            a->pending_panic_type = 0;
            return 0;
        }

        result = actor_check_pain_reaction(a->pending_panic_prop_index, (uint8_t)(a->pending_panic_type >= broadcast_threshold),
                                           (uint16_t)a->pending_panic_type, actor_index);
    }

    a->pending_panic_type = 0;
    return result;
}

extern "C" uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast, int16_t broadcast_threshold)
{
    return halo::ai::movement_ops(actor_index).gate_jump_traversal(threshold, allow_broadcast, broadcast_threshold);
}

namespace c_actor_get_cached_wander_position {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
}
}

extern "C" uint8_t actor_get_cached_wander_position(datum_index actor_index, real_vector3d *out_position);

/**
 * actor_get_cached_wander_position: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_cached_wander_position.c.txt.
 *
 * @address 0x4281f0
 */
uint8_t halo::ai::movement_ops::get_cached_wander_position(real_vector3d *out_position)
{
    using namespace c_actor_get_cached_wander_position;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->target_combat_status > 8 && actor_mode_definitions[self->mode].combat_grade == 4) {
        if (self->grenade_throw_pending != 0) {
            *out_position = self->facing_unknown_180;
            return 1;
        }
        if (self->firing_target_type > 0) {
            *out_position = *(real_vector3d *)&self->target_aim_vector[0];
            return 1;
        }
    }
    return 0;
}

extern "C" uint8_t actor_get_cached_wander_position(datum_index actor_index, real_vector3d *out_position)
{
    return halo::ai::movement_ops(actor_index).get_cached_wander_position(out_position);
}

namespace c_actor_get_requested_velocity {
extern "C" {
extern data_array *actor_data;

extern double sqrt(double x);

extern void actor_dispatch_type_vtable_0x1c(datum_index actor_index, uint32_t a, uint32_t b, uint32_t c);
}
}

extern "C" uint8_t actor_get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit);

/**
 * actor_get_requested_velocity: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_requested_velocity.c.txt.
 *
 * @address 0x417fa0
 */
uint8_t halo::ai::movement_ops::get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit)
{
    using namespace c_actor_get_requested_velocity;
    actor *self;
    float length;
    float scale;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if (self->swarm != 0) {

            actor_dispatch_type_vtable_0x1c(actor_index, object_index, *(uint32_t *)&speed_limit, (uint32_t)out_velocity);
            self->jump_velocity_request[0] = 0;
            return 1;
        }
        if (self->jump_velocity_request[0] != 0) {
            if (self->mode == 10 && *(int16_t *)&self->mode_data.raw[4] == 3) {
                skip_clamp = 1;
            }
            out_velocity->j = *(float *)&self->jump_velocity_request[8] *
                              *(float *)&self->jump_velocity_request[12];
            out_velocity->k = *(float *)&self->jump_velocity_request[16];
            out_velocity->i = *(float *)&self->jump_velocity_request[4] *
                              *(float *)&self->jump_velocity_request[12];
            length = (float)sqrt((double)(out_velocity->i * out_velocity->i +
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

    self->jump_velocity_request[0] = 0;
    return 1;
}

extern "C" uint8_t actor_get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit)
{
    return halo::ai::movement_ops::get_requested_velocity(skip_clamp, actor_index, out_velocity, object_index, speed_limit);
}

