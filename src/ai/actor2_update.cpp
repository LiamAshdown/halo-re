#include "halo/game/constants.hpp"
#include "halo/ai/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"

namespace halo::ai {

namespace actor_update_activation_state_local {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}

/**
 * Per-tick actor combat-state update: validates via actor_update_squad_link_state; if it says the link is still
 * good, refreshes idle/combat context, target relationship, best target and crouch state, then clears the
 * vocalization/recognition scratch run, and runs the mode-transition loop and its enter-
 *
 * @address 0x429160
 */
void ActorView::update_activation_state()
{
    using namespace actor_update_activation_state_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (halo::ai::actor_update_squad_link_state(actor_index) == 0) {
        return;
    }

    halo::ai::actor_update_idle_stagger(actor_index);
    halo::ai::actor_refresh_combat_context(actor_index);
    halo::ai::actor_target_relationship_think(actor_index);
    halo::ai::actor_choose_best_target(actor_index);
    halo::ai::actor_update_crouch_state(actor_index);

    memset(&self->flee_reason, 0, 0x21 * sizeof(uint32_t));
    self->secondary_action = -1;
    self->movement_style_override = -1;
    self->strafe_axis_override = -1;

    halo::ai::actor_run_mode_transition_loop(actor_index);

    {
        uint32_t proc = actor_mode_definitions[self->mode].tick_proc;
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }

    if (self->keep_unit_alive == 0) {
        if (self->swarm != 0) {
            halo::ai::actor_dispatch_type_vtable_0x18(actor_index);
            return;
        }
        halo::ai::actor_snapshot_orientation(actor_index);
        halo::ai::actor_invoke_type_handler(actor_index);
        halo::ai::actor_update_grenade_eligibility_state(actor_index);
        halo::ai::actor_schedule_grenade_throw(actor_index);
        halo::ai::actor_movement_advance_waypoint(actor_index);
        halo::ai::actor_update_flee_response(actor_index);
        halo::ai::actor_movement_update(actor_index);
        halo::ai::actor_update_look_target(actor_index);
        halo::ai::actor_update_firing_state(actor_index);
        halo::ai::actor_apply_queued_look_to_unit(actor_index);
    }
}

namespace actor_update_aim_wander_local {
static float aim_wander_random_fraction(void)
{
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    return (float)(int32_t)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
}
}

/**
 * Actor AI behaviour: update aim wander.
 *
 * @address 0x40fcb0
 */
void ActorView::update_aim_wander()
{
    using namespace actor_update_aim_wander_local;
    actor *a = halo::ai::actor_at(actor_index);
    ActorVariant *variant = reinterpret_cast<ActorVariant *>(halo::ai::actor_get_actor_definition(actor_index));
    int16_t team = a->team;
    actor_burst_parameters *burst = 0;
    actor_burst_scale *scale = 0;
    uint8_t bombard = 0;
    float time;
    float error;
    float angle_1, angle_2;
    float radius_a, radius_b;
    real_point3d target;
    real_vector3d side;
    real_vector3d wander;
    real_vector3d recoil;
    uint8_t moving;

    if (a->special_fire_secondary_pending != 0 && !halo::ai::actor_target_is_visible_or_object_count_ok(actor_index,
            (int16_t)static_cast<uint16_t>(((struct ActorVariant *)variant)->special_fire_situation))) {
        a->special_fire_secondary_pending = 0;
    }
    a->special_fire_secondary = a->special_fire_secondary_pending;
    a->special_fire_secondary_pending = 0;

    if (a->active_unit_index == k_datum_index_none) {
        moving = (a->airborne != 0 || a->moving != 0) ? 1 : 0;
    } else {
        object *vehicle = (object *)halo::ai::object_at(a->active_unit_index);
        real_vector3d *velocity = &vehicle->velocity;

        moving = (velocity->i * velocity->i + velocity->j * velocity->j + velocity->k * velocity->k > 1.0f) ? 1 : 0;
    }
    a->moving_firing_pattern = moving;
    a->new_target_firing_pattern = (halo::game::weapon_get_zoom_fov_resolved(0xd, team) * ((ActorVariant *)variant)->new_target_firing_pattern_time * halo::game::k_ticks_per_second_f >
        (float)a->firing_target_ticks) ? 1 : 0;

    halo::ai::actor_select_stance_offset_pair(actor_index, variant, &burst, &scale);

    if (a->burst_duration_override > 0.0f) {
        time = a->burst_duration_override;
    } else {
        time = aim_wander_random_fraction() * (burst->duration[1] - burst->duration[0]) +
            burst->duration[0];
        if (scale != 0 && scale->duration != 0.0f) {
            time = time * scale->duration;
        }
        if (a->playfight != 0) {
            time = time * 0.6f;
        }
    }
    a->firing_state_timer = (int16_t)(int32_t)(time * halo::game::k_ticks_per_second_f);

    error = halo::game::weapon_get_zoom_fov_resolved(0xb, team) * ((ActorVariant *)variant)->projectile_error;
    if (scale != 0 && scale->projectile_error != 0.0f) {
        error = error * scale->projectile_error;
    }
    if (a->playfight != 0) {
        error = error + error + 0.017453292f;
    }
    a->projectile_error = error;

    a->perception_scale = 0.0f;
    if (((ActorVariant *)variant)->weapon_damage_modifier > 0.0f) {
        a->perception_scale = ((ActorVariant *)variant)->weapon_damage_modifier;
    } else if (((ActorVariant *)variant)->damage_per_second > 0.0f) {
        datum_index weapon = halo::ai::actor_get_threat_weapon_object_index(actor_index);

        if (weapon != k_datum_index_none) {
            float rate;
            float damage = halo::items::weapon_trigger_get_average_damage(
                *(datum_index *)halo::ai::object_at(weapon), &rate);

            if (((ActorVariant *)variant)->rate_of_fire > 0.0f && rate > ((ActorVariant *)variant)->rate_of_fire) {
                rate = ((ActorVariant *)variant)->rate_of_fire;
            }
            damage = damage * rate;
            if (damage > 0.0f) {
                a->perception_scale = ((ActorVariant *)variant)->damage_per_second / damage;
            }
        }
    }
    if (a->special_fire_secondary != 0 || a->special_fire_overcharge != 0) {
        if (((ActorVariant *)variant)->special_damage_modifier > 0.0f) {
            a->perception_scale = a->perception_scale * ((ActorVariant *)variant)->special_damage_modifier;
        }
        a->projectile_error = ((ActorVariant *)variant)->special_projectile_error + a->projectile_error;
    }

    if (((ActorVariant *)variant)->bombardment_range > 0.0f && a->firing_target_type == 1) {
        struct prop *prop = halo::ai::prop_at(a->firing_target_prop_index);
        int16_t kind = prop->state;

        bombard = (kind < 2 || kind > 3 || prop->visual_perception == 0) ? 1 : 0;
    }
    target = a->firing_target_point;
    if (bombard) {
        halo::ai::actor_choose_random_point_near(&target, ((ActorVariant *)variant)->bombardment_range);
    }
    {
        float dx = target.x - a->aim_origin.x;
        float dy = target.y - a->aim_origin.y;
        float dz = (target.z - a->aim_origin.z) * 0.0f;

        side.i = dy - dz;
        side.j = dz - dx;
        side.k = dx * 0.0f - dy * 0.0f;
    }
    halo::math::vector3d_normalize_with_length(side);
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    if ((uint16_t)(halo::math::globals().random_seed_global >> 16) > 0x8000) {
        side.i = -side.i;
        side.j = -side.j;
        side.k = -side.k;
    }

    angle_1 = aim_wander_random_fraction() * (burst->origin_angle + burst->origin_angle) - burst->origin_angle;
    angle_2 = aim_wander_random_fraction() * (burst->return_angle + burst->return_angle) -
        burst->return_angle + angle_1;
    radius_a = halo::game::weapon_get_zoom_fov_resolved(0xc, team) * burst->origin_radius;
    radius_b = aim_wander_random_fraction() * (burst->return_length[1] - burst->return_length[0]) + burst->return_length[0];
    radius_b = halo::game::weapon_get_zoom_fov_resolved(0xc, team) * radius_b;
    if (a->playfight != 0) {
        radius_a = radius_a + radius_a;
        radius_b = radius_b + radius_b;
    }

    if (a->firing_state_timer > 0 && burst->angular_velocity > 0.0f) {
        float ticks = (float)(int32_t)a->firing_state_timer;
        float sweep = ticks * burst->angular_velocity * 0.033333335f;
        float limit;

        if (!(sweep <= 0.7853982f)) {
            sweep = 0.7853982f;
        }
        limit = (float)halo::x87::ftan((double)sweep) * a->firing_target_distance;
        if (radius_a > limit) {
            float limit_15 = limit * 1.5f;

            if (radius_a >= limit_15) {
                a->firing_state_timer = (int16_t)halo::x87::fistp_round(ticks * 1.5f);
                radius_b = limit_15 / radius_a * radius_b;
                radius_a = limit_15;
            } else {
                a->firing_state_timer = (int16_t)halo::x87::fistp_round(radius_a / limit * ticks);
            }
        }
    }

    {
        float c1 = (float)halo::x87::fcos((double)angle_1), s1 = (float)halo::x87::fsin((double)angle_1);
        float c2 = (float)halo::x87::fcos((double)angle_2), s2 = (float)halo::x87::fsin((double)angle_2);

        wander.i = (side.i * c1 + 0.0f * s1) * radius_a;
        wander.j = (side.j * c1 + 0.0f * s1) * radius_a;
        wander.k = (side.k * c1 + s1) * radius_a;
        recoil.i = -((side.i * c2 + s2 * 0.0f) * radius_b);
        recoil.j = -((side.j * c2 + s2 * 0.0f) * radius_b);
        recoil.k = -((side.k * c2 + s2) * radius_b);
    }
    if (a->firing_state_timer > 0) {
        float per_tick = 1.0f / (float)(int32_t)a->firing_state_timer;

        recoil.i = recoil.i * per_tick;
        recoil.j = recoil.j * per_tick;
        recoil.k = recoil.k * per_tick;
    }
    a->aim_target_point = target;
    a->aim_wander_offset = wander;
    a->aim_recoil_per_tick = recoil;
    a->grenade_aim_direction.i = wander.i + target.x;
    a->grenade_aim_direction.j = wander.j + target.y;
    a->grenade_aim_direction.k = wander.k + target.z;

    if (a->combat_status >= 7) {
        uint8_t prop_flag = 0;
        datum_index object = k_datum_index_none;
        int32_t code;

        if (a->firing_target_type == 1) {
            struct prop *prop = halo::ai::prop_at(a->firing_target_prop_index);

            prop_flag = prop->allegiance;
            object = prop->object_index;
        }
        if (a->berserking != 0) {
            code = 0x1c;
        } else if (prop_flag) {
            code = 0x1e;
        } else if ((int8_t)a->tally.threat_class_ge_1 >= 5) {
            code = 0x1d;
        } else {
            code = 0x1a + (a->vehicle_gunner != 0);
        }
        halo::ai::ai_communication_broadcast(code, a->unit_index, object, 3, k_datum_index_none, k_datum_index_none, 0);
    }
}

namespace actor_update_awareness_level_local {
static auto &actor_combat_status_min_grade = halo::link::ref<int16_t []>(halo::ai::vars().actor_combat_status_min_grade);
}

/**
 * Advances the actor's alertness/awareness state machine each tick from the highest-priority perception event
 * and the current target's combat status.
 *
 * @address 0x420290
 */
void ActorView::update_awareness_level()
{
    using namespace actor_update_awareness_level_local;
    actor *self;
    int16_t event;
    int16_t burst_counter;
    int16_t status_min_grade;
    int16_t event_floor;
    int16_t old_grade;
    int16_t new_grade;

    self = halo::ai::actor_at(actor_index);

    event = self->perception_event;
    if (0 < event) {
        if (self->suspicion_status < event) {
            self->suspicion_status = event;
            self->suspicion_timer = self->perception_event_data;
        } else if (self->suspicion_status == event) {
            if (self->suspicion_timer <= self->perception_event_data) {
                self->suspicion_timer = self->perception_event_data;
            }
        }
        self->perception_event = 0;
    }

    burst_counter = self->minimum_combat_status;
    status_min_grade = actor_combat_status_min_grade[self->target_combat_status];
    event_floor = (burst_counter <= status_min_grade) ? status_min_grade : burst_counter;

    old_grade = self->suspicion_status;
    new_grade = old_grade;
    if (old_grade <= event_floor) {
        new_grade = (burst_counter <= status_min_grade) ? status_min_grade : burst_counter;
    }
    self->combat_status = new_grade;

    if (old_grade < new_grade) {
        self->suspicion_status = 0;
    }

    if (self->awareness_level < 3) {
        self->ticks_in_combat = 0;
    } else {
        self->ticks_in_combat = self->ticks_in_combat + 1;
    }

    bool threatened = false;

    if (new_grade == 0) {
        self->ticks_alerted = 0;
    } else {
        self->ticks_alerted = self->ticks_alerted + 1;
        if (3 < new_grade) {
            self->ticks_threatened = self->ticks_threatened + 1;
            self->ticks_since_threatened = 0;
            threatened = true;
        }
    }
    if (!threatened) {
        self->ticks_threatened = 0;
        if (self->ticks_since_threatened != -1) {
            self->ticks_since_threatened = self->ticks_since_threatened + 1;
        }
    }

    if (6 < new_grade) {
        self->has_engaged = 1;
    }
}

namespace actor_update_combat_behavior_local {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}

/**
 * Actor AI behaviour: update combat behavior.
 *
 * @address 0x40d610
 */
uint8_t ActorView::update_combat_behavior(uint8_t param_1, uint8_t param_2)
{
    using namespace actor_update_combat_behavior_local;
    actor *self;
    uint8_t result;
    uint8_t use_param_1;
    prop *target_prop;

    self = halo::ai::actor_at(actor_index);
    result = 0;
    use_param_1 = 1;
    if (param_2 == 0) {
        use_param_1 = param_1;
    }

    enum class next_action { use_default, melee, transition };
    next_action action = next_action::use_default;

    switch (actor_mode_definitions[self->mode].combat_grade) {
    case 1:
    case 2:
        if (use_param_1 == 0) {
            break;
        }
        if (self->combat_status < 5) {
            if (self->combat_status < 2 && self->mode != halo::ai::actor_mode::alert &&
                (self->combat_status != 0 || (self->stood_down == 0 && self->post_combat_action < 1))) {
                break;
            }
            action = next_action::melee;
        } else {
            action = next_action::transition;
        }
        break;
    case 3:
        if (use_param_1 == 0 || self->combat_status < 4) {
            action = next_action::melee;
            if (1 < self->combat_status) {
                if (self->target_unit_index == (datum_index)k_datum_index_none) {
                    action = next_action::use_default;
                    break;
                }
                target_prop = halo::ai::prop_at(self->target_unit_index);
                if (self->target_unit_index == self->pursuit_target_prop_index &&
                    (target_prop->noticed_a != 0 || (self->mode == halo::ai::actor_mode::uncover && self->mode_data.uncover.stage == 0)) &&
                    (target_prop->noticed_b != 0 ||
                     ((self->mode == halo::ai::actor_mode::uncover && self->mode_data.uncover.stage == 0) ||
                      (self->mode == halo::ai::actor_mode::search && self->mode_data.search.stage == 0)))) {
                    action = next_action::use_default;
                }
            }
        } else {
            action = next_action::transition;
        }
        break;
    case 4:
        action = self->combat_status < 4 ? next_action::melee : next_action::transition;
        break;
    default:
        break;
    }
    if (action == next_action::melee) {
        result = halo::ai::actor_update_melee_combat_action(actor_index);
    } else if (action == next_action::transition) {
        result = halo::ai::actor_evaluate_combat_state_transition(actor_index);
    }
    if (result != 0) {
        return result;
    }
    if (param_2 != 0) {
        result = halo::ai::actor_update_melee_combat_action(actor_index);
    }
    return result;
}

namespace actor_update_crouch_state_local {
}

/**
 * 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call site in the binary cleans
 * up 0x1c bytes, so the shorter forms Ghidra recovers at some sites are artefacts, not a reduced-arity overload.
 *
 * @address 0x4213b0
 */
void ActorView::update_crouch_state()
{
    using namespace actor_update_crouch_state_local;
    actor *self;
    Actor *actor_definition;
    prop *p;
    datum_index prop_index;
    float *threat_level;
    float *threat_level_smoothed;
    uint8_t *crouching;
    int16_t *crouch_timer;
    uint8_t *flag_35c;
    uint8_t *flag_35d;
    uint8_t *flag_35e;
    uint8_t *flag_35f;
    int16_t *countdown_360;
    int16_t *countdown_368;
    real_vector3d cover_direction;
    real_vector3d target_direction;
    real_vector3d flank_offset;
    real_vector3d steering_direction;
    real_point3d probe_point;
    float decay;
    float threshold;
    float cosine_limit;
    float dot;
    int16_t combat_status;
    int16_t threat_class;
    int16_t grade;
    int16_t grade_second;
    uint8_t platoon_flag;
    uint8_t want_crouch;

    self = halo::ai::actor_at(actor_index);
    actor_definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    threat_level           = &self->threat_level;
    threat_level_smoothed  = &self->danger_meter;
    crouching              = &self->crouch_active;
    crouch_timer           = &self->crouch_ticks;
    flag_35c               = &self->crouch_cover_flags[0];
    flag_35d               = &self->crouch_cover_flags[1];
    flag_35e               = &self->crouch_cover_flags[2];
    flag_35f               = &self->crouch_cover_flags[3];
    countdown_360          = &self->incoming_fire_ticks;
    countdown_368          = &self->evasion_delay_ticks;

    if (self->berserking != 0 &&
        (self->combat_status == 0 || self->awareness_level < 3 ||
         (self->shield_vitality == 1.0f && self->combat_status < 3))) {
        halo::ai::actor_set_combat_alert_flag(actor_index, 0);
    }

    platoon_flag = self->platoon_defending;
    if (self->defending != platoon_flag) {
        self->defending = platoon_flag;
        if (self->unit_index != (datum_index)k_datum_index_none) {
            halo::ai::ai_communication_broadcast((int16_t)((platoon_flag != 0) + 0x16), self->unit_index,
                                       0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0);
        }
    }

    if (self->berserking == 0 && !halo::has(static_cast<halo::tags::actor_tag_flag>(actor_definition->flags), halo::tags::actor_tag_flag::always_charge_at_enemies)) {
        self->always_charge = 0;
    } else {
        self->always_charge = 1;
    }
    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if (halo::has(static_cast<halo::tags::actor_tag_flag>(actor_definition->flags), halo::tags::actor_tag_flag::always_charge_in_attacking_mode) && self->defending == 0) {
            self->always_charge = 1;
        }
    } else {
        self->always_charge = 0;
    }

