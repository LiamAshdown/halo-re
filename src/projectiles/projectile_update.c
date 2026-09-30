// projectile_update  (Ghidra: FUN_004bdc00; renamed per out/phase4/projectiles_types_notes.md; 0x4be1b0 is a
//   mid-function address Ghidra promoted to a bogus "resolution_list_add_resolution", not a function)
// address 0x4bdc00, size 3873 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4bdc00..0x4beb20. The draft's homing crossed the vectors the wrong way round (turning
//   away from the target) and never fetched the target's eye, rotated nothing (argless rotate calls), called the
//   cross product with NULL pointers when orienting a projectile along its velocity (a crash on the first such
//   shot), built the fly-by sound from a guessed bundle, relinked with an uninitialised leaf and advanced the
//   contrail without its handle. Per step: drop the contrail of a non-tracer, advance arming / deceleration delay /
//   detonation timer (raising the state to 1), then while flying (or arming) and free: homing (tag +0x1ec rad/s,
//   difficulty-scaled against players, wander around the target's eye within 10), deceleration to the final
//   velocity (+0x1e8, rate +0x25c) or vanishing when nothing can happen (state 2), gravity (air +0x1cc / water
//   +0x1d8), the maximum range (+0x1c8, state 1), up to 10 collisions (0x4c0450 / 0x4bf390, impact noise), the
//   travelled distance, the local player's fly-by sound (+0x210), orientation along the velocity or the spin
//   (+0x264 axis, +0x270 / +0x274 sin/cos), commit and contrail. Then detonate (state 1, armed) and delete.
// blam-cc: stack -> projectile_index

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_vector3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"
#include "game.h"
#include "sound.h"
#include "physics.h"
#include "fn_game.h"
#include "fn_sound.h"
#include "fn_math.h"
#include <string.h>

extern data_array *object_data;        // 0x008603b0
extern tag_instance *tag_instances;    // 0x0087bc14
extern game_time_globals *game_time;   // 0x006f1d6c
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;        // 0x0087a480
extern real_vector3d *global_origin3d_pointer; // 0x00696714
extern game_main_globals *main_game_globals; // 0x006b0b80
extern float k_physics_gravity;           // 0x0069c52c

extern void contrail_delete(datum_index attachment_handle); // 0x44cad0
extern void projectile_update_function_values(datum_index projectile_index); // 0x4c0250
extern void projectile_request_state(datum_index projectile_index, int16_t requested_state); // 0x4bf0f0, EAX, ECX
extern uint8_t projectile_collision_test(uint32_t object_index, real_point3d *target, void *out_record); // 0x4c0450, EAX, EDI, stack
extern void projectile_response(datum_index projectile_index, collision_result *hit, real_point3d *out_position,
    real_vector3d *velocity); // 0x4bf390, stack + EAX
extern void ai_accumulate_repeated_event(datum_index object_index, real_point3d *origin, int32_t kind,
    int16_t noise, int32_t unused); // 0x42c610

extern void unit_get_secondary_eye_marker_position(uint32_t object_index, real_point3d *out); // 0x569280, ECX, ESI
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0, EAX, stack
extern double cos(double x);
extern double sin(double x);
extern double sqrt(double x);
extern real vector3d_magnitude_squared(real_vector3d *v); // 0x401000, EAX


extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX, ECX, stack

extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310
extern void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time); // 0x44ca60, EDI, stack
extern void projectile_send_detonation(datum_index projectile_index); // 0x4bda60
extern void projectile_detonate(uint32_t object_index, char first_collision, real remaining_tick_fraction); // 0x4c0670, EBX, stack
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define F(p, o) (*(float *)((p) + (o)))

static void projectile_raise_state(uint32_t projectile_index, int16_t state)
{
    uint8_t *o = OBJECT_DATA(projectile_index);

    if (((projectile_object *)o)->projectile.state < state) {
        ((projectile_object *)o)->projectile.state = state;
    }
}

