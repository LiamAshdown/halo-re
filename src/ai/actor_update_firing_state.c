// actor_update_firing_state  (Ghidra: actor_update_firing_state, renamed)
// address 0x40e7b0, size 3752 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x40e7b0..0x40f657. The draft wrote the vehicle firing origin through a garbage pointer
//   (argless unit_get_camera_position), called the grenade top-up, lead, drift, line-of-fire and trigger helpers
//   without their operands, and resolved the aim point from the wrong fields. Per tick: age the countdowns, pick the
//   target (+0x270 prop, or the +0x460 point) and track it, maybe decide on a grenade (definition +0x154..+0x15c),
//   find the aiming vector (0x4c2b40), gate firing (forced +0x457, stance, minimum range, visibility, range +0x608),
//   run the burst state machine (+0x5f2: 0 idle, 1 hold, 2 aim, 3 pause, 4 special), and while aiming build the aim
//   point (drift +0xbc and lead +0xc0 toward the prop, plus the accumulated error +0x664), the firing origin (camera
//   in a vehicle, else a marker offset), check friends in the line of fire (0x42b190; after 45 blocked ticks shout
//   0xe at them) and pull the primary trigger (burst timer +0x5f8) or flag the secondary (actor +0x6d0 bit 0x1000)
//   through 0x42a5e0.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include "units.h"
#include "game.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern player_globals *local_player_globals; // 0x0087a478

extern real random_real_range(real min, real max);          // 0x401050
extern real random_real(void);                                // 0x4019f0
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale); // 0x401930, EAX, ECX, stack
extern real vector3d_distance(real_point3d *a, real_point3d *b); // 0x4088b0, EAX, ECX
extern real vector3d_magnitude_squared(real_vector3d *v);        // 0x401000, EAX
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern uint8_t actor_grenade_behavior_kind_allowed(datum_index actor_index, int16_t kind); // 0x40f670, EAX, stack
extern uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind); // 0x40f700, EAX, stack
extern void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3]); // 0x40f9b0, EAX, ECX
extern uint8_t *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX
extern void actor_update_aim_wander(datum_index actor_index);        // 0x40fcb0
extern void actor_reseed_movement_pause_timer(datum_index actor_index); // 0x4104e0
extern uint8_t actor_should_hold_position(datum_index actor_index, uint8_t *definition);  // 0x4105c0, EAX, EDX
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b); // 0x4106b0
extern uint8_t actor_action_has_queued_secondary(datum_index actor_index); // 0x417b70, EAX
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index); // 0x4282c0, EAX
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370, EAX
extern void actor_set_override_target(datum_index actor_index, uint8_t enable, datum_index override_target); // 0x42a5e0, EAX, stack
extern uint8_t actor_grenade_trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index,
    datum_index exclude_object_index, real_point3d *landing_position, int32_t *out_blocking_prop); // 0x42b190, EAX, ECX, stack
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying); // 0x42b270, AX, CX, ESI, EDI, stack
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340
extern int32_t fistp_round(float x); // harness/x87_shims.c: FISTP in the current (round-to-nearest) mode
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX, AX
extern uint8_t weapon_trigger_get_aiming_vector(datum_index weapon_index, int16_t trigger_index, real_point3d *origin,
    real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction, real *out_time, real *out_range,
    uint8_t *out_used_straight_line); // 0x4c2b40, EAX, CX, stack
extern real weapon_trigger_projectile_time_fraction(datum_index item_index, int16_t trigger_index, real elapsed); // 0x4c2be0, EAX, CX, stack
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, ECX, EDI
extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point, uint32_t reference_direction,
    uint32_t offsets, real_point3d *accumulator); // 0x569190, stack, EAX
extern int32_t unit_set_grenade_type_and_count_delta(uint32_t unit_index, int16_t grenade_type, int8_t delta); // 0x56d160, EAX, DX, stack