    for (threat_class = 9; threat_class > 0; threat_class--) {
        if ((int8_t)self->tally.by_threat_class[threat_class] > 0) {
            break;
        }
    }

    if (threat_class >= 8) {
        *threat_level = 2.0f;
    } else if (threat_class >= 7) {
        *threat_level = 1.8f;
    } else if (threat_class >= 6) {
        *threat_level = 1.6f;
    } else if (threat_class >= 5) {
        *threat_level = 1.2f;
    } else if (threat_class >= 3) {
        *threat_level = 0.7f;
    } else {
        *threat_level = 0.0f;
    }

    decay = (float)halo::libm::exp2(1.4426950408889634 * -0.04620981216430664);
    *threat_level_smoothed = (*threat_level - *threat_level_smoothed) * (1.0f - decay) +
                             *threat_level_smoothed;

    if (self->stood_down != 0) {
        self->stood_down_body_vitality = self->body_vitality;
    }

    if (halo::ai::flag_set(actor_definition->flags, halo::tags::actor_tag_flag::crouch_when_in_line_of_fire) ||
        halo::ai::flag_set(actor_definition->flags, halo::tags::actor_tag_flag::avoid_friends_line_of_fire)) {
        if (self->active_unit_index == (datum_index)k_datum_index_none && self->combat_status > 2) {
            combat_status = self->target_combat_status;
            *flag_35d = 0;
            *flag_35c = 0;
            *flag_35e = 0;
            *flag_35f = 0;

            if (combat_status > 8) {
                target_direction = *(real_vector3d *)&halo::ai::prop_at(self->target_unit_index)->direction;
            }
            for (prop_index = self->first_prop; prop_index != (datum_index)k_datum_index_none;
                 prop_index = p->next_in_actor) {
                p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];

                if (p->state > 1 && p->state < 4 && p->enemy == 0 && p->dead == 0 &&
                    p->swarm_owned == 0 &&
                    (p->is_parented != 0 || p->relationship_object_index == -1)) {

                    if (halo::ai::actor_get_ranged_attack_vector(prop_index, actor_index, (real_vector3d *)&cover_direction) != 0) {
                        grade = halo::ai::actor_evaluate_flank_offset(&cover_direction, &flank_offset,
                                                            &self->body_position,
                                                            &p->last_known_position);
                        if (grade >= 1) {
                            *flag_35d = 1;
                            if (p->is_parented != 0) {
                                *flag_35c = 1;
                            }
                        }

                        if ((int32_t)actor_definition->flags < 0 && p->is_parented != 0 &&
                            p->shooting != 0 &&
                            halo::math::vector3d_magnitude_squared(flank_offset) < 1.0f &&
                            (self->moving != 0 || *countdown_360 > 0)) {

                            steering_direction.i = self->desired_movement_vector.x;
                            steering_direction.j = self->desired_movement_vector.y;
                            steering_direction.k = self->desired_movement_vector.z;
                            if (halo::math::vector3d_normalize_with_length(steering_direction) > 0.0f) {
                                probe_point.x = steering_direction.i * 0.4f + self->body_position.x;
                                probe_point.y = steering_direction.j * 0.4f + self->body_position.y;
                                probe_point.z = steering_direction.k * 0.4f + self->body_position.z;
                                grade_second = halo::ai::actor_evaluate_flank_offset(&cover_direction,
                                                                           (real_vector3d *)0,
                                                                           &probe_point,
                                                                           &p->last_known_position);
                                if (grade_second <= grade) {
                                    grade_second = grade;
                                }
                                if (grade_second >= 1) {
                                    dot = flank_offset.k * steering_direction.k +
                                          flank_offset.j * steering_direction.j +
                                          flank_offset.i * steering_direction.i;
                                    if (halo::math::vector3d_magnitude_squared(flank_offset) >= 0.25f) {
                                        cosine_limit = 0.8660254f;
                                    } else {
                                        cosine_limit = 0.0f;
                                    }
                                    if (cosine_limit < dot) {
                                        *flag_35f = 1;
                                    }
                                }
                            }
                        }
                    }

                    if (combat_status > 8) {
                        grade = halo::ai::actor_evaluate_flank_offset(&target_direction, (real_vector3d *)0,
                                                            &p->last_known_position,
                                                            &self->body_position);
                        if (grade > 1) {
                            *flag_35e = 1;
                        }
                    }
                }
            }
        } else {
            *flag_35d = 0;
            *flag_35c = 0;
            *flag_35e = 0;
            *flag_35f = 0;
        }
    }

    if (*flag_35f == 0) {
        if (*countdown_360 > 0) {
            *countdown_360 = (int16_t)(*countdown_360 - 1);
        }
    } else {
        halo::ai::actor_push_recognition_entry(actor_index, self->firing_position_index, 1);
        *countdown_360 = 0x16;
    }

    if (*crouch_timer > 0) {
        *crouch_timer = (int16_t)(*crouch_timer - 1);
    } else {
        if (self->defending == 0 || self->berserking != 0) {
            threshold = actor_definition->attacking_crouch_threshold;
        } else {
            threshold = actor_definition->defending_crouch_threshold;
        }

        switch (actor_definition->defensive_crouch_type) {
        case 1:
            want_crouch = (uint8_t)(*threat_level_smoothed > threshold);
            break;
        case 2:
            want_crouch = (uint8_t)(self->shield_vitality < threshold);
            break;
        case 3:
            want_crouch = (uint8_t)(self->shield_vitality > threshold &&
                                    (int8_t)self->tally.threat_class_2 >= 1);
            break;
        case 4:
            want_crouch = (uint8_t)(self->combat_status > 0);
            break;
        case 5:
            want_crouch = halo::ai::actor_evaluate_custom_charge_trigger(actor_index);
            break;
        default:
            want_crouch = 0;
            break;
        }

        if (halo::has(static_cast<halo::tags::actor_tag_flag>(actor_definition->flags), halo::tags::actor_tag_flag::crouch_when_in_line_of_fire)) {
            if (*flag_35c != 0) {
                want_crouch = 1;
            } else if (*flag_35e != 0) {
                want_crouch = 0;
            } else if (*flag_35d != 0) {
                want_crouch = 1;
            }
        }

        if (*crouching == 0) {
            if (want_crouch != 0) {
                *crouching = 1;
                threshold = actor_definition->min_crouch_time;
                if (threshold <= 0.0f) {
                    *crouch_timer = 0x2d;
                } else {
                    *crouch_timer = (int16_t)halo::x87::__ftol((double)(threshold * 30.0f));
                }
            }
        } else if (want_crouch == 0) {
            *crouching = 0;
            threshold = actor_definition->min_stand_time;
            if (threshold <= 0.0f) {
                *crouch_timer = 0x2d;
            } else {
                *crouch_timer = (int16_t)halo::x87::__ftol((double)(threshold * 30.0f));
            }
        }
    }

    if (*countdown_368 > 0) {
        *countdown_368 = (int16_t)(*countdown_368 - 1);
    }
    halo::ai::actor_scan_allies_for_backup_request(actor_index);
}