int projectile_update(uint32_t projectile_index)
{
    uint8_t *obj = OBJECT_DATA(projectile_index);                 // ebx
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data; // [esp+0x38]
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *up = (real_vector3d *)(obj + 0x80);
    real remaining = 1.0f;          // [esp+0x18]
    int16_t collisions = 0;         // [esp+0x3c]
    uint8_t flyby_played = 0;       // [esp+0x2f]
    collision_result hit;           // [esp+0x118]; its leaf relinks the projectile every step

    memset(&hit, 0, sizeof(hit));

    // 0x4bdc3d: a non-tracer round drops its contrail
    if (!(((projectile_object *)obj)->projectile.flags & 2) && ((projectile_object *)obj)->projectile.contrail_attachment_index != -1) {
        int32_t slot = ((projectile_object *)obj)->projectile.contrail_attachment_index;

        if (((projectile_object *)obj)->base.attachment_handles[slot] != k_datum_index_none) {
            contrail_delete(((projectile_object *)obj)->base.attachment_handles[slot]);
        }
        *(datum_index *)(obj + 0x14c + ((projectile_object *)obj)->projectile.contrail_attachment_index * 4) = k_datum_index_none;
        ((projectile_object *)obj)->projectile.contrail_attachment_index = -1;
    }
    F(obj, 0x248) += F(obj, 0x24c); // arming
    F(obj, 0x254) = F(obj, 0x258) + F(obj, 0x254); // deceleration delay
    {
        int16_t starts = ((Projectile *)tag)->detonation_timer_starts;
        uint8_t condition = (starts == 1 || starts == 2) ? (uint8_t)((((projectile_object *)obj)->projectile.flags >> 4) & 1) : 1;
        uint32_t flags = ((projectile_object *)obj)->projectile.flags;

        if ((flags & 0x20) || (flags & 8) || condition) {
            if (!(flags & 0x20)) {
                ((projectile_object *)obj)->projectile.flags = flags | 0x20;
            }
            F(obj, 0x240) = F(obj, 0x244) + F(obj, 0x240);
            if (!(F(obj, 0x240) < 1.0f)) {
                projectile_raise_state(projectile_index, 1);
            }
        }
    }
    projectile_update_function_values(projectile_index);

    for (;;) {
        int16_t state = ((projectile_object *)obj)->projectile.state;
        real_vector3d vel;          // [esp+0x20], the velocity carried out of this step
        real_vector3d step;         // [esp+0x5c], this step's displacement per tick
        real_point3d swept;         // [esp+0x68]
        real speed;                 // [esp+0x14]
        real speed_after;           // [esp+0x34]
        real average_speed;         // [esp+0x40]
        real gravity;               // [esp+0x10]
        real vel_k;                 // st(0) carried from the deceleration branches
        real step_k;
        real scale;
        uint8_t collision_attempted = 0; // [esp+0x2e]
        datum_index shooter;        // [esp+0x54]

        if (state != 0 && !(state == 1 && F(obj, 0x24c) != 0.0f && F(obj, 0x248) < 1.0f)) {
            break;
        }
        if ((((projectile_object *)obj)->projectile.flags & 8) || (((projectile_object *)obj)->base.flags & 0x20) ||
            ((projectile_object *)obj)->base.parent_object != k_datum_index_none) {
            break;
        }
        vel = *velocity;
        speed = (real)sqrt(vel.i * vel.i + vel.j * vel.j + vel.k * vel.k);
        speed_after = speed;
        average_speed = speed;
        step = vel;
        shooter = ((projectile_object *)obj)->projectile.ignore_object_index;

        // 0x4bde1c: homing toward the tracked object's eye, wandering as it closes in
        if (((projectile_object *)obj)->projectile.tracked_object_index != k_datum_index_none && ((Projectile *)tag)->guided_angular_velocity > 0.0f) {
            datum_index tracked_index = ((projectile_object *)obj)->projectile.tracked_object_index;
            uint8_t *tracked = OBJECT_DATA(tracked_index);
            real turn = ((Projectile *)tag)->guided_angular_velocity * 0.033333335f;   // [esp+0x1c]
            real fade;                                  // [esp+0x30]
            real distance;
            real_point3d target;                        // [esp+0x74]
            real_vector3d to_target;                    // [esp+0x80]
            real_vector3d axis;                         // [esp+0xf4]
            int32_t salt = (int32_t)projectile_index >> 16;
            int32_t tick = game_time->game_time;
            real angle_a;
            real angle_b;

            if (((1u << (tracked[0xb4] & 0x1f)) & 3) && *(datum_index *)(tracked + 0x218) != k_datum_index_none) {
                turn *= weapon_get_zoom_fov(0x13, main_game_globals->difficulty);
            }
            {
                real dx = F(obj, 0xa0) - F(tracked, 0xa0);
                real dy = F(obj, 0xa4) - F(tracked, 0xa4);
                real dz = F(obj, 0xa8) - F(tracked, 0xa8);

                distance = (real)sqrt(dx * dx + dy * dy + dz * dz);
            }
            if (!(distance <= 10.0f)) {
                fade = 1.0f;
            } else if (distance <= 2.0f) {
                fade = 0.0f;
            } else {
                fade = (distance - 2.0f) * 0.125f;
                if (fade < 0.0f) {
                    fade = 0.0f;
                } else if (!(fade <= 1.0f)) {
                    fade = 1.0f;
                }
            }
            unit_get_secondary_eye_marker_position(tracked_index, &target);
            angle_a = periodic_function_evaluate(_periodic_function_wander,
                (double)((real)(int32_t)((salt * 7 + tick) & 0xffff) * 0.011111111f)) * 6.2831855f;
            angle_b = 3.1415927f - periodic_function_evaluate(_periodic_function_wander,
                (double)((real)(int32_t)((tick + salt * 3) & 0xffff) * 0.011111111f)) * 1.5707964f;
            {
                real cos_b = (real)cos(angle_b);
                real wander_x = (real)cos(angle_a) * cos_b;
                real wander_y = (real)sin(angle_a) * cos_b;
                real wander_z = (real)sin(angle_b);

                target.x += wander_x * fade;
                target.y += wander_y * fade;
                target.z += wander_z * fade;
            }
            to_target.i = target.x - F(obj, 0x5c);
            to_target.j = target.y - F(obj, 0x60);
            to_target.k = target.z - F(obj, 0x64);
            vector3d_cross_product(&axis, &to_target, velocity);
            if (to_target.k * velocity->k + to_target.j * velocity->j + to_target.i * velocity->i > 0.0f &&
                vector3d_normalize_with_length(&axis) > 0.0f) {
                vector3d_rotate_about_axis(&vel, &axis, (real)sin(turn), (real)cos(turn));
            }
        }

        // 0x4be11e: slow down to the final velocity once the deceleration delay has run
        vel_k = vel.k;
        if (!(F(obj, 0x254) < 1.0f)) {
            real final_speed = F(tag, 0x1e8);

            if (speed > final_speed && F(obj, 0x25c) != 0.0f) {
                real drop = remaining * F(obj, 0x25c);

                speed_after = speed - drop;
                if (!(speed_after > final_speed)) {
                    // reaches the final velocity inside this step
                    real f = (speed - final_speed) / drop;   // [esp+0x1c]
                    real g = 1.0f - f;                      // [esp+0x10]
                    real ratio;

                    speed_after = final_speed * 0.99f;
                    average_speed = (speed_after + speed) * f * 0.5f + g * final_speed;
                    ratio = speed_after / speed;
                    vel.i *= ratio;
                    vel.j *= ratio;
                    vel_k = ratio * vel.k;
                    step.i = (vel.i + velocity->i) * f * 0.5f + g * vel.i;
                    step.j = (vel.j + velocity->j) * f * 0.5f + g * vel.j;
                    step.k = (vel_k + velocity->k) * f * 0.5f + g * vel_k;
                } else {
                    real ratio;

                    average_speed = speed - drop * 0.5f;
                    ratio = speed_after / speed;
                    vel.i *= ratio;
                    vel.j *= ratio;
                    vel_k = ratio * vel.k;
                    step.i = (vel.i + velocity->i) * 0.5f;
                    step.j = (vel.j + velocity->j) * 0.5f;
                    step.k = (vel_k + velocity->k) * 0.5f;
                }
            } else if (F(tag, 0x1c8) == 0.0f && F(tag, 0x1c0) == 0.0f && !(F(tag, 0x1c4) > F(tag, 0x1e8)) &&
                       (F(obj, 0x25c) != 0.0f || !(F(obj, 0x250) < F(obj, 0x260)))) {
                // 0x4be31c: nothing left to do but vanish
                projectile_request_state(projectile_index, 2);
            } else if (speed < final_speed && speed > 0.0f) {
                // 0x4be364: back up to the final velocity
                real ratio = final_speed / speed * 0.99f;

                vel.i *= ratio;
                vel.j *= ratio;
                vel_k = ratio * vel.k;
            }
        }

        // 0x4be32d: gravity
        gravity = k_physics_gravity * ((((projectile_object *)obj)->base.flags & 0x10) ? ((Projectile *)tag)->water_gravity_scale : ((Projectile *)tag)->air_gravity_scale);
        vel.k = vel_k - gravity * remaining;
        step_k = step.k - gravity * remaining * 0.5f;

        // 0x4be3b2: the maximum range
        scale = 1.0f;
        if (F(tag, 0x1c8) != 0.0f && !(average_speed * remaining + F(obj, 0x250) <= F(tag, 0x1c8))) {
            if (average_speed == 0.0f) {
                scale = 0.0f;
            } else {
                scale = (F(tag, 0x1c8) - F(obj, 0x250)) / average_speed * remaining;
            }
            projectile_request_state(projectile_index, 1);
        }
        scale *= remaining;
        swept.x = step.i * scale + F(obj, 0x5c);
        swept.y = step.j * scale + F(obj, 0x60);
        swept.z = scale * step_k + F(obj, 0x64);

        // 0x4be462: collide
        {
            uint8_t hit_something = 0;

            if (collisions == 10) {
                projectile_raise_state(projectile_index, 1);
            } else if (((projectile_object *)obj)->projectile.state != 2) {
                collision_attempted = 1;
                hit_something = projectile_collision_test(projectile_index, &swept, &hit);
            }
            if (hit_something) {
                remaining = 1.0f - hit.t;
                vel.k += gravity * remaining;
                if (speed_after != 0.0f) {
                    real s = remaining * F(obj, 0x25c) + speed_after;
                    real ratio;

                    if (!(s <= speed)) {
                        s = speed;
                    }
                    ratio = s / speed_after;
                    vel.i *= ratio;
                    vel.j *= ratio;
                    vel.k *= ratio;
                }
                if (hit.plane.normal.k > 0.3f) {
                    ((projectile_object *)obj)->projectile.flags |= 4;
                }
                ((projectile_object *)obj)->projectile.ignore_object_index = k_datum_index_none;
                projectile_response(projectile_index, &hit, &swept, &vel);
                collisions++;
                ai_accumulate_repeated_event(projectile_index, &hit.point, 1, ((Projectile *)tag)->impact_noise, 1);
                if (((projectile_object *)obj)->projectile.flags & 8) {
                    goto next_step;
                }
            } else {
                remaining = 0.0f;
                if (!collision_attempted) {
                    break;
                }
            }
        }

        // 0x4be5c1: distance travelled and the fly-by sound
        {
            real_vector3d moved;    // [esp+0x44]

            moved.i = swept.x - F(obj, 0x5c);
            moved.j = swept.y - F(obj, 0x60);
            moved.k = swept.z - F(obj, 0x64);
            F(obj, 0x250) = (real)sqrt(moved.k * moved.k + moved.j * moved.j + moved.i * moved.i) + F(obj, 0x250);
            if (!flyby_played && *(datum_index *)&((Projectile *)tag)->flyby_sound.tag_id != k_datum_index_none &&
                *(datum_index *)local_player_globals->local_players != k_datum_index_none) {
                datum_index player = *(datum_index *)local_player_globals->local_players;
                datum_index listener = *(datum_index *)((uint8_t *)player_data->data + (player & 0xffff) * 0x200 + 0x34);

                if (listener != k_datum_index_none && listener != shooter) {
                    real_point3d *center = (real_point3d *)(OBJECT_DATA(listener) + 0xa0);
                    real radius = sound_definition_maximum_distance(*(datum_index *)&((Projectile *)tag)->flyby_sound.tag_id);
                    real_vector3d to_listener;  // [esp+0xa4]
                    real_vector3d projected;    // [esp+0xb0]
                    real_vector3d perpendicular; // [esp+0x98]
                    real along;

                    to_listener.i = center->x - F(obj, 0x5c);
                    to_listener.j = center->y - F(obj, 0x60);
                    to_listener.k = center->z - F(obj, 0x64);
                    vector3d_project_onto_axis(&projected, &moved, &to_listener, &perpendicular);
                    along = projected.k * moved.k + projected.j * moved.j + projected.i * moved.i;
                    if (!(along < 0.0f) && vector3d_magnitude_squared(&moved) > along &&
                        radius * radius > vector3d_magnitude_squared(&perpendicular)) {
                        sound_placement placement;  // [esp+0xc8]

                        placement.position.x = center->x - perpendicular.i;
                        placement.position.y = center->y - perpendicular.j;
                        placement.position.z = center->z - perpendicular.k;
                        *(real_vector3d *)&placement.forward = moved;
                        vector3d_normalize_with_length((real_vector3d *)&placement.forward);
                        *(real_vector3d *)&placement.velocity = *global_origin3d_pointer;
                        placement.leaf_index = *(int32_t *)&hit.leaf;
                        *(int32_t *)&placement.cluster_index = *(int32_t *)((uint8_t *)&hit.leaf + 4);
                        sound_start_at_location(*(datum_index *)&((Projectile *)tag)->flyby_sound.tag_id, &placement, 1.0f);
                        flyby_played = 1;
                    }
                }
            }
        }

        // 0x4be809: orientation
        if ((((Projectile *)tag)->projectile_flags & 1) &&
            (velocity->i != 0.0f || velocity->j != 0.0f || velocity->k != 0.0f)) {
            real_vector3d direction = *velocity;

            if (vector3d_normalize_with_length(&direction) > 0.0f) {
                real_vector3d side;

                *forward = direction;
                vector3d_cross_product(&side, forward, up);
                vector3d_cross_product(up, &side, forward);
                if (vector3d_normalize_with_length(up) == 0.0f) {
                    vector3d_build_perpendicular(up, forward);
                    vector3d_normalize_with_length(up);
                }
            }
            vector3d_rotate_about_axis(up, forward, F(obj, 0x270), F(obj, 0x274));
        } else if (((projectile_object *)obj)->projectile.flags & 1) {
            real_vector3d *axis = (real_vector3d *)(obj + 0x264);
            real_vector3d side;

            vector3d_rotate_about_axis(forward, axis, F(obj, 0x270), F(obj, 0x274));
            vector3d_rotate_about_axis(up, axis, F(obj, 0x270), F(obj, 0x274));
            vector3d_normalize_with_length(forward);
            vector3d_cross_product(&side, forward, up);
            vector3d_cross_product(up, &side, forward);
            vector3d_normalize_with_length(up);
        }

        // 0x4be99b: commit the step
        object_unlink_cluster_or_notify_parent(projectile_index);
        ((projectile_object *)obj)->base.position = swept;
        object_set_cluster_and_parent(projectile_index, &hit.leaf);
        *velocity = vel;
        if (remaining != 0.0f && collisions != 0 && ((projectile_object *)obj)->projectile.contrail_attachment_index != -1 &&
            *(datum_index *)(obj + 0x14c + ((projectile_object *)obj)->projectile.contrail_attachment_index * 4) != k_datum_index_none) {
            object_recalculate_bounding_radius(projectile_index);
            contrail_advance(*(datum_index *)(obj + 0x14c + ((projectile_object *)obj)->projectile.contrail_attachment_index * 4), 0,
                             (1.0f - remaining) * 0.033333335f);
        }
    next_step:
        if (!(remaining > 0.0f)) {
            break;
        }
    }

    // 0x4bea68: detonate / vanish
    switch (((projectile_object *)obj)->projectile.state) {
    case 1:
        if (F(obj, 0x24c) != 0.0f && F(obj, 0x248) < 1.0f) {
            return 1;
        }
        if (((projectile_object *)obj)->base.network_role == 1) {
            return 1;
        }
        if (((projectile_object *)obj)->base.network_role == 0 && obj[0x278] == 1) {
            projectile_send_detonation(projectile_index);
        }
        projectile_detonate(projectile_index, (char)(collisions == 0), remaining);
        // fall through: a detonated projectile is removed like a vanished one
    case 2: {
        int32_t role = *(int32_t *)(OBJECT_DATA(projectile_index) + 0x4);

        if (role == 0) {
            object_delete_unparented(projectile_index);
            object_delete_recursive(projectile_index, 0);
        } else if (role == 3) {
            object_delete_recursive(projectile_index, 0);
        }
        break;
    }
    default:
        break;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bdc00), the complete function (Ghidra's own "size=1456" batch
metadata is wrong -- it is the byte offset of 0x4be1b0, which this decompilation already fully
absorbs; see the file header):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004bdc00(uint param_1)

{
  float *pfVar1;
  float fVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  bool bVar12;
  bool bVar13;
  char cVar14;
  byte bVar15;
  int iVar16;
  short sVar17;
  float10 fVar18;
  float10 fVar19;
  float10 fVar20;
  float10 fVar21;
  float10 extraout_ST1;
  float10 extraout_ST1_00;
  float local_158;
  float fStack_154;
  float fStack_150;
  float fStack_14c;
  float fStack_148;
  float fStack_140;
  float fStack_13c;
  float fStack_130;
  float fStack_114;
  float fStack_110;
  float fStack_10c;
  float fStack_108;
  float fStack_104;
  float fStack_100;
  float fStack_fc;
  float fStack_f8;
  float fStack_f4;
  float fStack_f0;
  float fStack_ec;
  float fStack_e8;
  float fStack_e4;
  uint uStack_e0;
  uint uStack_dc;
  float fStack_d8;
  float fStack_d4;
  float fStack_d0;
  float fStack_cc;
  float fStack_c8;
  float fStack_c4;
  float fStack_c0;
  float fStack_bc;
  float fStack_b8;
  float fStack_b4;
  float fStack_ac;
  float fStack_a8;
  float fStack_a4;
  float fStack_a0;
  float fStack_9c;
  float fStack_98;
  float fStack_94;
  undefined4 uStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined1 auStack_58 [12];
  undefined4 uStack_4c;
  undefined4 uStack_48;
  float fStack_44;
  undefined1 auStack_40 [20];
  float fStack_2c;

  iVar16 = (param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_158 = 1.0;
  sVar17 = 0;
  bVar13 = false;
  if (((puVar3[0x8b] & 2) == 0) && (puVar3[0x8f] != 0xffffffff)) {
    if (puVar3[puVar3[0x8f] + 0x53] != 0xffffffff) {
      contrail_delete(puVar3[puVar3[0x8f] + 0x53]);
    }
    puVar3[puVar3[0x8f] + 0x53] = 0xffffffff;
    puVar3[0x8f] = 0xffffffff;
  }
  puVar3[0x92] = (uint)((float)puVar3[0x92] + (float)puVar3[0x93]);
  puVar3[0x95] = (uint)((float)puVar3[0x96] + (float)puVar3[0x95]);
  if ((*(short *)(iVar4 + 0x180) == 1) || (*(short *)(iVar4 + 0x180) == 2)) {
    bVar15 = (byte)(puVar3[0x8b] >> 4) & 1;
  }
  else {
    bVar15 = 1;
  }
  uVar5 = puVar3[0x8b];
  if ((((uVar5 & 0x20) != 0) || ((uVar5 & 8) != 0)) || (bVar15 != 0)) {
    if ((uVar5 & 0x20) == 0) {
      puVar3[0x8b] = uVar5 | 0x20;
    }
    fVar2 = (float)puVar3[0x90];
    puVar3[0x90] = (uint)((float)puVar3[0x91] + fVar2);
    if ((1.0 <= (float)puVar3[0x91] + fVar2) &&
       (iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16), *(short *)(iVar6 + 0x230) < 1)
       ) {
      *(undefined2 *)(iVar6 + 0x230) = 1;
    }
  }
  item_update_function_values(param_1);
  do {
    if ((((short)puVar3[0x8c] != 0) &&
        ((((short)puVar3[0x8c] != 1 || ((float)puVar3[0x93] == 0.0)) || (1.0 <= (float)puVar3[0x92])
         ))) || ((((puVar3[0x8b] & 8) != 0 || ((puVar3[4] & 0x20) != 0)) ||
                 (puVar3[0x47] != 0xffffffff)))) break;
    pfVar1 = (float *)(puVar3 + 0x1a);
    fStack_150 = *pfVar1;
    fStack_14c = (float)puVar3[0x1b];
    fVar2 = (float)puVar3[0x1c];
    bVar12 = false;
    fVar8 = SQRT((float)puVar3[0x1c] * (float)puVar3[0x1c] +
                 (float)puVar3[0x1b] * (float)puVar3[0x1b] + *pfVar1 * *pfVar1);
    fStack_114 = *pfVar1;
    fStack_110 = (float)puVar3[0x1b];
    fStack_10c = (float)puVar3[0x1c];
    uVar5 = puVar3[0x8d];
    if ((puVar3[0x8e] != 0xffffffff) && (0.0 < *(float *)(iVar4 + 0x1ec))) {
      fStack_154 = *(float *)(iVar4 + 0x1ec) * 0.033333335;
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar3[0x8e] & 0xffff) * 0xc);
      if (((1 << (*(byte *)(iVar6 + 0xb4) & 0x1f) & 3U) != 0) && (*(int *)(iVar6 + 0x218) != -1)) {
        fVar18 = (float10)FUN_0046fe10();
        fStack_154 = (float)(fVar18 * (float10)fStack_154);
      }
      fVar9 = (float)puVar3[0x28] - *(float *)(iVar6 + 0xa0);
      fVar10 = (float)puVar3[0x29] - *(float *)(iVar6 + 0xa4);
      fVar11 = (float)puVar3[0x2a] - *(float *)(iVar6 + 0xa8);
      fVar9 = SQRT(fVar10 * fVar10 + fVar11 * fVar11 + fVar9 * fVar9);
      if (fVar9 <= 10.0) {
        if ((fVar9 <= 2.0) || (fStack_140 = (fVar9 - 2.0) * 0.125, fStack_140 < 0.0)) {
          fStack_140 = 0.0;
        }
        else if (1.0 < fStack_140) {
          fStack_140 = 1.0;
        }
      }
      else {
        fStack_140 = 1.0;
      }
      FUN_00569280();
      iVar6 = *(int *)(DAT_006f1d6c + 0xc);
      fVar18 = (float10)periodic_function_evaluate
                                  ((double)((float)(((int)param_1 >> 0x10) * 7 + iVar6 & 0xffff) *
                                           0.011111111));
      fVar19 = (float10)periodic_function_evaluate
                                  ((double)((float)(iVar6 + ((int)param_1 >> 0x10) * 3 & 0xffff) *
                                           0.011111111));
      fVar19 = (float10)3.1415927 - fVar19 * (float10)1.5707964;
      fVar20 = (float10)fcos(fVar19);
      fVar21 = (float10)fcos((float10)(float)(fVar18 * (float10)6.2831855));
      fStack_b4 = (float)(fVar21 * fVar20);
      fVar18 = (float10)fsin((float10)(float)(fVar18 * (float10)6.2831855));
      fVar19 = (float10)fsin(fVar19);
      fStack_ac = (float)fVar19;
      fStack_fc = fStack_b4 * fStack_140 + fStack_fc;
      fStack_f8 = (float)(fVar18 * fVar20 * (float10)fStack_140 + (float10)fStack_f8);
      fStack_f4 = fStack_ac * fStack_140 + fStack_f4;
      fStack_f0 = fStack_fc - (float)puVar3[0x17];
      fStack_ec = fStack_f8 - (float)puVar3[0x18];
      fStack_e8 = fStack_f4 - (float)puVar3[0x19];
      vector3d_cross_product(pfVar1);
      if ((0.0 < fStack_f0 * *pfVar1 +
                 fStack_ec * (float)puVar3[0x1b] + fStack_e8 * (float)puVar3[0x1c]) &&
         (fVar18 = (float10)vector3d_normalize_with_length(), (float10)0.0 < fVar18)) {
        fVar18 = (float10)fcos((float10)fStack_154);
        fVar19 = (float10)fsin((float10)fStack_154);
        vector3d_rotate_about_axis((float)fVar19,(float)fVar18);
      }
    }
    fStack_13c = fVar8;
    fStack_130 = fVar8;
    if ((float)puVar3[0x95] < 1.0) {
LAB_004be329:
      fVar18 = (float10)fVar2;
    }
    else {
      if ((fVar8 <= *(float *)(iVar4 + 0x1e8)) || ((float)puVar3[0x97] == 0.0)) {
        if (((*(float *)(iVar4 + 0x1c8) != 0.0) ||
            ((*(float *)(iVar4 + 0x1c0) != 0.0 ||
             (*(float *)(iVar4 + 0x1c4) < *(float *)(iVar4 + 0x1e8) ==
              (*(float *)(iVar4 + 0x1c4) == *(float *)(iVar4 + 0x1e8)))))) ||
           (((float)puVar3[0x97] == 0.0 && ((float)puVar3[0x94] < (float)puVar3[0x98])))) {
          if ((fVar8 < *(float *)(iVar4 + 0x1e8)) && (0.0 < fVar8)) {
            fVar18 = ((float10)*(float *)(iVar4 + 0x1e8) / (float10)fVar8) * (float10)0.99;
            fStack_150 = (float)((float10)fStack_150 * fVar18);
            fStack_14c = (float)((float10)fStack_14c * fVar18);
            fVar18 = fVar18 * (float10)fVar2;
            goto LAB_004be32d;
          }
        }
        else {
          item_update_max_permutation_reached();
        }
        goto LAB_004be329;
      }
      fVar9 = local_158 * (float)puVar3[0x97];
      fStack_13c = fVar8 - fVar9;
      if (fStack_13c < *(float *)(iVar4 + 0x1e8) == (fStack_13c == *(float *)(iVar4 + 0x1e8))) {
        fVar18 = (float10)fStack_13c / (float10)fVar8;
        fStack_150 = (float)((float10)fStack_150 * fVar18);
        fStack_14c = (float)((float10)fStack_14c * fVar18);
        fVar18 = fVar18 * (float10)fVar2;
        fStack_114 = (fStack_150 + *pfVar1) * 0.5;
        fStack_110 = (fStack_14c + (float)puVar3[0x1b]) * 0.5;
        fStack_10c = (float)((fVar18 + (float10)(float)puVar3[0x1c]) * (float10)0.5);
        fStack_130 = fVar8 - fVar9 * 0.5;
      }
      else {
        fVar9 = (fVar8 - *(float *)(iVar4 + 0x1e8)) / fVar9;
        fStack_13c = *(float *)(iVar4 + 0x1e8) * 0.99;
        fVar10 = 1.0 - fVar9;
        fVar18 = (float10)fStack_13c / (float10)fVar8;
        fStack_150 = (float)((float10)fStack_150 * fVar18);
        fStack_14c = (float)((float10)fStack_14c * fVar18);
        fVar18 = fVar18 * (float10)fVar2;
        fStack_114 = fVar10 * fStack_150 + (fStack_150 + *pfVar1) * fVar9 * 0.5;
        fStack_110 = fVar10 * fStack_14c + (fStack_14c + (float)puVar3[0x1b]) * fVar9 * 0.5;
        fStack_10c = (float)((float10)fVar10 * fVar18 +
                            (fVar18 + (float10)(float)puVar3[0x1c]) * (float10)fVar9 * (float10)0.5)
        ;
        fStack_130 = fVar10 * *(float *)(iVar4 + 0x1e8) + (fStack_13c + fVar8) * fVar9 * 0.5;
      }
    }
LAB_004be32d:
    if ((puVar3[4] & 0x10) == 0) {
      fVar2 = *(float *)(iVar4 + 0x1cc);
    }
    else {
      fVar2 = *(float *)(iVar4 + 0x1d8);
    }
    fVar2 = _DAT_0069c52c * fVar2;
    fVar19 = (float10)fVar2 * (float10)local_158;
    fStack_148 = (float)(fVar18 - fVar19);
    fVar18 = (float10)fStack_10c - fVar19 * (float10)0.5;
    if ((*(float *)(iVar4 + 0x1c8) == 0.0) ||
       (fStack_130 * local_158 + (float)puVar3[0x94] <= *(float *)(iVar4 + 0x1c8))) {
      fVar19 = (float10)1.0;
    }
    else if (fStack_130 == 0.0) {
      fVar19 = (float10)item_update_max_permutation_reached();
      fVar18 = extraout_ST1_00;
    }
    else {
      fVar19 = (float10)item_update_max_permutation_reached();
      fVar18 = extraout_ST1;
    }
    fVar19 = fVar19 * (float10)local_158;
    fStack_108 = (float)((float10)fStack_114 * fVar19 + (float10)(float)puVar3[0x17]);
    fStack_104 = (float)((float10)fStack_110 * fVar19 + (float10)(float)puVar3[0x18]);
    fStack_100 = (float)(fVar19 * fVar18 + (float10)(float)puVar3[0x19]);
    if (sVar17 == 10) {
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
      if (*(short *)(iVar6 + 0x230) < 1) {
        *(undefined2 *)(iVar6 + 0x230) = 1;
      }
LAB_004be5ad:
      local_158 = 0.0;
      if (!bVar12) break;
LAB_004be5c1:
      fVar2 = fStack_108 - (float)puVar3[0x17];
      fVar8 = fStack_104 - (float)puVar3[0x18];
      fVar9 = fStack_100 - (float)puVar3[0x19];
      puVar3[0x94] = (uint)(SQRT(fVar2 * fVar2 + fVar8 * fVar8 + fVar9 * fVar9) +
                           (float)puVar3[0x94]);
      if ((((!bVar13) && (*(int *)(iVar4 + 0x210) != -1)) &&
          (*(uint *)(DAT_0087a478 + 4) != 0xffffffff)) &&
         ((uVar7 = *(uint *)((*(uint *)(DAT_0087a478 + 4) & 0xffff) * 0x200 + 0x34 +
                            *(int *)(DAT_0087a480 + 0x34)), uVar7 != 0xffffffff && (uVar7 != uVar5))
         )) {
        iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
        fVar18 = (float10)FUN_00545460();
        fStack_cc = *(float *)(iVar6 + 0xa0) - (float)puVar3[0x17];
        fStack_c8 = *(float *)(iVar6 + 0xa4) - (float)puVar3[0x18];
        fStack_c4 = *(float *)(iVar6 + 0xa8) - (float)puVar3[0x19];
        vector3d_project_onto_axis();
        fVar10 = fStack_c0 * fVar2 + fStack_bc * fVar8 + fStack_b8 * fVar9;
        if ((0.0 <= fVar10) &&
           ((fVar19 = (float10)FUN_00401000(), (float10)fVar10 < fVar19 &&
            (fVar19 = (float10)FUN_00401000(),
            fVar19 < (float10)(float)fVar18 * (float10)(float)fVar18)))) {
          fStack_a8 = *(float *)(iVar6 + 0xa0) - fStack_d8;
          fStack_a4 = *(float *)(iVar6 + 0xa4) - fStack_d4;
          fStack_a0 = *(float *)(iVar6 + 0xa8) - fStack_d0;
          fStack_9c = fVar2;
          fStack_98 = fVar8;
          fStack_94 = fVar9;
          vector3d_normalize_with_length();
          uStack_90 = *(undefined4 *)PTR_DAT_00696714;
          uStack_8c = *(undefined4 *)(PTR_DAT_00696714 + 4);
          uStack_88 = *(undefined4 *)(PTR_DAT_00696714 + 8);
          uStack_84 = uStack_4c;
          uStack_80 = uStack_48;
          FUN_00543d80();
          bVar13 = true;
        }
      }
      if (((*(byte *)(iVar4 + 0x17c) & 1) == 0) ||
         ((((float)puVar3[0x1a] == 0.0 && ((float)puVar3[0x1b] == 0.0)) &&
          ((float)puVar3[0x1c] == 0.0)))) {
        if ((puVar3[0x8b] & 1) != 0) {
          vector3d_rotate_about_axis(puVar3[0x9c]);
          vector3d_rotate_about_axis(puVar3[0x9c],puVar3[0x9d]);
          vector3d_normalize_with_length();
          vector3d_cross_product(puVar3 + 0x20);
          vector3d_cross_product();
          vector3d_normalize_with_length();
        }
      }
      else {
        fStack_e4 = (float)puVar3[0x1a];
        uStack_e0 = puVar3[0x1b];
        uStack_dc = puVar3[0x1c];
        fVar18 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 < fVar18) {
          puVar3[0x1d] = (uint)fStack_e4;
          puVar3[0x1e] = uStack_e0;
          puVar3[0x1f] = uStack_dc;
          vector3d_cross_product();
          vector3d_cross_product();
          fVar18 = (float10)vector3d_normalize_with_length();
          if ((float10)0.0 == fVar18) {
            vector3d_build_perpendicular();
            vector3d_normalize_with_length();
          }
        }
        vector3d_rotate_about_axis(puVar3[0x9c]);
      }
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
      object_unlink_cluster_or_notify_parent();
      *(float *)(iVar6 + 0x5c) = fStack_108;
      *(float *)(iVar6 + 0x60) = fStack_104;
      *(float *)(iVar6 + 100) = fStack_100;
      object_set_cluster_and_parent(param_1);
      puVar3[0x1a] = (uint)fStack_150;
      puVar3[0x1b] = (uint)fStack_14c;
      puVar3[0x1c] = (uint)fStack_148;
      if (((local_158 != 0.0) && (sVar17 != 0)) &&
         ((puVar3[0x8f] != 0xffffffff && (puVar3[puVar3[0x8f] + 0x53] != 0xffffffff)))) {
        object_recalculate_bounding_radius();
        FUN_0044ca60(0);
      }
    }
    else {
      if ((short)puVar3[0x8c] == 2) goto LAB_004be5ad;
      bVar12 = true;
      cVar14 = FUN_004c0450();
      if (cVar14 == '\0') goto LAB_004be5ad;
      local_158 = 1.0 - fStack_44;
      fStack_148 = fVar2 * local_158 + fStack_148;
      if (fStack_13c != 0.0) {
        fVar2 = local_158 * (float)puVar3[0x97] + fStack_13c;
        if (fVar8 < fVar2) {
          fVar2 = fVar8;
        }
        fVar2 = fVar2 / fStack_13c;
        fStack_150 = fStack_150 * fVar2;
        fStack_14c = fStack_14c * fVar2;
        fStack_148 = fVar2 * fStack_148;
      }
      if (0.3 < fStack_2c) {
        puVar3[0x8b] = puVar3[0x8b] | 4;
      }
      puVar3[0x8d] = 0xffffffff;
      FUN_004bf390(param_1,auStack_58,&fStack_108);
      sVar17 = sVar17 + 1;
      FUN_0042c610(param_1,auStack_40,1,*(undefined2 *)(iVar4 + 0x182));
      if ((puVar3[0x8b] & 8) == 0) goto LAB_004be5c1;
    }
  } while (0.0 < local_158);
  if ((short)puVar3[0x8c] == 1) {
    if (((float)puVar3[0x93] != 0.0) && ((float)puVar3[0x92] < 1.0)) {
      return 1;
    }
    if (puVar3[1] == 1) {
      return 1;
    }
    if ((puVar3[1] == 0) && ((char)puVar3[0x9e] == '\x01')) {
      FUN_004bda60();
    }
    item_detonate(sVar17 == 0,local_158);
  }
  else if ((short)puVar3[0x8c] != 2) {
    return 1;
  }
  iVar4 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16) + 4);
  if (iVar4 == 0) {
    object_delete_unparented();
  }
  else if (iVar4 != 3) {
    return 1;
  }
  object_delete_recursive(param_1,0);
  return 1;
}
#endif
