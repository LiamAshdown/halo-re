#include "halo/ai/actor_view.hpp"
#include "halo/ai/flags.hpp"
#include "halo/tags/flags.hpp"
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
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/x87.hpp"

namespace halo::ai {

namespace actor_update_firing_state_local {
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
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
    Actor *actor_tag = halo::ai::tag_data<Actor>(a->actor_definition_tag);
    ActorVariant *variant = halo::ai::tag_data<ActorVariant>(a->actor_variant_tag);
    ActorVariant *def = reinterpret_cast<ActorVariant *>(halo::ai::actor_get_actor_definition(actor_index));
    Weapon *weapon_tag = 0;
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
        weapon_tag = halo::ai::tag_data<Weapon>(halo::ai::object_at(weapon)->definition_tag);
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
            if (a->forced_aim_valid) {
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
            changed = (uint8_t)(halo::math::vector3d_distance_squared(a->firing_target_free_point, a->forced_aim_point) > 0.25f);
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
            *(real_point3d *)&a->firing_target_prop_index = a->forced_aim_point;
        }
    }
    a->target_in_firing_range = 0;
    a->maximum_firing_distance = halo::ai::actor_has_unshielded_threat_weapon(actor_index) ? def->maximum_firing_distance : 0.0f;

    const bool go_idle = [&]() -> bool {
        if (a->throw_grenade) {
            int16_t grenade = variant->grenade_type;

            if (grenade != -1 && halo::units::unit_data_of(halo::ai::object_at(a->unit_index))->grenade_counts[grenade] == 0) {
                halo::units::unit_set_grenade_type_and_count_delta(a->unit_index, grenade, 1);
            }
            a->control_flags |= halo::units::to_bits(halo::units::unit_control_flag::grenade);
            halo::ai::ai_communication_broadcast(9, a->unit_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
            return true;
        }
        if (!halo::ai::actor_has_unshielded_threat_weapon(actor_index)) {
            return true;
        }
        if (a->firing_state == 4) {
            wants_fire = 1;
            return false;
        }

        if (def->special_fire_mode > 0 && a->firing_state != 2 && !(a->special_fire_timer > 0) && !(a->special_fire_strafe_cooldown > 0)) {
            Weapon *threat_tag = halo::ai::tag_data<Weapon>(halo::ai::object_at(weapon)->definition_tag);
            uint8_t allowed;

            halo::game::weapon_get_zoom_fov_resolved(0x12, a->team);
            if (def->special_fire_mode == 1) {
                halo::game::weapon_get_zoom_fov_resolved(0x11, a->team);
                allowed = (uint8_t)((int32_t)threat_tag->triggers.count > 0);
            } else if (def->special_fire_mode == 2) {
                allowed = (uint8_t)((int32_t)threat_tag->triggers.count > 1);
            } else {
                allowed = 1;
            }
            if (allowed && halo::ai::actor_grenade_behavior_kind_allowed(actor_index, (int16_t)def->special_fire_situation)) {
                float delay = halo::math::random_real_range(0.0f, 1.5f) + def->special_fire_delay;
                float roll = halo::math::random_real();

                a->special_fire_timer = (int16_t)(int32_t)(delay * 30.0f);
                if (roll < def->special_fire_chance &&
                    halo::ai::actor_target_is_visible_or_object_count_ok(actor_index, (int16_t)def->special_fire_situation)) {
                    if (def->special_fire_situation == 3) {
                        a->special_fire_strafe_cooldown = 3;
                    }
                    if (def->special_fire_mode == 1) {
                        a->special_fire_overcharge = 1;
                    } else if (def->special_fire_mode == 2) {
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
                a->firing_target_obstruction = p->obstruction;
                a->firing_target_in_water = p->in_water;
                a->firing_target_hidden = 1;
                if (p->location.cluster_index != -1) {
                    int32_t bit = p->location.cluster_index;

                    a->firing_target_hidden = (uint8_t)!(*(uint32_t *)&local_player_globals->cluster_pvs[(bit >> 5)] & (1u << (bit & 0x1f)));
                }
            } else {
                a->firing_target_point = *(real_point3d *)&a->firing_target_prop_index;
                a->firing_target_distance = halo::math::vector3d_distance(a->firing_target_free_point, a->aim_origin);
                a->firing_target_in_water = 0;
                a->firing_target_hidden = 0;
                if (a->firing_target_ticks % 10 == 0) {
                    a->firing_target_obstruction = (int16_t)halo::ai::actor_evaluate_engagement_reachability(a->location.cluster_index, -1,
                        &a->firing_target_point, &a->aim_origin, 0, 0, k_datum_index_none,
                        (uint8_t)(a->active_unit_index != k_datum_index_none));
                }
            }
            a->use_high_arc = (uint8_t)(def->super_ballistic_range > 0.0f && a->firing_target_distance > def->super_ballistic_range);
            a->fire_blindly = (uint8_t)(a->unknown_455[0] && def->bombardment_range > 0.0f);
            if (!halo::items::weapon_trigger_get_aiming_vector(weapon, 0, &a->aim_origin, &a->firing_target_point,
                                                  a->use_high_arc, &a->target_aim_vector, 0, &a->target_aim_range,
                                                  &used_straight_line)) {
                a->firing_target_type = 0;
            }
        }
        if (a->firing_target_type == 0 || a->firing_target_hidden) {
            return true;
        }

        {
            uint8_t forced = a->force_fire;

            if (!forced && a->firing_delay_timer > 0) return true;
            if (halo::ai::actor_action_has_queued_secondary(actor_index)) return true;
            if (!forced) {
                if (a->airborne && !a->flying && !(static_cast<uint8_t>(variant->flags) & 1)) return true;
                if (halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::must_crouch_to_shoot) && !a->crouching) return true;
                if (halo::ai::flag_set(actor_tag->more_flags, halo::tags::actor_more_tag_flag::must_stand_to_fire) && a->crouching) return true;
                if (halo::ai::flag_set(actor_tag->more_flags, halo::tags::actor_more_tag_flag::must_stop_to_fire) && a->moving) return true;
            }
            if (a->firing_target_in_water || a->in_water) return true;
            if (weapon_tag != 0 && weapon_tag->minimum_target_range > 0.0f && a->firing_target_distance < weapon_tag->minimum_target_range) return true;
            if (a->flee_reason == 0 || a->flee_source.code != 2 || a->look_claimed) return true;
            if (a->firing_state == 2) {
                wants_fire = 1;
                return false;
            }
            a->firing_line_clear = (uint8_t)(a->firing_target_obstruction == 0 || a->firing_target_obstruction == 1);
            if (!a->firing_line_clear && !a->fire_blindly) return true;
            if (!forced && !(a->firing_target_distance < a->maximum_firing_distance)) return true;
            wants_fire = 1;
            a->target_in_firing_range = 1;
            if (!halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::start_firing_before_aligned)) {
                float tolerance = a->firing_target_distance >= 1.5f ? 0.97f : a->firing_target_distance * 0.17526217f + 0.70710677f;
                real_vector3d aim;

                halo::ai::actor_get_aim_from_position(actor_index, (uint32_t *)&aim);
                if (!(aim.j * a->target_aim_vector.j + aim.k * a->target_aim_vector.k + aim.i * a->target_aim_vector.i >= tolerance)) {
                    fire_primary = 1;
                }
            }
            return false;
        }
    }();
    if (go_idle) {
        wants_fire = 0;
        a->firing_state = 0;
    }

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
        real_point3d *aim_point = &a->firing_aim_point;
        real_point3d *final_point = (real_point3d *)&a->grenade_aim_direction;
        real_point3d origin;
        int32_t blocking_prop = -1;

        *aim_point = a->aim_target_point;
        if (a->firing_target_type == 1) {
            prop *p = halo::ai::prop_at(a->firing_target_prop_index);
            float f;

            exclude = p->parent_object_index;
            f = halo::game::weapon_get_zoom_fov_resolved(0xf, a->team) + def->target_tracking;
            if (!(f <= 0.0f && f < 1.0f) && !a->fire_blindly) {
                real_vector3d delta;

                delta.i = p->center_of_mass.x - a->aim_target_point.x;
                delta.j = p->center_of_mass.y - a->aim_target_point.y;
                delta.k = p->center_of_mass.z - a->aim_target_point.z;
                halo::math::point3d_add_scaled(*aim_point, delta, *aim_point, def->target_tracking);
            }
            f = halo::game::weapon_get_zoom_fov_resolved(0x10, a->team) + def->target_leading;
            if (!(f <= 0.0f && f < 1.0f)) {
                float lead = def->target_leading;
                real t = halo::items::weapon_trigger_projectile_time_fraction(weapon, (int16_t)(a->special_fire_secondary != 0), a->target_aim_range);

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
                Vector3D *v = &def->custom_crouch_gun_offset;

                if (v->i * v->i + v->j * v->j + v->k * v->k > 9.999999747378752e-05f) {
                    offset = reinterpret_cast<uint8_t *>(v);
                } else if (halo::math::vector3d_magnitude_squared(*reinterpret_cast<real_vector3d *>(&actor_tag->crouching_gun_offset)) > 9.999999747378752e-05f) {
                    offset = reinterpret_cast<uint8_t *>(&actor_tag->crouching_gun_offset);
                }
            } else {
                Vector3D *v = &def->custom_stand_gun_offset;

                if (v->i * v->i + v->j * v->j + v->k * v->k > 9.999999747378752e-05f) {
                    offset = reinterpret_cast<uint8_t *>(v);
                } else if (halo::math::vector3d_magnitude_squared(*reinterpret_cast<real_vector3d *>(&actor_tag->standing_gun_offset)) > 9.999999747378752e-05f) {
                    offset = reinterpret_cast<uint8_t *>(&actor_tag->standing_gun_offset);
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
                halo::units::unit_add_marker_relative_offset(a->unit_index, 3, (float *)&a->body_position, (uint32_t)&facing,
                                                (uint32_t)offset, &origin);
            }
        }

        halo::items::weapon_trigger_get_aiming_vector(weapon, (int16_t)(a->special_fire_secondary != 0), &origin, final_point, a->use_high_arc,
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
                    datum_index in_the_way = blocking_prop != -1 ? halo::ai::prop_at(blocking_prop)->object_index : k_datum_index_none;

                    halo::ai::ai_communication_broadcast(0xe, a->unit_index, in_the_way, 2, k_datum_index_none, k_datum_index_none, 0);
                    a->line_of_fire_blocked_ticks = 0;
                }
            }
        }
    }

    if (fire_primary) {
        float burst = def->rate_of_fire;

        if (a->special_fire_overcharge) {
            a->special_fire_overcharge = 0;
        } else if (burst == 0.0f) {
            enable = 1;
            value = 1.0f;
        } else if (a->refire_timer == 0) {
            actor_burst_parameters *stance_a;
            actor_burst_scale *stance_b = 0;
            float rate;
            int16_t ticks;

            enable = 1;
            value = 1.0f;
            rate = halo::game::weapon_get_zoom_fov_resolved(0xa, a->team) * burst;
            halo::ai::actor_select_stance_offset_pair(actor_index, def, &stance_a, &stance_b);
            if (stance_b != 0 && stance_b->rate_of_fire > 0.0f) {
                rate *= stance_b->rate_of_fire;
            }
            ticks = (int16_t)halo::x87::fistp_round(30.0f / rate);
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


}