namespace actor_update_danger_avoidance_local {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}

/**
 * Actor AI behaviour: update danger avoidance.
 *
 * @address 0x40c040
 */
uint8_t ActorView::update_danger_avoidance()
{
    using namespace actor_update_danger_avoidance_local;
    struct actor *actor = halo::ai::actor_at(actor_index);
    uint8_t result = 0;
    uint8_t crossing = 0;
    uint8_t in_danger;
    uint8_t near;
    uint8_t reacting;
    real_vector3d path_delta;
    float radius_squared;
    float danger_time;
    real_point3d *position = &actor->body_position;
    real_point3d *path_start = &actor->flee_from_point;

    if (actor->danger_type == 0 || actor->danger_reacting == 0 || actor->danger_is_own != 0) {
        return 0;
    }
    {
        float dx = actor->danger_center.x - position->x;
        float dy = actor->danger_center.y - position->y;
        float dz = actor->danger_center.z - position->z;
        float r = actor->danger_radius + 3.0f;

        if (r * r < dz * dz + dy * dy + dx * dx) {
            return 0;
        }
    }
    if (halo::ai::actor_action_has_queued_secondary(actor_index) || actor->order_committed != 0) {
        return 0;
    }

    path_delta.i = actor->danger_segment_end.x - path_start->x;
    path_delta.j = actor->danger_segment_end.y - path_start->y;
    path_delta.k = actor->danger_segment_end.z - path_start->z;
    {
        float distance_squared = halo::math::point3d_distance_squared_to_segment(*path_start, path_delta, *position);
        float radius = actor->danger_object_radius;
        float wide = radius + 3.5f;

        radius_squared = radius * radius;
        in_danger = distance_squared < radius_squared;
        near = distance_squared < wide * wide;
    }

    const bool returned_early = [&]() -> bool {
        {
            uint8_t towards;

            if (halo::ai::actor_movement_action_is_complete(actor_index) == 0) {
                towards = in_danger;
                crossing = 0;
            } else {
                towards = halo::math::point3d_distance_squared_to_segment(*path_start, path_delta, actor->path_end_point) <
                    radius_squared;
                if (actor->moving != 0 && !in_danger && !towards) {
                    real_vector3d movement;

                    movement.i = actor->desired_movement_vector.x * 3.0f;
                    movement.j = actor->desired_movement_vector.y * 3.0f;
                    movement.k = actor->desired_movement_vector.z * 3.0f;
                    if (halo::math::segment3d_distance_squared_to_segment(path_start, position, &movement, &path_delta) <
                        radius_squared) {
                        crossing = 1;
                        towards = 1;
                    } else {
                        crossing = 0;
                        return false;
                    }
                }
            }
            if (towards && actor->firing_position_index != -1) {
                halo::ai::actor_push_recognition_entry(actor_index, actor->firing_position_index, 1);
            }
        }
        if (!in_danger) {
            return false;
        }

        in_danger = 0;
        danger_time = halo::math::ray_intersect_sphere_distance(*position, *path_start, path_delta, actor->danger_object_radius);
        if (danger_time < 3.4028234663852886e+38f) {
            danger_time = danger_time * 45.0f;
        }
        reacting = 0;
        switch (actor->danger_type) {
        case 3:
            if (danger_time < 30.0f) {
                reacting = 1;
            }
            break;
        case 2:
            if (danger_time == 0.0f && actor->danger_countdown != -1 && actor->danger_countdown < 0x14) {
                in_danger = 1;
            }
            break;
        case 1:
            if (danger_time == 0.0f && actor->danger_countdown != -1 && actor->danger_countdown < 0x1e) {
                in_danger = 1;
            }
            break;
        default:
            break;
        }
        if (actor->danger_type != 3 || !reacting) {
            reacting = in_danger;
        }

        if ((actor->danger_owner_relation == 0 || reacting) && actor->danger_reported == 0) {
            int32_t reason;

            switch (actor->danger_owner_relation) {
            case 0: reason = 3; break;
            case 1: reason = 2; break;
            case 2: reason = 1; break;
            default: reason = -1; break;
            }
            if (actor->danger_type == 2) {
                halo::ai::ai_communication_broadcast(0xc, actor->unit_index, halo::k_dword_none, reason, halo::k_dword_none, halo::k_dword_none, 0);
            }
            actor->danger_reported = 1;
        }

        {
            int16_t escape = 0;
            float escape_step = 0.0f;
            uint8_t found = halo::ai::actor_find_danger_escape(actor_index, &escape, &escape_step, &path_delta, &in_danger);

            if (escape != -1 && actor->danger_dive != 0 && actor->active_unit_index == halo::k_dword_none) {
                int take = 0;

                switch (actor->danger_type) {
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
                    return false;
                }
                if (take || reacting) {
                    Actor *definition = halo::ai::tag_data<Actor>(actor->actor_definition_tag);
                    float distance = halo::ai::flag_set(definition->flags, halo::tags::actor_tag_flag::dive_off_ledges) ? 8.0f : 0.0f;

                    result = halo::ai::actor_take_danger_escape(&path_delta, actor_index, (uint16_t)escape, escape_step,
                        distance);
                    if (result) {
                        return true;
                    }
                }
            }
        }
        return false;
    }();
    if (returned_early) {
        return result;
    }

    if (!near) {
        return result;
    }
    if (actor->movement_action_complete != 0 && actor->movement_completed == 0 && actor->moving != 0 && crossing == 0) {
        return result;
    }
    if (actor->always_charge != 0) {
        return result;
    }
    {
        int16_t grade = actor_mode_definitions[actor->mode].combat_grade;

        if (grade == 1 || grade == 3) {
            uint32_t mode_data[0x21];

            mode_data[0] = 0;
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::avoid, mode_data);
            result = 1;
        }
    }
    return result;
}


