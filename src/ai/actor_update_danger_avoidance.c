// actor_update_danger_avoidance  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40c040, size 1258 bytes
// name confidence: 0.4  rewrite confidence: 0.75
// objdump 0x40c040..0x40c529, raw actor offsets (0x724 each). A danger (+0x280 kind: 1, 2, 3; +0x287 live,
//   +0x28a not yet handled) whose sphere (centre +0x2dc, radius +0x2d8 + 3) contains the actor position (+0x12c),
//   for an actor with no queued secondary action and not busy (+0x160):
//   - in_danger: the actor is within the danger radius (+0x294) of the danger path (+0x2b0 .. +0x2c8); near: within
//     radius + 3.5. When the movement action is still running, the path is also tested against the movement
//     destination (+0x4ac, the result "towards") and, with a moving target (+0x504) and not in danger, the
//     movement segment (+0x518 * 3 from the position) against the path ("crossing", local +0x0a);
//   - towards or in danger with a recognition slot (+0x3b8) pushes a recognition entry (type 1);
//   - in danger: the time to reach the sphere (ray_intersect_sphere_distance) at 45 ticks per unit decides:
//     kind 3 below 30 ticks; kinds 1/2 already inside (t == 0) with the danger's own countdown +0x2e8 below
//     30/20; a first reaction (+0x282 not yet 0.. or reacting) broadcasts event 12 (kind 2 only) with the reason
//     3/2/1 from +0x282, once (+0x289);
//   - an escape position (0x40bc40) found while +0x288 is set and no firing target (+0x158) is taken with 0x40e060
//     (distance 8 when the actor definition has flag 0x2000000, else 0) when the kind and timing call for it;
//   - otherwise a near danger, unless the actor's firing point rules forbid it (+0x4a8 without +0x484 while
//     moving and not crossing) or it is committed (+0x375), switches a combat-grade 1 or 3 mode to avoid (13).
//   Returns whether it acted (escape taken or avoid mode set).
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "fn_ai.h"
#include "fn_math.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254


extern real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction,
    real_point3d *point); // 0x4cde30, EAX, ECX, EDX

extern real segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start, real_vector3d *a_direction,
    real_vector3d *b_direction); // 0x4cdef0, stack, EBX, ESI, EDI


extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340


#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

uint8_t actor_update_danger_avoidance(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t result = 0;
    uint8_t crossing = 0;
    uint8_t in_danger;
    uint8_t near;
    uint8_t reacting;
    real_vector3d path_delta;
    float radius_squared;
    float danger_time;
    real_point3d *position = (real_point3d *)(actor + 0x12c);
    real_point3d *path_start = (real_point3d *)(actor + 0x2b0);

    if (W(0x280) == 0 || B(0x287) == 0 || B(0x28a) != 0) {
        return 0;
    }
    {
        float dx = F(0x2dc) - position->x;
        float dy = F(0x2e0) - position->y;
        float dz = F(0x2e4) - position->z;
        float r = F(0x2d8) + 3.0f;

        if (r * r < dz * dz + dy * dy + dx * dx) {
            return 0;
        }
    }
    if (actor_action_has_queued_secondary(actor_index) || B(0x160) != 0) {
        return 0;
    }

    path_delta.i = F(0x2c8) - path_start->x;
    path_delta.j = F(0x2cc) - path_start->y;
    path_delta.k = F(0x2d0) - path_start->z;
    {
        float distance_squared = point3d_distance_squared_to_segment(path_start, &path_delta, position);
        float radius = F(0x294);
        float wide = radius + 3.5f;

        radius_squared = radius * radius;
        in_danger = distance_squared < radius_squared;
        near = distance_squared < wide * wide;
    }

    {
        uint8_t towards;

        if (actor_movement_action_is_complete(actor_index) == 0) {
            towards = in_danger;
            crossing = 0;
        } else {
            towards = point3d_distance_squared_to_segment(path_start, &path_delta, (real_point3d *)(actor + 0x4ac)) <
                radius_squared;
            if (B(0x504) != 0 && !in_danger && !towards) {
                real_vector3d movement;

                movement.i = F(0x518) * 3.0f;
                movement.j = F(0x51c) * 3.0f;
                movement.k = F(0x520) * 3.0f;
                if (segment3d_distance_squared_to_segment(path_start, position, &movement, &path_delta) <
                    radius_squared) {
                    crossing = 1;
                    towards = 1;
                } else {
                    crossing = 0;
                    goto flee_check;
                }
            }
        }
        if (towards && W(0x3b8) != -1) {
            actor_push_recognition_entry(actor_index, W(0x3b8), 1);
        }
    }
    if (!in_danger) {
        goto flee_check;
    }

    in_danger = 0;
    danger_time = ray_intersect_sphere_distance(position, path_start, &path_delta, F(0x294)); // 0x40c2a2 fst
    if (danger_time < 3.4028234663852886e+38f) {
        danger_time = danger_time * 45.0f;
    }
    reacting = 0;
    switch (W(0x280)) {
    case 3:
        if (danger_time < 30.0f) {
            reacting = 1;
        }
        break;
    case 2:
        if (danger_time == 0.0f && W(0x2e8) != -1 && W(0x2e8) < 0x14) {
            in_danger = 1;
        }
        break;
    case 1:
        if (danger_time == 0.0f && W(0x2e8) != -1 && W(0x2e8) < 0x1e) {
            in_danger = 1;
        }
        break;
    default:
        break;
    }
    if (W(0x280) != 3 || !reacting) {
        reacting = in_danger;
    }

    if ((W(0x282) == 0 || reacting) && B(0x289) == 0) {
        int32_t reason;

        switch (W(0x282)) {
        case 0: reason = 3; break;
        case 1: reason = 2; break;
        case 2: reason = 1; break;
        default: reason = -1; break;
        }
        if (W(0x280) == 2) {
            ai_communication_broadcast(0xc, D(0x18), 0xffffffff, reason, 0xffffffff, 0xffffffff, 0);
        }
        B(0x289) = 1;
    }

    {
        uint32_t escape = 0;
        uint8_t escape_position[0x40];
        uint8_t found = actor_find_danger_escape(actor_index, &escape, escape_position, &path_delta, &in_danger);

        if ((int16_t)escape != -1 && B(0x288) != 0 && D(0x158) == 0xffffffff) {
            int take = 0;

            switch (W(0x280)) {
            case 2:
                take = (found || in_danger) ? (danger_time < 7.0f) : 0;
                break;
            case 1:
                take = (found || in_danger) ? (danger_time == 0.0f) : 0;
                break;
            case 3:
                take = 0;
                break;
            default:
                goto flee_check;
            }
            if (take || reacting) {
                uint8_t *definition = (uint8_t *)tag_instances[D(0x58) & 0xffff].data;
                float distance = (*(uint32_t *)definition & 0x2000000) ? 8.0f : 0.0f;

                result = actor_take_danger_escape(&path_delta, actor_index, escape, *(uint32_t *)escape_position,
                    distance);
                if (result) {
                    return result;
                }
            }
        }
    }

flee_check:
    if (!near) {
        return result;
    }
    if (B(0x4a8) != 0 && B(0x484) == 0 && B(0x504) != 0 && crossing == 0) {
        return result;
    }
    if (B(0x375) != 0) {
        return result;
    }
    {
        int16_t grade = actor_mode_definitions[W(0x6c)].combat_grade;

        if (grade == 1 || grade == 3) {
            uint32_t mode_data[0x21];

            mode_data[0] = 0;
            actor_set_mode(actor_index, 0xd, mode_data);
            result = 1;
        }
    }
    return result;
}
