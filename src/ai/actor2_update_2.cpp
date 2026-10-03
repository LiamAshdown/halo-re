#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

namespace halo::ai {

namespace actor_update_firing_state_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern player_globals *local_player_globals;
extern uint8_t actor_grenade_behavior_kind_allowed(datum_index actor_index, int16_t kind);
extern uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind);
extern void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3]);
extern uint8_t *actor_get_actor_definition(datum_index actor_index);
extern void actor_update_aim_wander(datum_index actor_index);
extern void actor_reseed_movement_pause_timer(datum_index actor_index);
extern uint8_t actor_should_hold_position(datum_index actor_index, uint8_t *definition);
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b);
extern uint8_t actor_action_has_queued_secondary(datum_index actor_index);
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index);
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
extern void actor_set_override_target(datum_index actor_index, uint8_t enable, datum_index override_target);
extern uint8_t actor_grenade_trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index,
    datum_index exclude_object_index, real_point3d *landing_position, int32_t *out_blocking_prop);
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern int32_t fistp_round(float x);
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
#define F(p, o) (*(float *)((p) + (o)))
#define W(p, o) (*(int16_t *)((p) + (o)))
#define D(p, o) (*(datum_index *)((p) + (o)))
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
}
}

/**
 * Actor AI behaviour: update firing state.
 *
 * @address 0x40e7b0
 */
void ActorView::update_firing_state()
{
    using namespace actor_update_firing_state_local;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = TAG_DATA(D(a, 0x58));
    uint8_t *variant = TAG_DATA(D(a, 0x5c));
    uint8_t *def = actor_get_actor_definition(actor_index);
    uint8_t *weapon_tag = 0;
    datum_index weapon;
    uint8_t fire_primary = 0;
    uint8_t fire_secondary = 0;
    uint8_t used_straight_line = 0;
    uint8_t wants_fire = 0;
    int16_t state;
    uint8_t enable = 0;
    float value = 0.0f;
    uint8_t secondary_flag = 0;

    weapon = actor_get_threat_weapon_object_index(actor_index);
    if (weapon != k_datum_index_none) {
        weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
    }
    weapon = actor_get_threat_weapon_object_index(actor_index);

    if (W(a, 0x5f4) > 0) W(a, 0x5f4) -= 1;
    if (W(a, 0x5f6) > 0) W(a, 0x5f6) -= 1;
    if (W(a, 0x5f8) > 0) W(a, 0x5f8) -= 1;
    if (W(a, 0x5fc) > 0) W(a, 0x5fc) -= 1;
    if (W(a, 0x60c) > 0) ((struct actor *)a)->firing_target_ticks += 1;

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
            changed = (uint8_t)(halo::math::vector3d_distance_squared(*(real_point3d *)(a + 0x610), *(real_point3d *)(a + 0x460)) > 0.25f);
        } else {
            changed = 0;
        }
        if (changed) {
            ((struct actor *)a)->firing_target_ticks = 0;
        }
        W(a, 0x60c) = kind;
        if (kind == 1) {
            D(a, 0x610) = D(a, 0x270);
        } else if (kind == 2) {
            *(real_point3d *)&((struct actor *)a)->firing_target_prop_index = *(real_point3d *)(a + 0x460);
        }
    }
    a[0x628] = 0;
    F(a, 0x608) = actor_has_unshielded_threat_weapon(actor_index) ? F(def, 0x74) : 0.0f;

    if (a[0x45c]) {
        int16_t grenade = W(variant, 0x180);

        if (grenade != -1 && *(int8_t *)(OBJECT_DATA(D(a, 0x18)) + 0x31e + grenade) == 0) {
            halo::units::unit_set_grenade_type_and_count_delta(D(a, 0x18), grenade, 1);
        }
        ((actor *)a)->control_flags |= 0x2000;
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
            float delay = halo::math::random_real_range(0.0f, 1.5f) + F(def, 0x15c);
            float roll = halo::math::random_real();

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

    if (W(a, 0x60c) > 0) {
        if (W(a, 0x60c) == 1) {
            uint8_t *p = PROP(D(a, 0x610));

            F(a, 0x638) = F(p, 0x11c);
            ((actor *)a)->firing_target_point = *(real_point3d *)(p + 0xc8);
            W(a, 0x626) = W(p, 0x38);
            a[0x621] = p[0x118];
            a[0x624] = 1;
            if (W(p, 0x100) != -1) {
                int32_t bit = W(p, 0x100);

                a[0x624] = (uint8_t)!(*(uint32_t *)&local_player_globals->cluster_pvs[(bit >> 5)] & (1u << (bit & 0x1f)));
            }
        } else {
            ((actor *)a)->firing_target_point = *(real_point3d *)&((struct actor *)a)->firing_target_prop_index;
            F(a, 0x638) = halo::math::vector3d_distance(*(real_point3d *)(a + 0x610), *(real_point3d *)(a + 0x120));
            a[0x621] = 0;
            a[0x624] = 0;
            if (((struct actor *)a)->firing_target_ticks % 10 == 0) {
                W(a, 0x626) = (int16_t)actor_evaluate_engagement_reachability(W(a, 0x148), -1,
                    (real_point3d *)(a + 0x62c), (real_point3d *)(a + 0x120), 0, 0, k_datum_index_none,
                    (uint8_t)(D(a, 0x158) != k_datum_index_none));
            }
        }
        a[0x622] = (uint8_t)(F(def, 0x148) > 0.0f && F(a, 0x638) > F(def, 0x148));
        a[0x623] = (uint8_t)(a[0x455] && F(def, 0x14c) > 0.0f);
        if (!halo::items::weapon_trigger_get_aiming_vector(weapon, 0, (real_point3d *)(a + 0x120), (real_point3d *)(a + 0x62c),
                                              a[0x622], (real_vector3d *)(a + 0x63c), 0, (real *)(a + 0x648),
                                              &used_straight_line)) {
            W(a, 0x60c) = 0;
        }
    }
    if (W(a, 0x60c) == 0 || a[0x624]) {
        goto idle;
    }

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
            float tolerance = F(a, 0x638) >= 1.5f ? 0.97f : F(a, 0x638) * 0.17526217f + 0.70710677f;
            real_vector3d aim;

            actor_get_aim_from_position(actor_index, (uint32_t *)&aim);
            if (!(aim.j * F(a, 0x640) + aim.k * F(a, 0x644) + aim.i * F(a, 0x63c) >= tolerance)) {
                fire_primary = 1;
            }
        }
        goto dispatch;
    }