namespace actor_update_facing_change_timer_local {
}

/**
 * Once the actor's "facing change pending" flag is set and its Actor tag allows a nonzero stand-facing-change
 * time, clears the flag and converts that time to a tick count. If the actor's current target prop is within 4
 * world units, raises the danger meter to at least 1.8.
 *
 * @address 0x423670
 */
void ActorView::update_facing_change_timer()
{
    using namespace actor_update_facing_change_timer_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    uint8_t *pending_flag = &self->crouch_active;
    int16_t *ticks_field = &self->crouch_ticks;
    float *smoothing_field = &self->danger_meter;

    if (*pending_flag != 0 && actor_tag->change_facing_stand_time > 0.0f) {
        *pending_flag = 0;
        *ticks_field = (int16_t)halo::x87::__ftol((double)(actor_tag->change_facing_stand_time * 30.0f));

        if (self->target_unit_index != (datum_index)k_datum_index_none) {
            prop *target = &((prop *)halo::ai::globals().prop_data->data)[self->target_unit_index & halo::k_slot_mask];
            if (target->distance < 4.0f) {
                if (*smoothing_field <= 1.8f) {
                    *smoothing_field = 1.8f;
                }
            }
        }
    }
}

namespace actor_update_flee_response_local {
}

/**
 * Tracks how long the flee condition at 0x3ec has held. While it is clear and the movement action has finished,
 * the persistence counter at 0x3e8 is reset; once the counter passes two ticks with the condition still set, the
 * flee-point resolver runs and its result is latched into unknown_505.
 *
 * @address 0x414250
 */
uint8_t ActorView::update_flee_response()
{
    using namespace actor_update_flee_response_local;
    actor *self;
    uint8_t result;

    self = halo::ai::actor_at(actor_index);

    result = 0;

    if (self->flee_source.code == 0 && self->movement_action_complete == 0) {
        self->flee_reason = 0;
    }

    if (self->flee_reason > 2 && self->flee_source.code != 0) {
        result = halo::ai::actor_resolve_flee_source_point(&self->flee_source,
            &self->forced_aim_direction, actor_index);
        if (result != 0) {
            self->forced_aim = 1;
            return result;
        }
    }

    self->forced_aim = 0;
    return result;
}

namespace actor_update_idle_stagger_local {
}

/**
 * Advances the actor's idle/boredom counter (faster -- +3 instead of +1 -- while in a boarding-ish vehicle
 * sub-state), and, once it exceeds a global per-tick cap (and is itself over 15), claims the shared per-tick
 * "stagger" slot and resets, raising needs_new_path; otherwise just tracks the highest idl
 *
 * @address 0x429430
 */
void ActorView::update_idle_stagger()
{
    using namespace actor_update_idle_stagger_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    int16_t vehicle_substate = self->mode_data.charge.stage;
    int fast = self->mode == halo::ai::actor_mode::charge &&
               (vehicle_substate == 2 || vehicle_substate == 3 || vehicle_substate == 4 || vehicle_substate == 5);

    self->idle_counter = self->idle_counter + (fast ? 3 : 1);

    if (halo::ai::globals().state->stagger_claimed == 0 && halo::ai::globals().state->stagger_threshold < self->idle_counter &&
        self->idle_counter > 15) {
        self->idle_counter = 0;
        halo::ai::globals().state->stagger_claimed = 1;
        self->needs_new_path = 1;
        return;
    }

    if (halo::ai::globals().state->stagger_highest < self->idle_counter) {
        halo::ai::globals().state->stagger_highest = self->idle_counter;
    }
    self->needs_new_path = 0;
}

namespace actor_update_look_target_local {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
static uint8_t ult_cone(real_point3d *point, real_point3d *reference, float cos_threshold)
{
    return halo::math::point3d_within_horizontal_cone(*point, *reference, cos_threshold);
}
static uint8_t ult_lane(real_point3d *point, real_point3d *forward, real_point3d *axis, float cos_threshold, float *side)
{
    return halo::ai::actor_point_in_directional_lane(point, forward, axis, cos_threshold, side);
}
}

/**
 * Actor AI behaviour: update look target.
 *
 * @address 0x415480
 */
