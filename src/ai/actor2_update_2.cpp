#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/flags.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"

namespace halo::ai {

namespace actor_update_firing_state_local {
extern "C" {
extern player_globals *local_player_globals;
extern int32_t fistp_round(float x);
extern float halo::game::weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))
#define W(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))
#define D(p, o) (*(datum_index *)((uint8_t *)(p) + (o)))
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & halo::k_slot_mask].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
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
    actor *a = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = TAG_DATA(a->actor_definition_tag);
    uint8_t *variant = TAG_DATA(a->actor_variant_tag);
    uint8_t *def = (uint8_t *)halo::ai::actor_get_actor_definition(actor_index);
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

    weapon = halo::ai::actor_get_threat_weapon_object_index(actor_index);
    if (weapon != k_datum_index_none) {
        weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
    }
    weapon = halo::ai::actor_get_threat_weapon_object_index(actor_index);

    if (a->firing_state_timer > 0) a->firing_state_timer -= 1;
    if (a->firing_delay_timer > 0) a->firing_delay_timer -= 1;
    if (a->refire_timer > 0) a->refire_timer -= 1;
    if (a->special_fire_timer > 0) a->special_fire_timer -= 1;
    if (a->firing_target_type > 0) a->firing_target_ticks += 1;

    if (a->firing_state != 2) {
        int16_t kind = 0;
        uint8_t changed;

        if (a->wants_to_fire) {
            if (a->unknown_45d[0]) {
                kind = 2;
            } else if (a->target_unit_index != k_datum_index_none) {
                kind = 1;
            }
        }
        if (kind != a->firing_target_type) {
            changed = 1;
        } else if (kind == 1) {
            changed = (uint8_t)(a->firing_target_prop_index != a->target_unit_index);
        } else if (kind == 2) {
            changed = (uint8_t)(halo::math::vector3d_distance_squared(*(real_point3d *)((uint8_t *)a + 0x610), *(real_point3d *)((uint8_t *)a + 0x460)) > 0.25f);
        } else {
            changed = 0;
        }
        if (changed) {
            a->firing_target_ticks = 0;
        }
        a->firing_target_type = kind;
        if (kind == 1) {
            a->firing_target_prop_index = a->target_unit_index;
        } else if (kind == 2) {
            *(real_point3d *)&a->firing_target_prop_index = *(real_point3d *)((uint8_t *)a + 0x460);
        }
    }
    a->target_in_firing_range = 0;
    a->maximum_firing_distance = halo::ai::actor_has_unshielded_threat_weapon(actor_index) ? F(def, 0x74) : 0.0f;

    if (a->throw_grenade) {
        int16_t grenade = W(variant, 0x180);

        if (grenade != -1 && *(int8_t *)(OBJECT_DATA(a->unit_index) + 0x31e + grenade) == 0) {
            halo::units::unit_set_grenade_type_and_count_delta(a->unit_index, grenade, 1);
        }
        a->control_flags |= halo::units::to_bits(halo::units::unit_control_flag::grenade);
        halo::ai::ai_communication_broadcast(9, a->unit_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
        goto idle;
    }
    if (!halo::ai::actor_has_unshielded_threat_weapon(actor_index)) {
        goto idle;
    }
    if (a->firing_state == 4) {
        wants_fire = 1;
        goto dispatch;
    }

    if (W(def, 0x154) > 0 && a->firing_state != 2 && !(a->special_fire_timer > 0) && !(a->special_fire_strafe_cooldown > 0)) {
        uint8_t *threat_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
        uint8_t allowed;

        halo::game::weapon_get_zoom_fov_resolved(0x12, a->team);
        if (W(def, 0x154) == 1) {
            halo::game::weapon_get_zoom_fov_resolved(0x11, a->team);
            allowed = (uint8_t)(*(int32_t *)(threat_tag + 0x4fc) > 0);
        } else if (W(def, 0x154) == 2) {
            allowed = (uint8_t)(*(int32_t *)(threat_tag + 0x4fc) > 1);
        } else {
            allowed = 1;
        }
        if (allowed && halo::ai::actor_grenade_behavior_kind_allowed(actor_index, (int16_t)*(uint16_t *)(def + 0x156))) {
            float delay = halo::math::random_real_range(0.0f, 1.5f) + F(def, 0x15c);
            float roll = halo::math::random_real();

            a->special_fire_timer = (int16_t)(int32_t)(delay * 30.0f);
            if (roll < F(def, 0x158) &&
                halo::ai::actor_target_is_visible_or_object_count_ok(actor_index, (int16_t)*(uint16_t *)(def + 0x156))) {
                if (W(def, 0x156) == 3) {
                    a->special_fire_strafe_cooldown = 3;
                }
                if (W(def, 0x154) == 1) {
                    a->special_fire_overcharge = 1;
                } else if (W(def, 0x154) == 2) {
                    a->special_fire_secondary_pending = 1;
                }
            }
        }
    }

    if (a->firing_target_type > 0) {
        if (a->firing_target_type == 1) {
            prop *p = halo::ai::prop_at(a->firing_target_prop_index);

            a->firing_target_distance = p->distance;
            a->firing_target_point = p->center_of_mass;
            W(a, 0x626) = p->obstruction;
            a->unknown_620[1] = p->in_water;
            a->unknown_620[4] = 1;
            if (p->cluster_index != -1) {
                int32_t bit = p->cluster_index;

                a->unknown_620[4] = (uint8_t)!(*(uint32_t *)&local_player_globals->cluster_pvs[(bit >> 5)] & (1u << (bit & 0x1f)));
            }
        } else {
            a->firing_target_point = *(real_point3d *)&a->firing_target_prop_index;
            a->firing_target_distance = halo::math::vector3d_distance(*(real_point3d *)((uint8_t *)a + 0x610), a->aim_origin);
            a->unknown_620[1] = 0;
            a->unknown_620[4] = 0;
            if (a->firing_target_ticks % 10 == 0) {
                W(a, 0x626) = (int16_t)halo::ai::actor_evaluate_engagement_reachability(W(a, 0x148), -1,
                    &a->firing_target_point, &a->aim_origin, 0, 0, k_datum_index_none,
                    (uint8_t)(a->active_unit_index != k_datum_index_none));
            }
        }
        a->unknown_620[2] = (uint8_t)(F(def, 0x148) > 0.0f && a->firing_target_distance > F(def, 0x148));
        a->unknown_620[3] = (uint8_t)(a->unknown_455[0] && F(def, 0x14c) > 0.0f);
        if (!halo::items::weapon_trigger_get_aiming_vector(weapon, 0, &a->aim_origin, &a->firing_target_point,
                                              a->unknown_620[2], (real_vector3d *)((uint8_t *)a + 0x63c), 0, (real *)((uint8_t *)a + 0x648),
                                              &used_straight_line)) {
            a->firing_target_type = 0;
        }
    }
    if (a->firing_target_type == 0 || a->unknown_620[4]) {
        goto idle;
    }

    {
        uint8_t forced = a->force_fire;

        if (!forced && a->firing_delay_timer > 0) goto idle;
        if (halo::ai::actor_action_has_queued_secondary(actor_index)) goto idle;
        if (!forced) {
            if (a->airborne && !a->flying && !(variant[0] & 1)) goto idle;
            if ((*(uint32_t *)actor_tag & 0x200) && !a->crouching) goto idle;
            if ((actor_tag[4] & 2) && a->crouching) goto idle;
            if ((actor_tag[4] & 4) && a->moving) goto idle;
        }
        if (a->unknown_620[1] || a->in_water) goto idle;
        if (weapon_tag != 0 && F(weapon_tag, 0x40c) > 0.0f && a->firing_target_distance < F(weapon_tag, 0x40c)) goto idle;
        if (a->flee_reason == 0 || a->flee_source.code != 2 || a->unknown_58c[0]) goto idle;
        if (a->firing_state == 2) {
            wants_fire = 1;
            goto dispatch;
        }
        a->unknown_620[0] = (uint8_t)(W(a, 0x626) == 0 || W(a, 0x626) == 1);
        if (!a->unknown_620[0] && !a->unknown_620[3]) goto idle;
        if (!forced && !(a->firing_target_distance < a->maximum_firing_distance)) goto idle;
        wants_fire = 1;
        a->target_in_firing_range = 1;
        if (!(*(uint32_t *)actor_tag & 0x2000)) {
            float tolerance = a->firing_target_distance >= 1.5f ? 0.97f : a->firing_target_distance * 0.17526217f + 0.70710677f;
            real_vector3d aim;

            halo::ai::actor_get_aim_from_position(actor_index, (uint32_t *)&aim);
            if (!(aim.j * F(a, 0x640) + aim.k * F(a, 0x644) + aim.i * F(a, 0x63c) >= tolerance)) {
                fire_primary = 1;
            }
        }
        goto dispatch;
    }

idle:
    wants_fire = 0;
    a->firing_state = 0;

dispatch:
    {
        int16_t next = -1;

        switch (a->firing_state) {
        case 0:
            if (wants_fire) next = 1;
            break;
        case 1:
        case 3:
            if (!fire_primary && a->firing_state_timer == 0) next = 2;
            break;
        case 2:
            if (a->firing_state_timer == 0) next = 3;
            break;
        case 4:
            if (a->firing_state_timer == 0) next = 0;
            break;
        default:
            break;
        }
        if (next != -1) {
            if (next == 1) {
                if (!halo::ai::actor_should_hold_position(actor_index, def) && !fire_primary) {
                    next = 2;
                    halo::ai::actor_update_aim_wander(actor_index);
                }
            } else if (next == 2) {
                halo::ai::actor_update_aim_wander(actor_index);
            } else if (next == 3) {
                halo::ai::actor_reseed_movement_pause_timer(actor_index);
            }
            a->firing_state = next;
        }
    }

    fire_primary = 0;
    fire_secondary = 0;
    a->firing_vector_ballistic = 0;
    state = a->firing_state;
    if (state == 4) {
        fire_primary = 1;
    } else if (state == 2) {
        datum_index exclude = k_datum_index_none;
        real_point3d *aim_point = (real_point3d *)((uint8_t *)a + 0x658);
        real_point3d *final_point = (real_point3d *)((uint8_t *)a + 0x67c);
        real_point3d origin;
        int32_t blocking_prop = -1;

        *aim_point = a->aim_target_point;
        if (a->firing_target_type == 1) {
            prop *p = halo::ai::prop_at(a->firing_target_prop_index);
            float f;

            exclude = D(p, 0x114);
            f = halo::game::weapon_get_zoom_fov_resolved(0xf, a->team) + F(def, 0xbc);
            if (!(f <= 0.0f && f < 1.0f) && !a->unknown_620[3]) {
                real_vector3d delta;

                delta.i = p->center_of_mass.x - a->aim_target_point.x;
                delta.j = p->center_of_mass.y - a->aim_target_point.y;
                delta.k = p->center_of_mass.z - a->aim_target_point.z;
                halo::math::point3d_add_scaled(*aim_point, delta, *aim_point, F(def, 0xbc));
            }
            f = halo::game::weapon_get_zoom_fov_resolved(0x10, a->team) + F(def, 0xc0);
            if (!(f <= 0.0f && f < 1.0f)) {
                float lead = F(def, 0xc0);
                real t = halo::items::weapon_trigger_projectile_time_fraction(weapon, (int16_t)(a->special_fire_secondary != 0), F(a, 0x648));

                aim_point->x += t * p->velocity.x * lead;
                aim_point->y += t * p->velocity.y * lead;
                aim_point->z += t * p->velocity.z * lead;
            }
        }
        a->aim_wander_offset.i = a->aim_recoil_per_tick.i + a->aim_wander_offset.i;
        a->aim_wander_offset.j = a->aim_recoil_per_tick.j + a->aim_wander_offset.j;
        a->aim_wander_offset.k = a->aim_recoil_per_tick.k + a->aim_wander_offset.k;
        final_point->x = a->aim_wander_offset.i + aim_point->x;
        final_point->y = aim_point->y + a->aim_wander_offset.j;
        final_point->z = aim_point->z + a->aim_wander_offset.k;

        if (a->active_unit_index != k_datum_index_none) {
            halo::units::unit_get_camera_position(a->unit_index, &origin);
        } else {
            uint8_t *offset = 0;

            if (a->crouching) {
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
                origin = *(real_point3d *)&a->aim_origin.x;
            } else {
                real_vector3d facing;

                facing.i = a->body_position.x - final_point->x;
                facing.j = a->body_position.y - final_point->y;
                facing.k = a->body_position.z - final_point->z;
                if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&facing)) > 0.0f) {
                    facing.k = 0.0f;
                } else {
                    facing = *(real_vector3d *)&a->facing.i;
                }
                halo::units::unit_add_marker_relative_offset(a->unit_index, 3, (float *)((uint8_t *)a + 0x12c), (uint32_t)&facing,
                                                (uint32_t)offset, &origin);
            }
        }

        halo::items::weapon_trigger_get_aiming_vector(weapon, (int16_t)(a->special_fire_secondary != 0), &origin, final_point, a->unknown_620[2],
                                         &a->firing_vector, 0, 0, &used_straight_line);
        a->firing_vector_ballistic = (uint8_t)(used_straight_line == 0);
        {
            real_vector3d path;

            path.i = final_point->x - origin.x;
            path.j = final_point->y - origin.y;
            path.k = final_point->z - origin.z;
            if (halo::ai::actor_grenade_trajectory_blocked(&path, actor_index, exclude, &origin, &blocking_prop)) {
                a->line_of_fire_blocked_ticks = 0;
                if (a->special_fire_secondary) {
                    fire_secondary = 1;
                } else {
                    fire_primary = 1;
                }
            } else {
                a->line_of_fire_blocked_ticks += 1;
                a->firing_state_timer += 1;
                if (a->line_of_fire_blocked_ticks >= 0x2d && a->target_combat_status >= 7) {
                    datum_index in_the_way = blocking_prop != -1 ? D(PROP(blocking_prop), 0x18) : k_datum_index_none;

                    halo::ai::ai_communication_broadcast(0xe, a->unit_index, in_the_way, 2, k_datum_index_none, k_datum_index_none, 0);
                    a->line_of_fire_blocked_ticks = 0;
                }
            }
        }
    }

    if (fire_primary) {
        float burst = F(def, 0x78);

        if (a->special_fire_overcharge) {
            a->special_fire_overcharge = 0;
        } else if (burst == 0.0f) {
            enable = 1;
            value = 1.0f;
        } else if (a->refire_timer == 0) {
            uint8_t *stance_a;
            uint8_t *stance_b = 0;
            float rate;
            int16_t ticks;

            enable = 1;
            value = 1.0f;
            rate = halo::game::weapon_get_zoom_fov_resolved(0xa, a->team) * burst;
            halo::ai::actor_select_stance_offset_pair(actor_index, def, &stance_a, &stance_b);
            if (stance_b != 0 && F(stance_b, 0x8) > 0.0f) {
                rate *= F(stance_b, 0x8);
            }
            ticks = (int16_t)fistp_round(30.0f / rate);
            a->refire_timer = ticks < 2 ? 2 : ticks;
        }
    } else if (fire_secondary) {
        if (a->special_fire_overcharge) {
            a->special_fire_overcharge = 0;
        } else {
            secondary_flag = 1;
        }
    } else if (a->special_fire_overcharge) {
        enable = 1;
        value = 1.0f;
    }
    halo::ai::actor_set_override_target(actor_index, enable, *(datum_index *)&value);
    if (secondary_flag) {
        a->control_flags |= halo::units::to_bits(halo::units::unit_control_flag::secondary_trigger);
    } else {
        a->control_flags &= ~halo::units::to_bits(halo::units::unit_control_flag::secondary_trigger);
    }
}

#undef F
#undef W
#undef D
#undef PROP
#undef OBJECT_DATA
#undef TAG_DATA

}