idle:
    wants_fire = 0;
    W(a, 0x5f2) = 0;

dispatch:
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

    fire_primary = 0;
    fire_secondary = 0;
    a[0x688] = 0;
    state = W(a, 0x5f2);
    if (state == 4) {
        fire_primary = 1;
    } else if (state == 2) {
        datum_index exclude = k_datum_index_none;
        real_point3d *aim_point = (real_point3d *)(a + 0x658);
        real_point3d *final_point = (real_point3d *)(a + 0x67c);
        real_point3d origin;
        int32_t blocking_prop = -1;

        *aim_point = ((actor *)a)->aim_target_point;
        if (W(a, 0x60c) == 1) {
            uint8_t *p = PROP(D(a, 0x610));
            float f;

            exclude = D(p, 0x114);
            f = weapon_get_zoom_fov_resolved(0xf, W(a, 0x3e)) + F(def, 0xbc);
            if (!(f <= 0.0f && f < 1.0f) && !a[0x623]) {
                real_vector3d delta;

                delta.i = F(p, 0xc8) - F(a, 0x64c);
                delta.j = F(p, 0xcc) - F(a, 0x650);
                delta.k = F(p, 0xd0) - F(a, 0x654);
                halo::math::point3d_add_scaled(*aim_point, delta, *aim_point, F(def, 0xbc));
            }
            f = weapon_get_zoom_fov_resolved(0x10, W(a, 0x3e)) + F(def, 0xc0);
            if (!(f <= 0.0f && f < 1.0f)) {
                float lead = F(def, 0xc0);
                real t = halo::items::weapon_trigger_projectile_time_fraction(weapon, (int16_t)(a[0x603] != 0), F(a, 0x648));

                aim_point->x += t * F(p, 0xd4) * lead;
                aim_point->y += t * F(p, 0xd8) * lead;
                aim_point->z += t * F(p, 0xdc) * lead;
            }
        }
        F(a, 0x664) = F(a, 0x670) + F(a, 0x664);
        F(a, 0x668) = F(a, 0x674) + F(a, 0x668);
        F(a, 0x66c) = F(a, 0x678) + F(a, 0x66c);
        final_point->x = F(a, 0x664) + aim_point->x;
        final_point->y = aim_point->y + F(a, 0x668);
        final_point->z = aim_point->z + F(a, 0x66c);

        if (D(a, 0x158) != k_datum_index_none) {
            halo::units::unit_get_camera_position(D(a, 0x18), &origin);
        } else {
            uint8_t *offset = 0;

            if (a[0x508]) {
                real_vector3d *v = (real_vector3d *)(def + 0xb0);

                if (v->i * v->i + v->j * v->j + v->k * v->k > 9.999999747378752e-05f) {
                    offset = def + 0xb0;
                } else if (halo::math::vector3d_magnitude_squared(*(real_vector3d *)(actor_tag + 0x40)) > 9.999999747378752e-05f) {
                    offset = actor_tag + 0x40;
                }
            } else {
                real_vector3d *v = (real_vector3d *)(def + 0xa4);

                if (v->i * v->i + v->j * v->j + v->k * v->k > 9.999999747378752e-05f) {
                    offset = def + 0xa4;
                } else if (halo::math::vector3d_magnitude_squared(*(real_vector3d *)(actor_tag + 0x34)) > 9.999999747378752e-05f) {
                    offset = actor_tag + 0x34;
                }
            }
            if (offset == 0) {
                origin = *(real_point3d *)&((actor *)a)->aim_origin.x;
            } else {
                real_vector3d facing;

                facing.i = F(a, 0x12c) - final_point->x;
                facing.j = F(a, 0x130) - final_point->y;
                facing.k = F(a, 0x134) - final_point->z;
                if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&facing)) > 0.0f) {
                    facing.k = 0.0f;
                } else {
                    facing = *(real_vector3d *)&((actor *)a)->facing.i;
                }
                halo::units::unit_add_marker_relative_offset(D(a, 0x18), 3, (float *)(a + 0x12c), (uint32_t)&facing,
                                                (uint32_t)offset, &origin);
            }
        }

        halo::items::weapon_trigger_get_aiming_vector(weapon, (int16_t)(a[0x603] != 0), &origin, final_point, a[0x622],
                                         (real_vector3d *)(a + 0x68c), 0, 0, &used_straight_line);
        a[0x688] = (uint8_t)(used_straight_line == 0);
        {
            real_vector3d path;

            path.i = final_point->x - origin.x;
            path.j = final_point->y - origin.y;
            path.k = final_point->z - origin.z;
            if (actor_grenade_trajectory_blocked(&path, actor_index, exclude, &origin, &blocking_prop)) {
                W(a, 0x5fa) = 0;
                if (a[0x603]) {
                    fire_secondary = 1;
                } else {
                    fire_primary = 1;
                }
            } else {
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

    if (fire_primary) {
        float burst = F(def, 0x78);

        if (a[0x602]) {
            a[0x602] = 0;
        } else if (burst == 0.0f) {
            enable = 1;
            value = 1.0f;
        } else if (W(a, 0x5f8) == 0) {
            uint8_t *stance_a;
            uint8_t *stance_b = 0;
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
        ((actor *)a)->control_flags |= 0x1000;
    } else {
        ((actor *)a)->control_flags &= ~0x1000u;
    }
}

#undef F
#undef W
#undef D
#undef PROP
#undef OBJECT_DATA
#undef TAG_DATA

}