void ActorView::update_look_target()
{
    using namespace actor_update_look_target_local;
    actor *a = halo::ai::actor_at(actor_index);
    Actor *definition = halo::ai::tag_data<Actor>(a->actor_definition_tag);
    real_point3d *cache_a = &a->desired_facing_vector;
    real_point3d *cache_b = &a->desired_aiming_vector;
    real_point3d *cache_c = &a->desired_looking_vector;
    real_point3d *unit_facing = reinterpret_cast<real_point3d *>(&a->facing);
    int16_t look_mode = a->control_animation_mode;
    uint8_t flee_look = 0;
    uint8_t aim_speed_zero;

    if (look_mode == 1) {
        *cache_b = *cache_a;
        *cache_c = *cache_a;
    } else {
        uint8_t has_weapon;
        uint8_t side_a = a->aim_unlocked;
        uint8_t look_follows = 0;
        uint8_t free_aim = 1;
        uint8_t side_b = a->look_unlocked;
        uint8_t claimed = 0;
        uint8_t in_cone = 0;
        uint8_t resolved = 0;
        uint8_t range_1, trust;
        float cos_aim = definition->cosine_maximum_aiming_deviation.yaw;
        float cos_look = definition->cosine_maximum_looking_deviation.yaw;
        float side_cos[2];
        int16_t reason;
        int16_t priority = 0;
        real_point3d flee_point;
        real_point3d voc_point;
        uint8_t section_done;
        float *range;
        actor_flee_source_reason kind2;

        if (a->vehicle_gunner != 0) {
            has_weapon = 1;
        } else if (look_mode == 0 || look_mode == 2) {
            has_weapon = halo::ai::actor_get_threat_weapon_object_index(actor_index) != k_datum_index_none;
        } else {
            has_weapon = 0;
        }
        look_follows = has_weapon;
        if (a->awareness_level == 3) {
            side_cos[0] = (float)halo::libm::cos((double)definition->combat_look_delta_l);
            side_cos[1] = (float)halo::libm::cos((double)definition->combat_look_delta_r);
        } else {
            side_cos[0] = (float)halo::libm::cos((double)definition->noncombat_look_delta_l);
            side_cos[1] = (float)halo::libm::cos((double)definition->noncombat_look_delta_r);
        }

        memset(&kind2, 0, sizeof(kind2));
        kind2.code = 2;
        if (a->firing_target_type > 0 && a->firing_state == 2 && a->abort_burst == 0 &&
            halo::ai::actor_resolve_flee_source_point(&kind2, (real_vector3d *)&flee_point, actor_index)) {
            reason = 7;
            flee_look = 1;
        } else {
            reason = (int16_t)static_cast<uint16_t>(a->flee_reason);
            if (reason != 0 && reason != 1) {
                if (halo::ai::actor_resolve_flee_source_point(&a->flee_source,
                        (real_vector3d *)&flee_point, actor_index)) {
                    flee_look = a->flee_source.code == 2;
                } else {
                    reason = 0;
                }
            }
        }

        if (a->vocalization_line >= 0 && a->vocalization_state > 0 &&
            halo::ai::actor_resolve_flee_source_point(&a->vocalization_source, (real_vector3d *)&voc_point,
                actor_index)) {
            priority = (int16_t)static_cast<uint16_t>(a->vocalization_variant);
        }
        if (a->moving != 0 &&
            actor_mode_definitions[a->mode].combat_grade == 2 && priority > 5) {
            priority = 5;
        }
        if (a->vocalization_state > 0) {
            a->vocalization_state = (int16_t)(a->vocalization_state - 1);
            if (a->vocalization_state == 0) {
                a->vocalization_line = 0;
                a->vocalization_variant = 0;
            }
        }
        a->look_claimed = 0;

        if (reason < 2) {
            section_done = resolved;
        } else {
            uint8_t commit = 0;

            if (reason >= 5 && (side_a || ult_cone(&flee_point, cache_a, cos_aim))) {
                commit = 1;
            } else if (reason >= 3 && side_b) {
                side_b = 0;
                side_a = 1;
                commit = 1;
            }
            if (commit) {
                *cache_b = flee_point;
                free_aim = 0;
                look_follows = has_weapon;
                if (reason >= 7) {
                    claimed = 1;
                    if (has_weapon) {
                        *cache_c = flee_point;
                    }
                }
            }
            if (reason == 2) {
                reason = side_a ? 5 : 0;
            }
            if (side_a) {
                *cache_a = flee_point;
                a->turn_required = (uint8_t)(a->turn_required | (reason == 4));
                side_a = 0;
                side_b = 0;
            }
            section_done = ((a->aim_unlocked == 0 && a->look_unlocked == 0) || reason >= 6) ? 1 : 0;
        }

        auto voc_claim = [&]() {
            a->look_claimed = 1;
            claimed = 0;
            free_aim = 0;
        };
        auto voc_face = [&]() {
            side_a = 0;
            look_follows = has_weapon;
            voc_claim();
        };
        auto voc_aim = [&]() {
            *cache_b = voc_point;
            *cache_c = voc_point;
            voc_face();
        };
        auto take_all = [&]() {
            if (!(a->turn_required != 0 && in_cone)) {
                *cache_a = voc_point;
                a->turn_required = 0;
            }
            voc_aim();
        };
        auto lane_or_take = [&]() {
            if (has_weapon && ult_lane(&voc_point, cache_b, cache_a, cos_look, side_cos)) {
                *cache_c = voc_point;
                look_follows = 0;
            } else if (free_aim && in_cone) {
                *cache_b = voc_point;
                *cache_c = voc_point;
                look_follows = 1;
                free_aim = 0;
            }
        };
        auto priority_gate = [&]() {
            if (priority >= 5 || (priority >= 3 && free_aim)) {
                *cache_b = voc_point;
                *cache_c = voc_point;
                look_follows = has_weapon;
                voc_claim();
                return;
            }
            lane_or_take();
        };
        auto cone_gate = [&]() {
            if (!in_cone) {
                lane_or_take();
                return;
            }
            priority_gate();
        };

        if (priority >= 2 && priority <= 6) {
            in_cone = ult_cone(&voc_point, cache_a, cos_aim);
            if (section_done) {
                if (claimed) {
                    lane_or_take();
                } else {
                    cone_gate();
                }
            } else if (claimed) {
                lane_or_take();
            } else if (priority >= 6 && halo::ai::actor_reset_queued_look_vector(actor_index)) {
                take_all();
            } else if (priority >= 5 && (side_b || a->aim_unlocked != 0)) {
                take_all();
            } else if (priority >= 4) {
                if (in_cone) {
                    priority_gate();
                } else if (!side_a || !free_aim) {
                    lane_or_take();
                } else {
                    take_all();
                }
            } else {
                cone_gate();
            }
        } else if (priority == 7 || priority == 8) {
            bool skip_voc = false;
            bool aim_only = false;

            resolved = in_cone = (uint8_t)(priority == 8);
            if (a->aim_unlocked == 0) {
                if (!ult_cone(&voc_point, cache_a, cos_aim)) {
                    if (!halo::ai::actor_reset_queued_look_vector(actor_index)) {
                        skip_voc = true;
                    } else {
                        in_cone = 1;
                    }
                } else if (!resolved) {
                    aim_only = true;
                }
            }
            if (!skip_voc) {
                if (!aim_only) {
                    *cache_a = voc_point;
                    a->turn_required = in_cone;
                }
                voc_aim();
            }
        }
        if (reason == 2 && free_aim && ult_cone(&flee_point, cache_a, cos_aim)) {
            *cache_b = flee_point;
            if (look_follows) {
                *cache_c = flee_point;
            }
            a->look_claimed = 0;
            free_aim = 0;
        }

        range = halo::ai::actor_get_idle_facing_range(actor_index);
        range_1 = range[1] > 0.0f;
        side_b = range[3] > 0.0f;
        in_cone = range[5] > 0.0f;
        bool clear_hold = true;

        if (a->look_posture > 0 && !claimed && (free_aim || look_follows) &&
            (range_1 || side_b || in_cone)) {
            resolved = 0;
            claimed = 0;
            trust = (range_1 && side_a && reason == 1 && a->idle_facing_timer == 0) ? 1 : 0;
            if (a->idle_facing_timer > 0) {
                a->idle_facing_timer -= 1;
            }
            if (a->idle_major_active != 0 && a->idle_major_is_aiming != 0 && !free_aim) {
                a->idle_major_active = 1;
                a->idle_major_timer = halo::ai::actor_look_get_wait_ticks(actor_index, 2, 1, range);
                a->idle_major_point = *cache_b;
                a->idle_major_direction_type = 4;
            }
            if (!(a->idle_major_active != 0 && a->idle_major_timer != 0)) {
                uint8_t use_aiming;
                uint8_t force = 0;
                real_point3d *direction = 0;

                if (free_aim && side_b) {
                    use_aiming = 1;
                    force = (look_follows && in_cone) ? 1 : 0;
                    direction = cache_a;
                } else {
                    use_aiming = 0;
                    if (look_follows && in_cone) {
                        direction = cache_b;
                    }
                }
                if (direction != 0) {
                    a->idle_interesting_direction = halo::ai::actor_resolve_look_target(direction, actor_index, range, trust,
                        use_aiming, force);
                    resolved = 1;
                }
            }
            bool idle_timers = false;

            if (a->idle_major_active != 0) {
                a->idle_major_timer -= 1;
                if (halo::ai::actor_resolve_flee_source_point((actor_flee_source_reason *)&a->idle_major_direction_type, (real_vector3d *)&voc_point,
                        actor_index)) {
                    if (free_aim) {
                        bool take_ab = false;

                        if (side_a && range_1 && a->flying != 0) {
                            trust = 1;
                            claimed = 1;
                            take_ab = true;
                        } else if (trust) {
                            take_ab = true;
                        }
                        if (take_ab) {
                            *cache_a = voc_point;
                            *cache_b = voc_point;
                            a->look_claimed = 1;
                            idle_timers = true;
                        } else if (ult_cone(&voc_point, cache_a, cos_aim)) {
                            *cache_b = voc_point;
                            a->look_claimed = 1;
                            idle_timers = true;
                        }
                    } else if (ult_lane(&voc_point, cache_b, cache_a, cos_look, side_cos)) {
                        *cache_c = voc_point;
                        idle_timers = true;
                    }
                }
            }
            if (!idle_timers) {
                voc_point = *cache_b;
                a->idle_major_active = 0;
            } else if (resolved && in_cone) {
                a->idle_minor_active = 1;
                a->idle_minor_timer = halo::ai::actor_look_get_wait_ticks(actor_index, 2, a->idle_interesting_direction, range);
                memcpy(&a->idle_look_direction_type, &a->idle_major_direction_type, 16);
                if (trust) {
                    a->idle_facing_timer = halo::ai::actor_look_get_wait_ticks(actor_index, 0, a->idle_interesting_direction, range);
                }
            }
            if (free_aim && ((look_follows && in_cone) || (claimed && side_b))) {
                if (a->idle_minor_timer == 0) {
                    halo::ai::actor_look_randomize_direction(actor_index, range, (real_vector3d *)&voc_point);
                }
                a->idle_minor_timer -= 1;
                if (a->idle_minor_active == 0) {
                    clear_hold = false;
                } else if (halo::ai::actor_resolve_flee_source_point((actor_flee_source_reason *)&a->idle_look_direction_type, (real_vector3d *)&flee_point,
                               actor_index) &&
                           !(claimed ? !ult_cone(&flee_point, cache_a, cos_aim)
                                     : !ult_lane(&flee_point, cache_b, cache_a, cos_look, side_cos))) {
                    if (claimed) {
                        *cache_b = flee_point;
                    }
                    *cache_c = flee_point;
                    clear_hold = false;
                }
            }
        } else {
            a->idle_major_active = 0;
            a->idle_interesting_direction = 0;
        }
        if (clear_hold) {
            a->idle_minor_active = 0;
        }
        if (a->moving == 0 && a->forced_aim == 0 && !halo::units::unit_is_in_busy_animation_state(a->unit_index) &&
            a->active_unit_index == k_datum_index_none) {
            if (ult_cone(cache_b, cache_a, cos_aim) &&
                !ult_cone(cache_b, unit_facing, cos_aim)) {
                a->turn_required = 1;
            } else if (has_weapon) {
                if (ult_lane(cache_c, cache_b, cache_a, cos_look, side_cos) &&
                    !ult_lane(cache_c, cache_b, unit_facing, cos_look, side_cos)) {
                    a->turn_required = 1;
                }
            }
        }
        if (!has_weapon) {
            *cache_c = *cache_b;
        }
    }

    if (a->flying == 0 && !(halo::libm::fabs((double)a->desired_facing_vector.z) < 9.999999747378752e-05)) {
        a->desired_facing_vector.z = 0.0f;
        if (halo::math::vector2d_normalize_with_length(*(real_vector2d *)cache_a) == 0.0f) {
            *cache_a = *unit_facing;
        }
    }
    if (a->stationary_facing_enabled != 0) {
        if (a->stationary_facing_held == 0) {
            if (a->moving == 0 &&
                a->unit_aiming_vector.k * a->desired_aiming_vector.z + a->unit_aiming_vector.j * a->desired_aiming_vector.y +
                a->unit_aiming_vector.i * a->desired_aiming_vector.x > 0.9f) {
                *reinterpret_cast<real_point3d *>(&a->oversteer_angle[1]) = *cache_a;
                a->stationary_facing_held = 1;
            }
        } else if (definition->stationary_facing_angle > 0.0f) {
            float limit = (float)halo::libm::cos((double)definition->stationary_facing_angle);
            uint8_t keep = 0;

            if (a->flying != 0) {
                keep = a->desired_facing_vector.z * a->oversteer_angle[3] + a->desired_facing_vector.y * a->oversteer_angle[2] +
                       a->desired_facing_vector.x * a->oversteer_angle[1] > limit &&
                       a->oversteer_angle[3] * a->desired_aiming_vector.z + a->oversteer_angle[2] * a->desired_aiming_vector.y +
                       a->desired_aiming_vector.x * a->oversteer_angle[1] > limit;
            } else {
                real_vector2d aim2, face2, hold2;

                aim2.i = a->desired_aiming_vector.x;
                aim2.j = a->desired_aiming_vector.y;
                face2.i = a->desired_facing_vector.x;
                face2.j = a->desired_facing_vector.y;
                hold2.i = a->oversteer_angle[1];
                hold2.j = a->oversteer_angle[2];
                if (halo::math::vector2d_normalize_with_length(face2) != 0.0f && halo::math::vector2d_normalize_with_length(aim2) != 0.0f &&
                    halo::math::vector2d_normalize_with_length(hold2) != 0.0f) {
                    keep = hold2.j * face2.j + hold2.i * face2.i > limit && aim2.j * hold2.j + aim2.i * hold2.i > limit;
                }
            }
            if (!keep) {
                a->stationary_facing_held = 0;
                halo::ai::actor_update_facing_change_timer(actor_index);
            }
        }
    } else {
        a->stationary_facing_held = 0;
    }

    *reinterpret_cast<real_point3d *>(&a->snapshot_facing) = *cache_a;
    *reinterpret_cast<real_point3d *>(&a->aiming_vector_snapshot) = *cache_b;
    *reinterpret_cast<real_point3d *>(&a->looking_vector_snapshot) = *cache_c;
    if (a->turn_required != 0) {
        a->control_flags |= halo::units::to_bits(halo::units::unit_control_flag::exact_facing);
    } else {
        a->control_flags &= ~halo::units::to_bits(halo::units::unit_control_flag::exact_facing);
    }

    aim_speed_zero = 1;
    if (!flee_look && a->look_posture != 4) {
        switch (a->vocalization_line) {
        case 3: case 6: case 10: case 11: case 12:
            break;
        default:
            aim_speed_zero = 0;
            break;
        }
    }
    a->control_aiming_speed = aim_speed_zero ? 0 : 1;
}