#define F(p, o) (*(float *)((p) + (o)))
#define W(p, o) (*(int16_t *)((p) + (o)))
#define D(p, o) (*(datum_index *)((p) + (o)))
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// blam-cc: stack -> actor_index
void actor_update_firing_state(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;   // ebp
    uint8_t *actor_tag = TAG_DATA(D(a, 0x58));      // [esp+0x18]
    uint8_t *variant = TAG_DATA(D(a, 0x5c));        // [esp+0x20]
    uint8_t *def = actor_get_actor_definition(actor_index); // [esp+0x14]
    uint8_t *weapon_tag = 0;                        // [esp+0x24]
    datum_index weapon;                             // [esp+0x1c]
    uint8_t fire_primary = 0;                       // [esp+0x11]
    uint8_t fire_secondary = 0;                     // [esp+0x12]
    uint8_t used_straight_line = 0;                 // [esp+0x13]
    uint8_t wants_fire = 0;                         // bl
    int16_t state;
    uint8_t enable = 0;                             // [esp+0x18]
    float value = 0.0f;                             // [esp+0x1c]
    uint8_t secondary_flag = 0;                     // [esp+0x13]

    weapon = actor_get_threat_weapon_object_index(actor_index);
    if (weapon != k_datum_index_none) {
        weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
    }
    weapon = actor_get_threat_weapon_object_index(actor_index);

    if (W(a, 0x5f4) > 0) W(a, 0x5f4) -= 1;
    if (W(a, 0x5f6) > 0) W(a, 0x5f6) -= 1;
    if (W(a, 0x5f8) > 0) W(a, 0x5f8) -= 1;
    if (W(a, 0x5fc) > 0) W(a, 0x5fc) -= 1;
    if (W(a, 0x60c) > 0) *(int32_t *)(a + 0x61c) += 1;

    // 0x40e8c5: what are we shooting at (1 a prop, 2 a point)
    if (W(a, 0x5f2) != 2) {
        int16_t kind = 0;
        uint8_t changed;

        if (a[0x454]) {
            if (a[0x45d]) {
                kind = 2;
            } else if (D(a, 0x270) != k_datum_index_none) {
                kind = 1;
            }
        }
        if (kind != W(a, 0x60c)) {
            changed = 1;
        } else if (kind == 1) {
            changed = (uint8_t)(D(a, 0x610) != D(a, 0x270));
        } else if (kind == 2) {
            changed = (uint8_t)(vector3d_distance_squared((real_point3d *)(a + 0x610), (real_point3d *)(a + 0x460)) > 0.25f);
        } else {
            changed = 0;
        }
        if (changed) {
            *(int32_t *)(a + 0x61c) = 0;
        }
        W(a, 0x60c) = kind;
        if (kind == 1) {
            D(a, 0x610) = D(a, 0x270);
        } else if (kind == 2) {
            *(real_point3d *)(a + 0x610) = *(real_point3d *)(a + 0x460);
        }
    }
    a[0x628] = 0;
    F(a, 0x608) = actor_has_unshielded_threat_weapon(actor_index) ? F(def, 0x74) : 0.0f;

    if (a[0x45c]) {
        // 0x40e9c2: a scripted grenade request: top up an empty grenade slot and shout
        int16_t grenade = W(variant, 0x180);

        if (grenade != -1 && *(int8_t *)(OBJECT_DATA(D(a, 0x18)) + 0x31e + grenade) == 0) {
            unit_set_grenade_type_and_count_delta(D(a, 0x18), grenade, 1);
        }
        ((actor *)a)->flags |= 0x2000;
        ai_communication_broadcast(9, D(a, 0x18), k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
        goto idle;
    }
    if (!actor_has_unshielded_threat_weapon(actor_index)) {
        goto idle;
    }
    if (W(a, 0x5f2) == 4) {
        wants_fire = 1;
        goto dispatch;
    }

    // 0x40ea83: throw a grenade instead?
    if (W(def, 0x154) > 0 && W(a, 0x5f2) != 2 && !(W(a, 0x5fc) > 0) && !(W(a, 0x5fe) > 0)) {
        uint8_t *threat_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
        uint8_t allowed;

        weapon_get_zoom_fov_resolved(0x12, W(a, 0x3e));
        if (W(def, 0x154) == 1) {
            weapon_get_zoom_fov_resolved(0x11, W(a, 0x3e));
            allowed = (uint8_t)(*(int32_t *)(threat_tag + 0x4fc) > 0);
        } else if (W(def, 0x154) == 2) {
            allowed = (uint8_t)(*(int32_t *)(threat_tag + 0x4fc) > 1);
        } else {
            allowed = 1;
        }
        if (allowed && actor_grenade_behavior_kind_allowed(actor_index, (int16_t)*(uint16_t *)(def + 0x156))) {
            float delay = random_real_range(0.0f, 1.5f) + F(def, 0x15c);
            float roll = random_real();

            W(a, 0x5fc) = (int16_t)(int32_t)(delay * 30.0f);
            if (roll < F(def, 0x158) &&
                actor_target_is_visible_or_object_count_ok(actor_index, (int16_t)*(uint16_t *)(def + 0x156))) {
                if (W(def, 0x156) == 3) {
                    W(a, 0x5fe) = 3;
                }
                if (W(def, 0x154) == 1) {
                    a[0x602] = 1;
                } else if (W(def, 0x154) == 2) {
                    a[0x604] = 1;
                }
            }
        }
    }

    // 0x40ebdd: track the target and find the aiming vector
    if (W(a, 0x60c) > 0) {
        if (W(a, 0x60c) == 1) {
            uint8_t *p = PROP(D(a, 0x610));

            F(a, 0x638) = F(p, 0x11c);
            *(real_point3d *)&((actor *)a)->wander_unknown_62c = *(real_point3d *)(p + 0xc8);
            W(a, 0x626) = W(p, 0x38);
            a[0x621] = p[0x118];
            a[0x624] = 1;
            if (W(p, 0x100) != -1) {
                int32_t bit = W(p, 0x100);

                a[0x624] = (uint8_t)!(*(uint32_t *)&local_player_globals->cluster_pvs[(bit >> 5)] & (1u << (bit & 0x1f)));
            }
        } else {
            *(real_point3d *)&((actor *)a)->wander_unknown_62c = *(real_point3d *)(a + 0x610);
            F(a, 0x638) = vector3d_distance((real_point3d *)(a + 0x610), (real_point3d *)(a + 0x120));
            a[0x621] = 0;
            a[0x624] = 0;
            if (*(int32_t *)(a + 0x61c) % 10 == 0) {
                W(a, 0x626) = (int16_t)actor_evaluate_engagement_reachability(W(a, 0x148), -1,
                    (real_point3d *)(a + 0x62c), (real_point3d *)(a + 0x120), 0, 0, k_datum_index_none,
                    (uint8_t)(D(a, 0x158) != k_datum_index_none));
            }
        }
        a[0x622] = (uint8_t)(F(def, 0x148) > 0.0f && F(a, 0x638) > F(def, 0x148));
        a[0x623] = (uint8_t)(a[0x455] && F(def, 0x14c) > 0.0f);
        if (!weapon_trigger_get_aiming_vector(weapon, 0, (real_point3d *)(a + 0x120), (real_point3d *)(a + 0x62c),
                                              a[0x622], (real_vector3d *)(a + 0x63c), 0, (real *)(a + 0x648),
                                              &used_straight_line)) {
            W(a, 0x60c) = 0;
        }
    }
    if (W(a, 0x60c) == 0 || a[0x624]) {
        goto idle;
    }

    // 0x40edd8: may we fire at all
    {
        uint8_t forced = a[0x457];

        if (!forced && W(a, 0x5f6) > 0) goto idle;
        if (actor_action_has_queued_secondary(actor_index)) goto idle;
        if (!forced) {
            if (a[0x15c] && !a[0x99] && !(variant[0] & 1)) goto idle;
            if ((*(uint32_t *)actor_tag & 0x200) && !a[0x508]) goto idle;
            if ((actor_tag[4] & 2) && a[0x508]) goto idle;
            if ((actor_tag[4] & 4) && a[0x504]) goto idle;
        }
        if (a[0x621] || a[0x15d]) goto idle;
        if (weapon_tag != 0 && F(weapon_tag, 0x40c) > 0.0f && F(a, 0x638) < F(weapon_tag, 0x40c)) goto idle;
        if (W(a, 0x3e8) == 0 || W(a, 0x3ec) != 2 || a[0x58c]) goto idle;
        if (W(a, 0x5f2) == 2) {
            wants_fire = 1;
            goto dispatch;
        }
        a[0x620] = (uint8_t)(W(a, 0x626) == 0 || W(a, 0x626) == 1);
        if (!a[0x620] && !a[0x623]) goto idle;
        if (!forced && !(F(a, 0x638) < F(a, 0x608))) goto idle;
        wants_fire = 1;
        a[0x628] = 1;
        if (!(*(uint32_t *)actor_tag & 0x2000)) {
            // 0x40ef6d: on target when the aim direction is within the tolerance (wider up close)
            float tolerance = F(a, 0x638) >= 1.5f ? 0.97f : F(a, 0x638) * 0.17526217f + 0.70710677f;
            real_vector3d aim;

            actor_get_aim_from_position(actor_index, (uint32_t *)&aim);
            if (!(aim.j * F(a, 0x640) + aim.k * F(a, 0x644) + aim.i * F(a, 0x63c) >= tolerance)) {
                fire_primary = 1; // off target
            }
        }
        goto dispatch;
    }

idle:
    wants_fire = 0;
    W(a, 0x5f2) = 0;

dispatch:
    // 0x40ea4e: the burst state machine (0 idle, 1 hold, 2 aim, 3 pause, 4 special)
    {
        int16_t next = -1;

        switch (W(a, 0x5f2)) {
        case 0:
            if (wants_fire) next = 1;
            break;
        case 1:
        case 3:
            if (!fire_primary && W(a, 0x5f4) == 0) next = 2;
            break;
        case 2:
            if (W(a, 0x5f4) == 0) next = 3;
            break;
        case 4:
            if (W(a, 0x5f4) == 0) next = 0;
            break;
        default:
            break;
        }
        if (next != -1) {
            if (next == 1) {
                if (!actor_should_hold_position(actor_index, def) && !fire_primary) {
                    next = 2;
                    actor_update_aim_wander(actor_index);
                }
            } else if (next == 2) {
                actor_update_aim_wander(actor_index);
            } else if (next == 3) {
                actor_reseed_movement_pause_timer(actor_index);
            }
            W(a, 0x5f2) = next;
        }
    }

    // 0x40f067
    fire_primary = 0;
    fire_secondary = 0;
    a[0x688] = 0;
    state = W(a, 0x5f2);
    if (state == 4) {
        fire_primary = 1;
    } else if (state == 2) {
        // 0x40f099: build this tick's aim point
        datum_index exclude = k_datum_index_none;   // [esp+0x24]
        real_point3d *aim_point = (real_point3d *)(a + 0x658);
        real_point3d *final_point = (real_point3d *)(a + 0x67c);
        real_point3d origin;                        // [esp+0x30]
        int32_t blocking_prop = -1;                 // [esp+0x28]

        *aim_point = *(real_point3d *)&((actor *)a)->wander_unknown_64c.i;
        if (W(a, 0x60c) == 1) {
            uint8_t *p = PROP(D(a, 0x610));
            float f;

            exclude = D(p, 0x114);
            f = weapon_get_zoom_fov_resolved(0xf, W(a, 0x3e)) + F(def, 0xbc);
            if (!(f <= 0.0f && f < 1.0f) && !a[0x623]) {
                // drift toward the target's real position
                real_vector3d delta;

                delta.i = F(p, 0xc8) - F(a, 0x64c);
                delta.j = F(p, 0xcc) - F(a, 0x650);
                delta.k = F(p, 0xd0) - F(a, 0x654);
                point3d_add_scaled(aim_point, &delta, aim_point, F(def, 0xbc));
            }
            f = weapon_get_zoom_fov_resolved(0x10, W(a, 0x3e)) + F(def, 0xc0);
            if (!(f <= 0.0f && f < 1.0f)) {
                // lead the target by the projectile's flight time
                float lead = F(def, 0xc0);
                real t = weapon_trigger_projectile_time_fraction(weapon, (int16_t)(a[0x603] != 0), F(a, 0x648));

                aim_point->x += t * F(p, 0xd4) * lead;
                aim_point->y += t * F(p, 0xd8) * lead;
                aim_point->z += t * F(p, 0xdc) * lead;
            }
        }
        // 0x40f22e: add the accumulated aiming error
        F(a, 0x664) = F(a, 0x670) + F(a, 0x664);
        F(a, 0x668) = F(a, 0x674) + F(a, 0x668);
        F(a, 0x66c) = F(a, 0x678) + F(a, 0x66c);
        final_point->x = F(a, 0x664) + aim_point->x;
        final_point->y = aim_point->y + F(a, 0x668);
        final_point->z = aim_point->z + F(a, 0x66c);

        // 0x40f28c: where the shot leaves from
        if (D(a, 0x158) != k_datum_index_none) {
            unit_get_camera_position(D(a, 0x18), &origin);
        } else {
            uint8_t *offset = 0;

            if (a[0x508]) {
                real_vector3d *v = (real_vector3d *)(def + 0xb0);

                if (v->i * v->i + v->j * v->j + v->k * v->k > 9.999999747378752e-05f) {
                    offset = def + 0xb0;
                } else if (vector3d_magnitude_squared((real_vector3d *)(actor_tag + 0x40)) > 9.999999747378752e-05f) {
                    offset = actor_tag + 0x40;
                }
            } else {
                real_vector3d *v = (real_vector3d *)(def + 0xa4);

                if (v->i * v->i + v->j * v->j + v->k * v->k > 9.999999747378752e-05f) {
                    offset = def + 0xa4;
                } else if (vector3d_magnitude_squared((real_vector3d *)(actor_tag + 0x34)) > 9.999999747378752e-05f) {
                    offset = actor_tag + 0x34;
                }
            }
            if (offset == 0) {
                origin = *(real_point3d *)&((actor *)a)->aim_origin.x;
            } else {
                real_vector3d facing;       // [esp+0x3c]

                facing.i = F(a, 0x12c) - final_point->x;
                facing.j = F(a, 0x130) - final_point->y;
                facing.k = F(a, 0x134) - final_point->z;
                if (vector2d_normalize_with_length((real_vector2d *)&facing) > 0.0f) {
                    facing.k = 0.0f;
                } else {
                    facing = *(real_vector3d *)&((actor *)a)->facing.i;
                }
                unit_add_marker_relative_offset(D(a, 0x18), 3, (float *)(a + 0x12c), (uint32_t)&facing,
                                                (uint32_t)offset, &origin);
            }
        }

        // 0x40f3db: the firing direction, and whether a friend is in the way
        weapon_trigger_get_aiming_vector(weapon, (int16_t)(a[0x603] != 0), &origin, final_point, a[0x622],
                                         (real_vector3d *)(a + 0x68c), 0, 0, &used_straight_line);
        a[0x688] = (uint8_t)(used_straight_line == 0);
        {
            real_vector3d path;             // [esp+0x3c]

            path.i = final_point->x - origin.x;
            path.j = final_point->y - origin.y;
            path.k = final_point->z - origin.z;
            if (actor_grenade_trajectory_blocked(&path, actor_index, exclude, &origin, &blocking_prop)) {
                // clear: fire
                W(a, 0x5fa) = 0;
                if (a[0x603]) {
                    fire_secondary = 1;
                } else {
                    fire_primary = 1;
                }
            } else {
                // a friend is in the way: hold, and after 1.5 seconds tell them to move
                W(a, 0x5fa) += 1;
                W(a, 0x5f4) += 1;
                if (W(a, 0x5fa) >= 0x2d && W(a, 0x268) >= 7) {
                    datum_index in_the_way = blocking_prop != -1 ? D(PROP(blocking_prop), 0x18) : k_datum_index_none;

                    ai_communication_broadcast(0xe, D(a, 0x18), in_the_way, 2, k_datum_index_none, k_datum_index_none, 0);
                    W(a, 0x5fa) = 0;
                }
            }
        }
    }

    // 0x40f4e4: pull the trigger
    if (fire_primary) {
        float burst = F(def, 0x78);                  // [esp+0x24]

        if (a[0x602]) {
            a[0x602] = 0;
        } else if (burst == 0.0f) {
            enable = 1;
            value = 1.0f;
        } else if (W(a, 0x5f8) == 0) {
            uint8_t *stance_a;
            uint8_t *stance_b = 0;                  // [esp+0x28]
            float rate;
            int16_t ticks;

            enable = 1;
            value = 1.0f;
            rate = weapon_get_zoom_fov_resolved(0xa, W(a, 0x3e)) * burst;
            actor_select_stance_offset_pair(actor_index, def, &stance_a, &stance_b);
            if (stance_b != 0 && F(stance_b, 0x8) > 0.0f) {
                rate *= F(stance_b, 0x8);
            }
            ticks = (int16_t)fistp_round(30.0f / rate);
            W(a, 0x5f8) = ticks < 2 ? 2 : ticks;
        }
    } else if (fire_secondary) {
        if (a[0x602]) {
            a[0x602] = 0;
        } else {
            secondary_flag = 1;
        }
    } else if (a[0x602]) {
        enable = 1;
        value = 1.0f;
    }
    actor_set_override_target(actor_index, enable, *(datum_index *)&value);
    if (secondary_flag) {
        ((actor *)a)->flags |= 0x1000;
    } else {
        ((actor *)a)->flags &= ~0x1000u;
    }
}

#if 0
Original Ghidra decompilation (0x40e7b0): run `python tools/pack.py 0x40e7b0`.
The listing is 300+ lines with the frame spelled out as aliased locals (local_30 is first the
Actor tag pointer and later a byte pair, local_24 is first the weapon tag pointer and later a
cosine, local_18 / local_14 / local_10 are read before assignment on one branch). It is kept
in out/phase2/ai rather than duplicated here; the aliasing is described in the UNSURE block
at the top of this file.
#endif