namespace actor_update_melee_combat_action_local {
static auto &actor_type_procs = halo::link::ref<actor_type_table_entry *[16]>(halo::ai::vars().actor_type_procs);
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
static uint8_t actor_combat_commit_position(datum_index actor_index, actor *a, prop *target, int16_t position)
{
    datum_index object = target != 0 ? (uint32_t)target->last_perceived_time : k_datum_index_none;

    if (a->encounter_index != k_datum_index_none &&
        halo::ai::ai_pursuit_note_object(actor_index, a->encounter_index, position, (int32_t)object)) {
        if (a->pursuit_position_count == 0) {
            halo::ai::ai_communication_broadcast(0x10, a->unit_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
        }
        a->pursuit_position_count += 1;
    }
    return 1;
}
}

/**
 * Actor AI behaviour: update melee combat action.
 *
 * @address 0x40cdf0
 */
uint8_t ActorView::update_melee_combat_action()
{
    using namespace actor_update_melee_combat_action_local;
    actor *a = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(a->actor_definition_tag);
    datum_index encounter_index = a->encounter_index;
    struct encounter *encounter = encounter_index != k_datum_index_none ? halo::ai::encounter_at(encounter_index) : 0;
    uint8_t result = 0;
    uint8_t regroup = 0;
    uint8_t searching = 0;
    uint8_t order[0x8c];

    memset(order, 0, sizeof(order));
    auto guard = [&]() -> uint8_t {
        if (actor_mode_definitions[a->mode].combat_grade == 1 ) {
            return result;
        }
        {
            int16_t mode = a->mode;
            int16_t guard_at = ((mode == 7 && !a->mode_data.search.unknown_01) || mode == 8) ? 0 : 0x5a;

            halo::ai::actor_set_target_alert_stage3(a->target_unit_index, actor_index);
            halo::ai::actor_build_order_guard(actor_index, (actor_order *)order, guard_at);
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::guard, order);
        }
        return 1;
    };
    if (encounter != 0 && encounter->stood_down && a->combat_status <= 2 && a->minimum_combat_status == 0 && a->suspicion_status == 0) {
        regroup = 1;
    }
    if (a->post_combat_action > 0 && a->combat_status <= 2 && a->suspicion_status == 0) {
        searching = 1;
    }
    if (a->awareness_level < 3 && actor_mode_definitions[a->mode].combat_grade == 0 ) {
        return 1;
    }
    if (a->order_committed || searching || regroup) {
        if (searching) {
            if (a->mode == halo::ai::actor_mode::guard && a->mode_data.guard.command_pending) {
                return 1;
            }
            if (halo::ai::actor_build_order_search_wait(actor_index, (actor_order *)order)) {
                halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::guard, order);
                return 1;
            }
        }
        if (regroup) {
            result = halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
            if (result) {
                return result;
            }
        }
        return guard();
    }
    if (a->combat_status < 2) {
        return guard();
    }

    {
        prop *target = a->target_unit_index != k_datum_index_none ? halo::ai::prop_at(a->target_unit_index) : 0;
        uint8_t hold = 0;
        uint8_t advance = 0;
        uint8_t pressed = 0;
        uint8_t retreat = 0;
        uint8_t reposition = 0;
        uint8_t move_ok = 0;
        uint8_t wait_ok = 0;

        if (target == 0 || !target->noticed_c) {
            actor_type_table_entry *type = actor_type_procs[a->type];
            int16_t ax_mode = type->ax_mode;
            int16_t cx_mode = type->cx_mode;
            int16_t mode_b = type->mode_b;
            uint8_t phase = type->swarm;
            int16_t support_mode = 0;
            uint8_t reachable_b = 0;

            regroup = 0;
            if (target != 0) {
                pressed = 1;
                advance = 1;
            }
            reposition = 1;
            retreat = 1;
            move_ok = 1;
            if (encounter_index != k_datum_index_none) {
                halo::ai::ai_starting_location_derive_placement_flags(encounter_index, (int16_t)static_cast<uint16_t>(a->squad_index), &hold,
                                                            &support_mode, &phase, &ax_mode, &mode_b, &cx_mode);
                if (a->swarm) {
                    retreat = 1;
                    pressed = 1;
                    advance = 1;
                    move_ok = 1;
                    reposition = 1;
                } else {
                    halo::ai::encounter_evaluate_support_needs(encounter_index, actor_index, support_mode, phase, &pressed,
                                                     &retreat, &reposition, &move_ok, &regroup, &reachable_b, &wait_ok);
                }
            }
            halo::ai::actor_get_target_state_flags(ax_mode, cx_mode, regroup, actor_index, mode_b, 0, (char)a->always_charge, &advance,
                                         (char *)&pressed, &retreat, &reposition, &move_ok, &wait_ok);
        }

        if (a->pursuit_target_prop_index != a->target_unit_index) {
            a->pursuit_position_count = 0;
            a->pursuit_target_prop_index = a->target_unit_index;
            a->target_lost = 0;
            a->target_lost_reported = 0;
        }
        if (advance && halo::ai::actor_build_order_wait_byte(actor_index, retreat, (uint32_t *)order)) {
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::uncover, order);
            return 1;
        }
        halo::ai::actor_set_target_alert_stage1(a->target_unit_index, actor_index);
        if (retreat && halo::ai::actor_build_order_flee(actor_index, a->always_charge, (uint32_t *)order)) {
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::search, order);
            return 1;
        }
        halo::ai::actor_set_target_alert_stage2(a->target_unit_index, actor_index);
        if (a->target_lost && !a->target_lost_reported) {
            halo::ai::ai_communication_broadcast(0xd, a->unit_index, halo::ai::actor_get_target_prop_object_index(actor_index), -1,
                                       k_datum_index_none, k_datum_index_none, 0);
            a->target_lost_reported = 1;
        }

        if (reposition) {
            int16_t position = -1;
            uint8_t have_position = 0;
            bool no_position = false;

            a->search_firing_positions = 1;
            if (a->swarm) {
                if (move_ok && halo::ai::actor_build_order_minimal_stop(actor_index, (uint32_t *)order)) {
                    halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::search, order);
                    return actor_combat_commit_position(actor_index, a, target, position);
                }
            } else {
                int16_t limit;

                if (a->mode == halo::ai::actor_mode::uncover && move_ok && a->mode_data.uncover.stage == 1) {
                    position = a->mode_data.uncover.firing_position;
                    have_position = 1;
                }
                if (!(have_position && position != -1)) {
                    limit = a->nearby_friend_prop_index == k_datum_index_none ? static_cast<int16_t>(actor_tag->num_positions__normal_) : static_cast<int16_t>(actor_tag->num_positions__coord_);
                    if (!hold && a->pursuit_target_prop_index == a->target_unit_index && a->pursuit_position_count >= limit) {
                        no_position = true;
                    }
                    if (!no_position) {
                        {
                            static actor_firing_position_query query;
                            static actor_firing_position_candidate candidate;
                            static path_find_context path_context;
                            uint32_t previous_owner = 0;
                            uint8_t path_ok = 0;

                            memset(&query, 0, sizeof(query));
                            query.goal_kind = 5;
                            query.pursuit_target_index = a->target_unit_index;
                            query.pursuit_last_perceived_time = target != 0 ? (uint32_t)target->last_perceived_time : k_datum_index_none;
                            query.want_direction_from_target = (uint8_t)(a->target_unit_index != k_datum_index_none);
                            query.collect_all = hold;
                            query.group_mask = halo::ai::actor_get_firing_position_group_mask(actor_index, 5, 0);
                            query.search_radius = 20.0f;
                            position = (int16_t)halo::ai::actor_find_best_firing_position(actor_index, &query,
                                &candidate, &previous_owner,
                                &path_context, &path_ok);
                        }
                    }

                    if (!no_position && position == -1) {
                        no_position = true;
                    }
                    if (!no_position && !have_position &&
                        halo::ai::actor_build_order_face_seat_marker(actor_index, position, (uint32_t *)order)) {
                        halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::uncover, order);
                        return actor_combat_commit_position(actor_index, a, target, position);
                    }
                }
                if (!no_position && move_ok &&
                    halo::ai::actor_build_order_face_seat_marker_committed(actor_index, position, hold, (uint32_t *)order)) {
                    halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::search, order);
                    return actor_combat_commit_position(actor_index, a, target, position);
                }
            }
        }

        if (a->pursuit_position_count > 0 && a->unit_index != k_datum_index_none) {
            halo::ai::ai_communication_broadcast(0x13, a->unit_index, k_datum_index_none, -1, k_datum_index_none,
                                       k_datum_index_none, 0);
        }
        if (!a->swarm && wait_ok && halo::ai::actor_build_order_random_wait(actor_index, reposition, (uint32_t *)order)) {
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::wait, order);
            return 1;
        }
    }

    return guard();
}


namespace actor_update_movement_destination_local {
static real_point3d *actor_held_firing_position(struct actor *actor)
{
    ScenarioEncounter *encounter = &halo::ai::reflexive_data<ScenarioEncounter>(halo::scenario::globals().scenario->encounters)[actor->encounter_index & halo::k_slot_mask];

    return (real_point3d *)&halo::ai::reflexive_data<ScenarioFiringPosition>(encounter->firing_positions)[actor->firing_position_index];
}
}

/**
 * Actor AI behaviour: update movement destination.
 *
 * @address 0x403180
 */
uint8_t ActorView::update_movement_destination()
{
    using namespace actor_update_movement_destination_local;
    struct actor *actor = halo::ai::actor_at(actor_index);
    Actor *actor_tag;
    ActorVariant *definition;

    if (actor->needs_new_path == 0) {
        return 0;
    }
    actor_tag = halo::ai::tag_data<Actor>(actor->actor_definition_tag);
    definition = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);

    if (actor->order_committed == 0) {
        uint8_t follow_lead = 0;

        if (actor->crouch_active != 0 && (static_cast<uint8_t>(actor_tag->flags) & 0x20) != 0) {
            halo::ai::actor_update_target_lead_position(actor_index);
            follow_lead = halo::ai::actor_firing_position_near_point(actor_index, &actor->pathfinding_point,
                (int32_t)(uint32_t)actor->pathfinding_surface_index, 0);
        }
        if (follow_lead) {
            uint8_t at_position = 0;
            uint8_t drop = 0;

            if (actor->encounter_index != halo::k_dword_none && actor->firing_position_index != -1) {
                float radius = halo::ai::actor_compute_accuracy_scale(actor_index);

                if (radius * radius > halo::math::vector3d_distance_squared(*actor_held_firing_position(actor),
                        actor->body_position)) {
                    at_position = 1;
                }
            }
            if (actor->moving != 0 && actor->tally.threat_class_5 == 0) {
                if (actor->target_unit_index != halo::k_dword_none) {
                    prop *target = halo::ai::prop_at(actor->target_unit_index);

                    drop = target->distance < definition->desired_combat_range[1];
                }
            } else {
                drop = !at_position;
            }
            if (drop) {
                actor->firing_position_index = -1;
                halo::ai::actor_movement_action_stop(actor_index);
            }
        } else {
            static actor_firing_position_query query;
            static path_find_context path_context;
            actor_firing_position_candidate candidate;
            uint32_t previous_owner = halo::k_dword_none;
            uint8_t path_ok = 0;
            int16_t previous = actor->firing_position_index;
            int16_t selected;
            int16_t claimed;

            memset(&query, 0, sizeof(query));
            memset(&candidate, 0, sizeof(candidate));
            selected = halo::ai::actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
                &path_ok);
            claimed = halo::ai::actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok);
            actor = halo::ai::actor_at(actor_index);
            if (claimed == -1) {
                actor->mode_data.fight.position_hold_countdown = 0;
            } else if (claimed != previous) {
                float wait = halo::math::random_real_range(actor_tag->combat_position_time[0], actor_tag->combat_position_time[1]);

                if (actor->vehicle_driving_type > 0) {
                    object *vehicle = (object *)halo::ai::object_at(actor->active_unit_index);
                    Vehicle *vehicle_tag = halo::ai::tag_data<Vehicle>(vehicle->definition_tag);
                    float cap = vehicle_tag->ai_move_position_time;

                    if (cap > 0.0f && wait > cap) {
                        wait = cap;
                    }
                }
                actor->mode_data.fight.position_hold_countdown = (int16_t)(int32_t)(wait * halo::game::k_ticks_per_second_f);
            }
        }
    }

    if (actor->target_combat_status < 7) {
        return 0;
    }
    {
        prop *target = halo::ai::prop_at(actor->target_unit_index);
        uint8_t engaged = 1;

        if (halo::ai::actor_has_unshielded_threat_weapon(actor_index)) {
            if (target->distance < actor->maximum_firing_distance) {
                engaged = 0;
            } else if (actor->encounter_index != halo::k_dword_none && actor->firing_position_index != -1) {
                real_point3d *held = actor_held_firing_position(actor);
                float radius = halo::ai::actor_compute_accuracy_scale(actor_index);

                if (radius * radius < halo::math::vector3d_distance_squared(*held, actor->body_position)) {
                    float range = actor->maximum_firing_distance;

                    if (range * range > halo::math::vector3d_distance_squared(target->last_known_position, *held)) {
                        engaged = 0;
                    }
                }
            }
        }
        halo::ai::actor_target_mark_engaged(actor->target_unit_index, actor_index, engaged);
    }
    return 0;
}


namespace actor_update_path_if_needed_local {
}

/**
 * Actor AI behaviour: update path if needed.
 *
 * @address 0x4017b0
 */
uint8_t ActorView::update_path_if_needed()
{
    using namespace actor_update_path_if_needed_local;
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->needs_new_path != 0) {
        static actor_firing_position_query query;
        static path_find_context path_context;
        actor_firing_position_candidate candidate;
        uint32_t previous_owner = halo::k_dword_none;
        uint8_t path_ok = 0;
        int16_t selected;

        memset(&query, 0, sizeof(query));
        memset(&candidate, 0, sizeof(candidate));
        query.goal_kind = 6;
        selected = halo::ai::actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
            &path_ok);
        halo::ai::actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok);
        actor = halo::ai::actor_at(actor_index);
    }
    return actor->danger_type == 0;
}

namespace actor_update_squad_link_state_local {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}

/**
 * Per-tick housekeeping for an actor's link to its squad/encounter: releases a swarm actor that has lost its
 * swarm outright; ages a couple of timers; reactivates the actor's units when it or its encounter requests it;
 * otherwise, once its current target has become invalid/stale for long enough (or afte
 *
 * @address 0x429270
 */
uint8_t ActorView::update_squad_link_state()
{
    using namespace actor_update_squad_link_state_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    encounter *enc = 0;
    uint8_t combined_flag;

    if (self->swarm != 0 && self->swarm_index == (datum_index)k_datum_index_none) {
        halo::ai::actor_delete_or_release_unit(actor_index, 0);
        return 0;
    }

    self->path_resolved_this_tick = 0;

    if (self->suspicion_timer > 0) {
        self->suspicion_timer = self->suspicion_timer - 1;
        if (self->suspicion_timer == 0) {
            self->suspicion_status = 0;
        }
    }
    if (self->command_list_delay > 0) {
        self->command_list_delay = self->command_list_delay - 1;
    }

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        enc = &((encounter *)halo::ai::globals().encounter_data->data)[self->encounter_index & halo::k_slot_mask];
    }

    combined_flag = self->force_active;
    if (enc != 0) {
        combined_flag |= enc->force_active;
    }

    if (self->can_go_dormant == 0 || combined_flag != 0) {
        halo::ai::actor_set_units_active(actor_index, 0);
    } else if (self->keep_unit_alive == 0) {
        int16_t combat_grade = actor_mode_definitions[self->mode].combat_grade;
        int stale = 1;

        if (combat_grade != 2) {
            if (self->target_unit_index != (datum_index)k_datum_index_none) {
                prop *target = &((prop *)halo::ai::globals().prop_data->data)[self->target_unit_index & halo::k_slot_mask];
                if (target->is_parented != 0 && target->enemy != 0 && target->dead == 0) {
                    int16_t kind = target->state;
                    if ((kind >= 2 && kind <= 3) || (kind >= 4 && kind <= 5 && combat_grade == 3)) {
                        stale = 0;
                    }
                }
            }
        } else {
            stale = 0;
        }

        if (stale) {
            uint8_t movement_done = self->movement_action_complete;
            if (movement_done != 0) {
                if (self->active_movement.type == 3) {
                    if (self->mode == halo::ai::actor_mode::guard && enc != 0 && enc->follow_target_type == 1) {
                        return 1;
                    }
                } else if (self->active_movement.type == 5) {
                    prop *p = &((prop *)halo::ai::globals().prop_data->data)[halo::bit_cast<datum_index>(self->active_movement.destination.x) & halo::k_slot_mask];
                    if (p->is_parented != 0) {
                        return 1;
                    }
                }
            }
            self->inactive_ticks = self->inactive_ticks + 1;
            if (self->inactive_ticks > 0x3b) {
                halo::ai::actor_set_units_active(actor_index, 1);
                return 1;
            }
        }
    }
    return 1;
}

namespace actor_update_swarm_component_position_local {
}

/**
 * Given a swarm-slot index and a unit index, records either that unit's ground/death position marker (when it is
 * a biped) or none into the shared swarm-member position cache, alongside the unit's current position.
 *
 * @address 0x428130
 */
void ActorOps::update_swarm_component_position(datum_index component_index, datum_index unit_index)
{
    using namespace actor_update_swarm_component_position_local;
    object *unit_object = halo::ai::object_at(unit_index);
    swarm_component *component = &((swarm_component *)halo::ai::globals().swarm_component_data->data)[component_index & halo::k_slot_mask];
    datum_index marker;

    marker = (unit_object->type == 0) ? ((biped_object *)unit_object)->biped.ground_surface_index : (datum_index)k_datum_index_none;

    halo::objects::object_get_position(&component->position, unit_index);
    component->marker_index = marker;
}

namespace actor_update_target_combat_status_local {
}

/**
 * Recomputes the actor's cached combat-status code and aim/visibility flag for its currently selected target
 * prop.
 *
 * @address 0x4200d0
 */
void ActorView::update_target_combat_status()
{
    using namespace actor_update_target_combat_status_local;
    actor *self;
    prop *target;
    object *target_obj;
    int16_t status;

    self = halo::ai::actor_at(actor_index);

    if (self->target_unit_index == k_datum_index_none) {
        self->target_combat_status = 0;
        self->target_last_seen_time = k_datum_index_none;
        self->target_alive = 0;
        return;
    }

    target = halo::ai::prop_at(self->target_unit_index);
    target_obj = halo::ai::object_at(target->object_index);

    switch (target->state) {
    case 0:
        status = 0;
        self->target_unit_index = k_datum_index_none;
        self->target_last_seen_time = k_datum_index_none;
        break;
    case 1:
        status = 1;
        break;
    case 2:
    case 3:
        if (target->dead != 0) {
            status = 2;
        } else if (target->seen != 0) {
            status = 0xb;
        } else if (2 <= target->visual_perception) {
            status = 10;
        } else if (target->obstruction != 0 && target->obstruction != 1) {
            status = 7;
        } else if (2 < (int8_t)target->aiming_at_actor_class || !(target->distance < 6.0f)) {
            status = 8;
        } else {
            status = 9;
        }
        break;
    case 4:
        status = (int16_t)((target->has_current_information != 0) + 5);
        break;
    case 5:
        if (target->dead != 0) {
            status = 2;
        } else {
            status = (int16_t)(4 - (target->noticed_c != 0));
        }
        break;
    }

    self->target_combat_status = status;

    if (target->state < 2 || 3 < target->state) {
        self->target_alive = (uint8_t)(~(target_obj->vitality_flags >> 2) & 1);
    } else {
        self->target_alive = (uint8_t)(target->dead == 0);
        if (0 < target->visual_perception) {
            self->target_last_seen_time = target->last_seen_time;
        }
    }
}

namespace actor_update_target_lead_position_local {
}

/**
 * REWRITTEN from objdump 0x429570..0x429610. EAX: actor. When the cached location (+0x164) is unset, the point
 * the actor rides (+0x158, when its seat kind +0x15e is 2..3; 0x571de0) or from its own biped (0x55ab30), both
 * refining the point in place. The draft called object_try_and_get and 0x571de0 with
 *
 * @address 0x429570
 */
void ActorView::update_target_lead_position()
{
    using namespace actor_update_target_lead_position_local;
    actor *a = halo::ai::actor_at(actor_index);
    real_point3d *point = &a->pathfinding_point;
    datum_index vehicle;

    if (a->pathfinding_surface_index != -1) {
        return;
    }
    *point = *(real_point3d *)&a->body_position.x;
    if (a->flying) {
        return;
    }
    vehicle = a->active_unit_index;
    if (vehicle != k_datum_index_none) {
        int16_t seat_kind = a->vehicle_driving_type;

        if (seat_kind >= 2 && seat_kind <= 3) {
            a->pathfinding_surface_index = halo::units::unit_predict_aim_target_position(vehicle, point);
        }
        return;
    }
    if (halo::objects::object_try_and_get(a->unit_index, 1) != 0) {
        a->pathfinding_surface_index = (int32_t)halo::units::biped_get_cached_look_at_position(a->unit_index, point);
    }
}

}
