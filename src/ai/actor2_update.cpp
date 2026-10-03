#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

namespace halo::ai {

namespace actor_update_activation_state_local {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
extern uint8_t actor_update_squad_link_state(datum_index actor_index);
extern void actor_update_idle_stagger(datum_index actor_index);
extern void actor_refresh_combat_context(datum_index actor_index);
extern void actor_target_relationship_think(datum_index actor_index);
extern void actor_choose_best_target(datum_index actor_index);
extern void actor_update_crouch_state(datum_index actor_index);
extern void actor_run_mode_transition_loop(datum_index actor_index);
extern void actor_dispatch_type_vtable_0x18(datum_index actor_index);
extern void actor_snapshot_orientation(datum_index actor_index);
extern void actor_invoke_type_handler(uint32_t actor_index);
extern void actor_update_grenade_eligibility_state(datum_index actor_index);
extern void actor_schedule_grenade_throw(uint32_t actor_index);
extern void actor_movement_advance_waypoint(datum_index actor_index);
extern void actor_update_flee_response(datum_index actor_index);
extern void actor_movement_update(datum_index actor_index);
extern void actor_update_look_target(datum_index actor_index);
extern void actor_update_firing_state(datum_index actor_index);
extern void actor_apply_queued_look_to_unit(datum_index actor_index);
}
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
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (actor_update_squad_link_state(actor_index) == 0) {
        return;
    }

    actor_update_idle_stagger(actor_index);
    actor_refresh_combat_context(actor_index);
    actor_target_relationship_think(actor_index);
    actor_choose_best_target(actor_index);
    actor_update_crouch_state(actor_index);

    memset(&self->flee_reason, 0, 0x21 * sizeof(uint32_t));
    self->secondary_action = -1;
    self->movement_style_override = -1;
    self->strafe_axis_override = -1;

    actor_run_mode_transition_loop(actor_index);

    {
        uint32_t proc = actor_mode_definitions[self->mode].tick_proc;
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }

    if (self->keep_unit_alive == 0) {
        if (self->swarm != 0) {
            actor_dispatch_type_vtable_0x18(actor_index);
            return;
        }
        actor_snapshot_orientation(actor_index);
        actor_invoke_type_handler(actor_index);
        actor_update_grenade_eligibility_state(actor_index);
        actor_schedule_grenade_throw(actor_index);
        actor_movement_advance_waypoint(actor_index);
        actor_update_flee_response(actor_index);
        actor_movement_update(actor_index);
        actor_update_look_target(actor_index);
        actor_update_firing_state(actor_index);
        actor_apply_queued_look_to_unit(actor_index);
    }
}

namespace actor_update_aim_wander_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern double fcos(double x);
extern double fsin(double x);
extern double ftan(double x);
extern int32_t fistp_round(float x);
extern void *actor_get_actor_definition(datum_index actor_index);
extern uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind);
extern void actor_choose_random_point_near(real_point3d *inout_point, float radius);
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b);
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
extern float weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire);
static float aim_wander_random_fraction(void)
{
    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    return (float)(int32_t)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
}
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
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *variant = (uint8_t *)actor_get_actor_definition(actor_index);
    int16_t team = ((actor *)a)->team;
    uint8_t *burst = 0;
    uint8_t *scale = 0;
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

    if (a[0x604] != 0 && !actor_target_is_visible_or_object_count_ok(actor_index,
            (int16_t)*(uint16_t *)&((ActorVariant *)variant)->special_fire_situation)) {
        a[0x604] = 0;
    }
    a[0x603] = a[0x604];
    a[0x604] = 0;

    if (((actor *)a)->active_unit_index == k_datum_index_none) {
        moving = (a[0x15c] != 0 || a[0x504] != 0) ? 1 : 0;
    } else {
        uint8_t *vehicle = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[((actor *)a)->active_unit_index & 0xffff].data;
        real_vector3d *velocity = (real_vector3d *)(vehicle + 0x68);

        moving = (velocity->i * velocity->i + velocity->j * velocity->j + velocity->k * velocity->k > 1.0f) ? 1 : 0;
    }
    a[0x601] = moving;
    a[0x600] = (weapon_get_zoom_fov_resolved(0xd, team) * ((ActorVariant *)variant)->new_target_firing_pattern_time * 30.0f >
        (float)((struct actor *)a)->firing_target_ticks) ? 1 : 0;

    actor_select_stance_offset_pair(actor_index, variant, &burst, &scale);

    if (((struct actor *)a)->burst_duration_override > 0.0f) {
        time = ((struct actor *)a)->burst_duration_override;
    } else {
        time = aim_wander_random_fraction() * (*(float *)(burst + 0x18) - *(float *)(burst + 0x14)) +
            *(float *)(burst + 0x14);
        if (scale != 0 && *(float *)(scale + 0x0) != 0.0f) {
            time = time * *(float *)(scale + 0x0);
        }
        if (a[0x1ca] != 0) {
            time = time * 0.6f;
        }
    }
    ((struct actor *)a)->firing_state_timer = (int16_t)(int32_t)(time * 30.0f);

    error = weapon_get_zoom_fov_resolved(0xb, team) * ((ActorVariant *)variant)->projectile_error;
    if (scale != 0 && *(float *)(scale + 0xc) != 0.0f) {
        error = error * *(float *)(scale + 0xc);
    }
    if (a[0x1ca] != 0) {
        error = error + error + 0.017453292f;
    }
    ((struct actor *)a)->projectile_error = error;

    ((actor *)a)->perception_scale = 0.0f;
    if (((ActorVariant *)variant)->weapon_damage_modifier > 0.0f) {
        ((actor *)a)->perception_scale = ((ActorVariant *)variant)->weapon_damage_modifier;
    } else if (((ActorVariant *)variant)->damage_per_second > 0.0f) {
        datum_index weapon = actor_get_threat_weapon_object_index(actor_index);

        if (weapon != k_datum_index_none) {
            float rate;
            float damage = halo::items::weapon_trigger_get_average_damage(
                *(datum_index *)((object_header *)halo::objects::globals().object_data->data)[weapon & 0xffff].data, &rate);

            if (((ActorVariant *)variant)->rate_of_fire > 0.0f && rate > ((ActorVariant *)variant)->rate_of_fire) {
                rate = ((ActorVariant *)variant)->rate_of_fire;
            }
            damage = damage * rate;
            if (damage > 0.0f) {
                ((actor *)a)->perception_scale = ((ActorVariant *)variant)->damage_per_second / damage;
            }
        }
    }
    if (a[0x603] != 0 || a[0x602] != 0) {
        if (((ActorVariant *)variant)->special_damage_modifier > 0.0f) {
            ((actor *)a)->perception_scale = ((actor *)a)->perception_scale * ((ActorVariant *)variant)->special_damage_modifier;
        }
        ((struct actor *)a)->projectile_error = ((ActorVariant *)variant)->special_projectile_error + ((struct actor *)a)->projectile_error;
    }

    if (((ActorVariant *)variant)->bombardment_range > 0.0f && ((struct actor *)a)->firing_target_type == 1) {
        uint8_t *prop = (uint8_t *)prop_data->data + (((struct actor *)a)->firing_target_prop_index & 0xffff) * 0x138;
        int16_t kind = ((struct prop *)prop)->state;

        bombard = (kind < 2 || kind > 3 || ((struct prop *)prop)->visual_perception == 0) ? 1 : 0;
    }
    target = ((actor *)a)->firing_target_point;
    if (bombard) {
        actor_choose_random_point_near(&target, ((ActorVariant *)variant)->bombardment_range);
    }
    {
        float dx = target.x - ((actor *)a)->aim_origin.x;
        float dy = target.y - ((actor *)a)->aim_origin.y;
        float dz = (target.z - ((actor *)a)->aim_origin.z) * 0.0f;

        side.i = dy - dz;
        side.j = dz - dx;
        side.k = dx * 0.0f - dy * 0.0f;
    }
    halo::math::vector3d_normalize_with_length(side);
    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    if ((uint16_t)(halo::math::globals().random_seed_global >> 16) > 0x8000) {
        side.i = -side.i;
        side.j = -side.j;
        side.k = -side.k;
    }

    angle_1 = aim_wander_random_fraction() * (*(float *)(burst + 0x4) + *(float *)(burst + 0x4)) - *(float *)(burst + 0x4);
    angle_2 = aim_wander_random_fraction() * (*(float *)(burst + 0x10) + *(float *)(burst + 0x10)) -
        *(float *)(burst + 0x10) + angle_1;
    radius_a = weapon_get_zoom_fov_resolved(0xc, team) * *(float *)(burst + 0x0);
    radius_b = aim_wander_random_fraction() * (*(float *)(burst + 0xc) - *(float *)(burst + 0x8)) + *(float *)(burst + 0x8);
    radius_b = weapon_get_zoom_fov_resolved(0xc, team) * radius_b;
    if (a[0x1ca] != 0) {
        radius_a = radius_a + radius_a;
        radius_b = radius_b + radius_b;
    }

    if (((struct actor *)a)->firing_state_timer > 0 && *(float *)(burst + 0x24) > 0.0f) {
        float ticks = (float)(int32_t)((struct actor *)a)->firing_state_timer;
        float sweep = ticks * *(float *)(burst + 0x24) * 0.033333335f;
        float limit;

        if (!(sweep <= 0.7853982f)) {
            sweep = 0.7853982f;
        }
        limit = (float)ftan((double)sweep) * ((actor *)a)->firing_target_distance;
        if (radius_a > limit) {
            float limit_15 = limit * 1.5f;

            if (radius_a >= limit_15) {
                ((struct actor *)a)->firing_state_timer = (int16_t)fistp_round(ticks * 1.5f);
                radius_b = limit_15 / radius_a * radius_b;
                radius_a = limit_15;
            } else {
                ((struct actor *)a)->firing_state_timer = (int16_t)fistp_round(radius_a / limit * ticks);
            }
        }
    }

    {
        float c1 = (float)fcos((double)angle_1), s1 = (float)fsin((double)angle_1);
        float c2 = (float)fcos((double)angle_2), s2 = (float)fsin((double)angle_2);

        wander.i = (side.i * c1 + 0.0f * s1) * radius_a;
        wander.j = (side.j * c1 + 0.0f * s1) * radius_a;
        wander.k = (side.k * c1 + s1) * radius_a;
        recoil.i = -((side.i * c2 + s2 * 0.0f) * radius_b);
        recoil.j = -((side.j * c2 + s2 * 0.0f) * radius_b);
        recoil.k = -((side.k * c2 + s2) * radius_b);
    }
    if (((struct actor *)a)->firing_state_timer > 0) {
        float per_tick = 1.0f / (float)(int32_t)((struct actor *)a)->firing_state_timer;

        recoil.i = recoil.i * per_tick;
        recoil.j = recoil.j * per_tick;
        recoil.k = recoil.k * per_tick;
    }
    ((actor *)a)->aim_target_point = target;
    ((actor *)a)->aim_wander_offset = wander;
    ((actor *)a)->aim_recoil_per_tick = recoil;
    ((actor *)a)->grenade_aim_direction.i = wander.i + target.x;
    ((actor *)a)->grenade_aim_direction.j = wander.j + target.y;
    ((actor *)a)->grenade_aim_direction.k = wander.k + target.z;

    if (((struct actor *)a)->combat_status >= 7) {
        uint8_t prop_flag = 0;
        datum_index object = k_datum_index_none;
        int32_t code;

        if (((struct actor *)a)->firing_target_type == 1) {
            uint8_t *prop = (uint8_t *)prop_data->data + (((struct actor *)a)->firing_target_prop_index & 0xffff) * 0x138;

            prop_flag = prop[0x61];
            object = ((struct prop *)prop)->object_index;
        }
        if (a[0x378] != 0) {
            code = 0x1c;
        } else if (prop_flag) {
            code = 0x1e;
        } else if ((int8_t)a[0x1f8] >= 5) {
            code = 0x1d;
        } else {
            code = 0x1a + (a[0x161] != 0);
        }
        ai_communication_broadcast(code, ((actor *)a)->unit_index, object, 3, k_datum_index_none, k_datum_index_none, 0);
    }
}

namespace actor_update_awareness_level_local {
extern "C" {
extern data_array *actor_data;
extern int16_t actor_combat_status_min_grade[];
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

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

    if (new_grade == 0) {
        self->ticks_alerted = 0;
    } else {
        self->ticks_alerted = self->ticks_alerted + 1;
        if (3 < new_grade) {
            self->ticks_threatened = self->ticks_threatened + 1;
            self->ticks_since_threatened = 0;
            goto have_streaks;
        }
    }
    self->ticks_threatened = 0;
    if (self->ticks_since_threatened != -1) {
        self->ticks_since_threatened = self->ticks_since_threatened + 1;
    }

have_streaks:
    if (6 < new_grade) {
        self->has_engaged = 1;
    }
}

namespace actor_update_combat_behavior_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern actor_mode_definition actor_mode_definitions[16];
extern char actor_evaluate_combat_state_transition(uint32_t actor_index);
extern uint8_t actor_update_melee_combat_action(datum_index actor_index);
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    result = 0;
    use_param_1 = 1;
    if (param_2 == 0) {
        use_param_1 = param_1;
    }

    switch (actor_mode_definitions[self->mode].combat_grade) {
    case 1:
    case 2:
        if (use_param_1 == 0) goto use_default;
        if (self->combat_status < 5) {
            if (self->combat_status < 2 && self->mode != 2 &&
                (self->combat_status != 0 || (self->stood_down == 0 && self->post_combat_action < 1))) {
                goto use_default;
            }
            goto call_melee_combat_action;
        }
        result = actor_evaluate_combat_state_transition(actor_index);
        break;
    case 3:
        if (use_param_1 == 0 || self->combat_status < 4) {
            if (1 < self->combat_status) {
                if (self->target_unit_index == (datum_index)k_datum_index_none) {
                    goto use_default;
                }
                target_prop = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
                if (self->target_unit_index == self->pursuit_target_prop_index &&
                    (target_prop->noticed_a != 0 || (self->mode == 5 && *(int16_t *)(self->mode_data.raw + 8) == 0)) &&
                    (target_prop->noticed_b != 0 ||
                     ((self->mode == 5 && *(int16_t *)(self->mode_data.raw + 8) == 0) ||
                      (self->mode == 7 && *(int16_t *)(self->mode_data.raw + 8) == 0)))) {
                    goto use_default;
                }
            }
        call_melee_combat_action:
            result = actor_update_melee_combat_action(actor_index);
        } else {
            result = actor_evaluate_combat_state_transition(actor_index);
        }
        break;
    case 4:
        if (self->combat_status < 4) goto call_melee_combat_action;
        result = actor_evaluate_combat_state_transition(actor_index);
        break;
    default:
        goto use_default;
    }
    if (result != 0) {
        return result;
    }
use_default:
    if (param_2 != 0) {
        result = actor_update_melee_combat_action(actor_index);
    }
    return result;
}

namespace actor_update_crouch_state_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern double exp2(double x);
extern int32_t __ftol(double x);
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
extern uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index,
    real_vector3d *out_vector);
extern int16_t actor_evaluate_flank_offset(const real_vector3d *cover_direction,
                                           real_vector3d *out_offset,
                                           const real_point3d *threat_position,
                                           const real_point3d *candidate_position);
extern void actor_scan_allies_for_backup_request(datum_index actor_index);
extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag);
extern uint8_t actor_evaluate_custom_charge_trigger(datum_index actor_index);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & 0xffff].data;

    threat_level           = &self->unknown_350;
    threat_level_smoothed  = &self->danger_meter;
    crouching              = &self->unknown_358;
    crouch_timer           = &self->unknown_35a;
    flag_35c               = &self->unknown_35c[0];
    flag_35d               = &self->unknown_35c[1];
    flag_35e               = &self->unknown_35c[2];
    flag_35f               = &self->unknown_35c[3];
    countdown_360          = &self->unknown_360;
    countdown_368          = &self->evasion_delay_ticks;

    if (self->berserking != 0 &&
        (self->combat_status == 0 || self->awareness_level < 3 ||
         (*(int32_t *)&self->shield_vitality == 0x3f800000 && self->combat_status < 3))) {
        actor_set_combat_alert_flag(actor_index, 0);
    }

    platoon_flag = self->platoon_defending;
    if (self->defending != platoon_flag) {
        self->defending = platoon_flag;
        if (self->unit_index != (datum_index)k_datum_index_none) {
            ai_communication_broadcast((int16_t)((platoon_flag != 0) + 0x16), self->unit_index,
                                       0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0);
        }
    }

    if (self->berserking == 0 && (actor_definition->flags & 0x800) == 0) {
        self->always_charge = 0;
    } else {
        self->always_charge = 1;
    }
    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if ((actor_definition->flags & 0x1000000) != 0 && self->defending == 0) {
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

    decay = (float)exp2(1.4426950408889634 * -0.04620981216430664);
    *threat_level_smoothed = (*threat_level - *threat_level_smoothed) * (1.0f - decay) +
                             *threat_level_smoothed;

    if (self->stood_down != 0) {
        self->stood_down_body_vitality = self->body_vitality;
    }

    if ((actor_definition->flags & 0xc0000000u) != 0) {
        if (self->active_unit_index == (datum_index)k_datum_index_none && self->combat_status > 2) {
            combat_status = self->target_combat_status;
            *flag_35d = 0;
            *flag_35c = 0;
            *flag_35e = 0;
            *flag_35f = 0;

            if (combat_status > 8) {
                target_direction = *(real_vector3d *)((uint8_t *)prop_data->data +
                                                      (self->target_unit_index & 0xffff) * sizeof(prop) + 0xe0);
            }
            for (prop_index = self->first_prop; prop_index != (datum_index)k_datum_index_none;
                 prop_index = p->next_in_actor) {
                p = &((prop *)prop_data->data)[prop_index & 0xffff];

                if (p->state > 1 && p->state < 4 && p->enemy == 0 && p->dead == 0 &&
                    p->swarm_owned == 0 &&
                    (p->is_parented != 0 || p->relationship_object_index == -1)) {

                    if (actor_get_ranged_attack_vector(prop_index, actor_index, (real_vector3d *)&cover_direction) != 0) {
                        grade = actor_evaluate_flank_offset(&cover_direction, &flank_offset,
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
                                grade_second = actor_evaluate_flank_offset(&cover_direction,
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
                        grade = actor_evaluate_flank_offset(&target_direction, (real_vector3d *)0,
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
        actor_push_recognition_entry(actor_index, ((struct actor *)self)->firing_position_index, 1);
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
            want_crouch = (uint8_t)(*(float *)&self->shield_vitality < threshold);
            break;
        case 3:
            want_crouch = (uint8_t)(*(float *)&self->shield_vitality > threshold &&
                                    (int8_t)self->tally.threat_class_2 >= 1);
            break;
        case 4:
            want_crouch = (uint8_t)(self->combat_status > 0);
            break;
        case 5:
            want_crouch = actor_evaluate_custom_charge_trigger(actor_index);
            break;
        default:
            want_crouch = 0;
            break;
        }

        if ((actor_definition->flags & 0x40000000u) != 0) {
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
                    *crouch_timer = (int16_t)__ftol((double)(threshold * 30.0f));
                }
            }
        } else if (want_crouch == 0) {
            *crouching = 0;
            threshold = actor_definition->min_stand_time;
            if (threshold <= 0.0f) {
                *crouch_timer = 0x2d;
            } else {
                *crouch_timer = (int16_t)__ftol((double)(threshold * 30.0f));
            }
        }
    }

    if (*countdown_368 > 0) {
        *countdown_368 = (int16_t)(*countdown_368 - 1);
    }
    actor_scan_allies_for_backup_request(actor_index);
}

namespace actor_update_danger_avoidance_local {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
extern uint8_t actor_action_has_queued_secondary(datum_index actor_index);
extern uint8_t actor_movement_action_is_complete(datum_index actor_index);
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern uint8_t actor_find_danger_escape(datum_index actor_index, uint32_t *out_word, uint8_t *out_position,
    real_vector3d *path_delta, uint8_t *in_danger);
extern uint8_t actor_take_danger_escape(real_vector3d *path_delta, datum_index actor_index, uint32_t escape,
    uint32_t extra, float distance);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))
}
}

/**
 * Actor AI behaviour: update danger avoidance.
 *
 * @address 0x40c040
 */
uint8_t ActorView::update_danger_avoidance()
{
    using namespace actor_update_danger_avoidance_local;
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
        float distance_squared = halo::math::point3d_distance_squared_to_segment(*path_start, path_delta, *position);
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
            towards = halo::math::point3d_distance_squared_to_segment(*path_start, path_delta, *(real_point3d *)(actor + 0x4ac)) <
                radius_squared;
            if (B(0x504) != 0 && !in_danger && !towards) {
                real_vector3d movement;

                movement.i = F(0x518) * 3.0f;
                movement.j = F(0x51c) * 3.0f;
                movement.k = F(0x520) * 3.0f;
                if (halo::math::segment3d_distance_squared_to_segment(path_start, position, &movement, &path_delta) <
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
    danger_time = halo::math::ray_intersect_sphere_distance(*position, *path_start, path_delta, F(0x294));
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
                uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[D(0x58) & 0xffff].data;
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

#undef B
#undef W
#undef D
#undef F

namespace actor_update_facing_change_timer_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern int32_t __ftol(double x);
}
}

/**
 * Once the actor's "facing change pending" flag is set and its Actor tag allows a nonzero stand-facing-change
 * time, clears the flag and converts that time to a tick count. If the actor's current target (read here as a
 * prop index, see UNSURE above) is within 4 world units, clamps a smoothing field (unk
 *
 * @address 0x423670
 */
void ActorView::update_facing_change_timer()
{
    using namespace actor_update_facing_change_timer_local;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(halo::cache::globals().tag_instances[self->actor_definition_tag & 0xffff].data);
    uint8_t *pending_flag = &self->unknown_358;
    int16_t *ticks_field = &self->unknown_35a;
    float *smoothing_field = &self->danger_meter;

    if (*pending_flag != 0 && actor_tag->change_facing_stand_time > 0.0f) {
        *pending_flag = 0;
        *ticks_field = (int16_t)__ftol((double)(actor_tag->change_facing_stand_time * 30.0f));

        if (self->target_unit_index != (datum_index)k_datum_index_none) {
            prop *target = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
            if (target->distance < 4.0f) {
                if (*smoothing_field <= 1.8f) {
                    *smoothing_field = 1.8f;
                }
            }
        }
    }
}

namespace actor_update_flee_response_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out,
    datum_index actor_index);
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    result = 0;

    if (self->flee_source.code == 0 && self->movement_action_complete == 0) {
        self->flee_reason = 0;
    }

    if (self->flee_reason > 2 && self->flee_source.code != 0) {
        result = actor_resolve_flee_source_point(&self->flee_source,
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
extern "C" {
extern data_array *actor_data;
extern ai_globals *ai_globals_ptr;
}
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
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    int16_t vehicle_substate = *(int16_t *)&self->mode_data.raw[4];
    int fast = self->mode == _actor_mode_vehicle &&
               (vehicle_substate == 2 || vehicle_substate == 3 || vehicle_substate == 4 || vehicle_substate == 5);

    self->idle_counter = self->idle_counter + (fast ? 3 : 1);

    if (ai_globals_ptr->stagger_claimed == 0 && ai_globals_ptr->stagger_threshold < self->idle_counter &&
        self->idle_counter > 15) {
        self->idle_counter = 0;
        ai_globals_ptr->stagger_claimed = 1;
        self->needs_new_path = 1;
        return;
    }

    if (ai_globals_ptr->stagger_highest < self->idle_counter) {
        ai_globals_ptr->stagger_highest = self->idle_counter;
    }
    self->needs_new_path = 0;
}

namespace actor_update_look_target_local {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index);
extern uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out,
    datum_index actor_index);
extern uint8_t actor_point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis,
    float min_cos_threshold, float side_thresholds[2]);
extern uint8_t actor_resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table,
    uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback);
extern void actor_look_randomize_direction(datum_index actor_index, float *deviation_table, real_vector3d *base_direction);
extern float *actor_get_idle_facing_range(datum_index actor_index);
extern int32_t actor_look_get_wait_ticks(datum_index actor_index, int16_t mode, uint32_t flags, float *deviation_table);
extern uint8_t actor_reset_queued_look_vector(datum_index actor_index);
extern void actor_update_facing_change_timer(datum_index actor_index);
extern double cos(double x);
extern double fabs(double x);
#define ULT_V3(p) (*(real_point3d *)(p))
static uint8_t ult_cone(real_point3d *point, uint8_t *reference, float cos_threshold)
{
    return halo::math::point3d_within_horizontal_cone(*point, *(real_point3d *)reference, cos_threshold);
}
static uint8_t ult_lane(real_point3d *point, uint8_t *forward, uint8_t *axis, float cos_threshold, float *side)
{
    return actor_point_in_directional_lane(point, (real_point3d *)forward, (real_point3d *)axis, cos_threshold, side);
}
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
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data;
    uint8_t *cache_a = a + 0x5a4;
    uint8_t *cache_b = a + 0x5b0;
    uint8_t *cache_c = a + 0x5bc;
    int16_t look_mode = ((struct actor *)a)->control_animation_mode;
    uint8_t flee_look = 0;
    uint8_t aim_speed_zero;

    if (look_mode == 1) {
        ULT_V3(cache_b) = ULT_V3(cache_a);
        ULT_V3(cache_c) = ULT_V3(cache_a);
    } else {
        uint8_t has_weapon;
        uint8_t side_a = a[0x58d];
        uint8_t look_follows = 0;
        uint8_t free_aim = 1;
        uint8_t side_b = a[0x58e];
        uint8_t claimed = 0;
        uint8_t in_cone = 0;
        uint8_t resolved = 0;
        uint8_t range_1, trust;
        float cos_aim = *(float *)(definition + 0x12c);
        float cos_look = *(float *)(definition + 0x134);
        float side_cos[2];
        int16_t reason;
        int16_t priority = 0;
        real_point3d flee_point;
        real_point3d voc_point;
        uint8_t section_done;
        float *range;
        actor_flee_source_reason kind2;

        if (a[0x161] != 0) {
            has_weapon = 1;
        } else if (look_mode == 0 || look_mode == 2) {
            has_weapon = actor_get_threat_weapon_object_index(actor_index) != k_datum_index_none;
        } else {
            has_weapon = 0;
        }
        look_follows = has_weapon;
        if (((actor *)a)->awareness_level == 3) {
            side_cos[0] = (float)cos((double)*(float *)(definition + 0xbc));
            side_cos[1] = (float)cos((double)*(float *)(definition + 0xc0));
        } else {
            side_cos[0] = (float)cos((double)*(float *)(definition + 0xb4));
            side_cos[1] = (float)cos((double)*(float *)(definition + 0xb8));
        }

        memset(&kind2, 0, sizeof(kind2));
        kind2.code = 2;
        if (((struct actor *)a)->firing_target_type > 0 && ((struct actor *)a)->firing_state == 2 && a[0x456] == 0 &&
            actor_resolve_flee_source_point(&kind2, (real_vector3d *)&flee_point, actor_index)) {
            reason = 7;
            flee_look = 1;
        } else {
            reason = (int16_t)*(uint16_t *)&((actor *)a)->flee_reason;
            if (reason != 0 && reason != 1) {
                if (actor_resolve_flee_source_point(&((actor *)a)->flee_source,
                        (real_vector3d *)&flee_point, actor_index)) {
                    flee_look = ((actor *)a)->flee_source.code == 2;
                } else {
                    reason = 0;
                }
            }
        }

        if (((actor *)a)->vocalization_line >= 0 && ((actor *)a)->vocalization_state > 0 &&
            actor_resolve_flee_source_point(&((actor *)a)->vocalization_source, (real_vector3d *)&voc_point,
                actor_index)) {
            priority = (int16_t)*(uint16_t *)&((actor *)a)->vocalization_variant;
        }
        if (a[0x504] != 0 &&
            *(int16_t *)((uint8_t *)actor_mode_definitions + ((actor *)a)->mode * 0x38 + 4) == 2 && priority > 5) {
            priority = 5;
        }
        if (((actor *)a)->vocalization_state > 0) {
            ((actor *)a)->vocalization_state = (int16_t)(((actor *)a)->vocalization_state - 1);
            if (((actor *)a)->vocalization_state == 0) {
                ((actor *)a)->vocalization_line = 0;
                ((actor *)a)->vocalization_variant = 0;
            }
        }
        a[0x58c] = 0;

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
                ULT_V3(cache_b) = flee_point;
                free_aim = 0;
                look_follows = has_weapon;
                if (reason >= 7) {
                    claimed = 1;
                    if (has_weapon) {
                        ULT_V3(cache_c) = flee_point;
                    }
                }
            }
            if (reason == 2) {
                reason = side_a ? 5 : 0;
            }
            if (side_a) {
                ULT_V3(cache_a) = flee_point;
                a[0x591] = (uint8_t)(a[0x591] | (reason == 4));
                side_a = 0;
                side_b = 0;
            }
            section_done = ((a[0x58d] == 0 && a[0x58e] == 0) || reason >= 6) ? 1 : 0;
        }

        if (priority >= 2 && priority <= 6) {
            in_cone = ult_cone(&voc_point, cache_a, cos_aim);
            if (section_done) {
                if (claimed) {
                    goto lane_or_take;
                }
                goto cone_gate;
            }
            if (claimed) {
                goto lane_or_take;
            }
            if (priority >= 6 && actor_reset_queued_look_vector(actor_index)) {
                goto take_all;
            }
            if (priority >= 5 && (side_b || a[0x58d] != 0)) {
                goto take_all;
            }
            if (priority >= 4) {
                if (in_cone) {
                    goto priority_gate;
                }
                if (!side_a || !free_aim) {
                    goto lane_or_take;
                }
                goto take_all;
            }
        cone_gate:
            if (!in_cone) {
                goto lane_or_take;
            }
        priority_gate:
            if (priority >= 5 || (priority >= 3 && free_aim)) {
                ULT_V3(cache_b) = voc_point;
                ULT_V3(cache_c) = voc_point;
                look_follows = has_weapon;
                goto voc_claim;
            }
        lane_or_take:
            if (has_weapon && ult_lane(&voc_point, cache_b, cache_a, cos_look, side_cos)) {
                ULT_V3(cache_c) = voc_point;
                look_follows = 0;
            } else if (free_aim && in_cone) {
                ULT_V3(cache_b) = voc_point;
                ULT_V3(cache_c) = voc_point;
                look_follows = 1;
                free_aim = 0;
            }
            goto switch_done;
        take_all:
            if (!(a[0x591] != 0 && in_cone)) {
                ULT_V3(cache_a) = voc_point;
                a[0x591] = 0;
            }
            ULT_V3(cache_b) = voc_point;
            ULT_V3(cache_c) = voc_point;
            goto voc_face;
        } else if (priority == 7 || priority == 8) {
            resolved = in_cone = (uint8_t)(priority == 8);
            if (a[0x58d] == 0) {
                if (!ult_cone(&voc_point, cache_a, cos_aim)) {
                    if (!actor_reset_queued_look_vector(actor_index)) {
                        goto switch_done;
                    }
                    in_cone = 1;
                } else if (!resolved) {
                    goto voc_aim;
                }
            }
            ULT_V3(cache_a) = voc_point;
            a[0x591] = in_cone;
        voc_aim:
            ULT_V3(cache_b) = voc_point;
            ULT_V3(cache_c) = voc_point;
        voc_face:
            side_a = 0;
            look_follows = has_weapon;
        voc_claim:
            a[0x58c] = 1;
            claimed = 0;
            free_aim = 0;
        }
    switch_done:
        if (reason == 2 && free_aim && ult_cone(&flee_point, cache_a, cos_aim)) {
            ULT_V3(cache_b) = flee_point;
            if (look_follows) {
                ULT_V3(cache_c) = flee_point;
            }
            a[0x58c] = 0;
            free_aim = 0;
        }

        range = actor_get_idle_facing_range(actor_index);
        range_1 = range[1] > 0.0f;
        side_b = range[3] > 0.0f;
        in_cone = range[5] > 0.0f;
        if (((struct actor *)a)->look_posture > 0 && !claimed && (free_aim || look_follows) &&
            (range_1 || side_b || in_cone)) {
            resolved = 0;
            claimed = 0;
            trust = (range_1 && side_a && reason == 1 && *(int32_t *)(a + 0x560) == 0) ? 1 : 0;
            if (*(int32_t *)(a + 0x560) > 0) {
                *(int32_t *)(a + 0x560) -= 1;
            }
            if (a[0x55c] != 0 && a[0x55d] != 0 && !free_aim) {
                a[0x55c] = 1;
                ((struct actor *)a)->idle_major_timer = actor_look_get_wait_ticks(actor_index, 2, 1, range);
                ULT_V3(a + 0x570) = ULT_V3(cache_b);
                ((struct actor *)a)->idle_major_direction_type = 4;
            }
            if (!(a[0x55c] != 0 && ((struct actor *)a)->idle_major_timer != 0)) {
                uint8_t use_aiming;
                uint8_t force = 0;
                uint8_t *direction = 0;

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
                    a[0x55e] = actor_resolve_look_target((real_point3d *)direction, actor_index, range, trust,
                        use_aiming, force);
                    resolved = 1;
                }
            }
            if (a[0x55c] != 0) {
                ((struct actor *)a)->idle_major_timer -= 1;
                if (actor_resolve_flee_source_point((actor_flee_source_reason *)(a + 0x56c), (real_vector3d *)&voc_point,
                        actor_index)) {
                    if (free_aim) {
                        if (side_a && range_1 && a[0x99] != 0) {
                            trust = 1;
                            claimed = 1;
                            goto idle_take_ab;
                        }
                        if (trust) {
                            goto idle_take_ab;
                        }
                        if (!ult_cone(&voc_point, cache_a, cos_aim)) {
                            goto idle_reset;
                        }
                        ULT_V3(cache_b) = voc_point;
                        a[0x58c] = 1;
                        goto idle_timers;
                    idle_take_ab:
                        ULT_V3(cache_a) = voc_point;
                        ULT_V3(cache_b) = voc_point;
                        a[0x58c] = 1;
                        goto idle_timers;
                    }
                    if (!ult_lane(&voc_point, cache_b, cache_a, cos_look, side_cos)) {
                        goto idle_reset;
                    }
                    ULT_V3(cache_c) = voc_point;
                    goto idle_timers;
                }
            }
        idle_reset:
            voc_point = ULT_V3(cache_b);
            a[0x55c] = 0;
            goto idle_follow;
        idle_timers:
            if (resolved && in_cone) {
                a[0x55f] = 1;
                *(int32_t *)(a + 0x568) = actor_look_get_wait_ticks(actor_index, 2, a[0x55e], range);
                memcpy(a + 0x57c, a + 0x56c, 16);
                if (trust) {
                    *(int32_t *)(a + 0x560) = actor_look_get_wait_ticks(actor_index, 0, a[0x55e], range);
                }
            }
        idle_follow:
            if (!free_aim) {
                goto clear_hold;
            }
            if (!((look_follows && in_cone) || (claimed && side_b))) {
                goto clear_hold;
            }
            if (*(int32_t *)(a + 0x568) == 0) {
                actor_look_randomize_direction(actor_index, range, (real_vector3d *)&voc_point);
            }
            *(int32_t *)(a + 0x568) -= 1;
            if (a[0x55f] == 0) {
                goto body_turn;
            }
            if (!actor_resolve_flee_source_point((actor_flee_source_reason *)(a + 0x57c), (real_vector3d *)&flee_point,
                    actor_index)) {
                goto clear_hold;
            }
            if (claimed ? !ult_cone(&flee_point, cache_a, cos_aim)
                        : !ult_lane(&flee_point, cache_b, cache_a, cos_look, side_cos)) {
                goto clear_hold;
            }
            if (claimed) {
                ULT_V3(cache_b) = flee_point;
            }
            ULT_V3(cache_c) = flee_point;
            goto body_turn;
        }
        a[0x55c] = 0;
        a[0x55e] = 0;
    clear_hold:
        a[0x55f] = 0;
    body_turn:
        if (a[0x504] == 0 && a[0x505] == 0 && !halo::units::unit_is_in_busy_animation_state(*(uint32_t *)&((actor *)a)->unit_index) &&
            ((actor *)a)->active_unit_index == k_datum_index_none) {
            if (ult_cone((real_point3d *)cache_b, cache_a, cos_aim) &&
                !ult_cone((real_point3d *)cache_b, a + 0x174, cos_aim)) {
                a[0x591] = 1;
            } else if (has_weapon) {
                if (ult_lane((real_point3d *)cache_c, cache_b, cache_a, cos_look, side_cos) &&
                    !ult_lane((real_point3d *)cache_c, cache_b, a + 0x174, cos_look, side_cos)) {
                    a[0x591] = 1;
                }
            }
        }
        if (!has_weapon) {
            ULT_V3(cache_c) = ULT_V3(cache_b);
        }
    }

    if (a[0x99] == 0 && !(fabs((double)((actor *)a)->desired_facing_vector.z) < 9.999999747378752e-05)) {
        ((actor *)a)->desired_facing_vector.z = 0.0f;
        if (halo::math::vector2d_normalize_with_length(*(real_vector2d *)cache_a) == 0.0f) {
            ULT_V3(cache_a) = ULT_V3(a + 0x174);
        }
    }
    if (a[0x58f] != 0) {
        if (a[0x590] == 0) {
            if (a[0x504] == 0 &&
                ((actor *)a)->unit_aiming_vector.k * ((actor *)a)->desired_aiming_vector.z + ((actor *)a)->unit_aiming_vector.j * ((actor *)a)->desired_aiming_vector.y +
                ((actor *)a)->unit_aiming_vector.i * ((actor *)a)->desired_aiming_vector.x > 0.9f) {
                ULT_V3(a + 0x598) = ULT_V3(cache_a);
                a[0x590] = 1;
            }
        } else if (*(float *)(definition + 0x330) > 0.0f) {
            float limit = (float)cos((double)*(float *)(definition + 0x330));
            uint8_t keep = 0;

            if (a[0x99] != 0) {
                keep = ((actor *)a)->desired_facing_vector.z * *(float *)(a + 0x5a0) + ((actor *)a)->desired_facing_vector.y * *(float *)(a + 0x59c) +
                       ((actor *)a)->desired_facing_vector.x * *(float *)(a + 0x598) > limit &&
                       *(float *)(a + 0x5a0) * ((actor *)a)->desired_aiming_vector.z + *(float *)(a + 0x59c) * ((actor *)a)->desired_aiming_vector.y +
                       ((actor *)a)->desired_aiming_vector.x * *(float *)(a + 0x598) > limit;
            } else {
                real_vector2d aim2, face2, hold2;

                aim2.i = ((actor *)a)->desired_aiming_vector.x;
                aim2.j = ((actor *)a)->desired_aiming_vector.y;
                face2.i = ((actor *)a)->desired_facing_vector.x;
                face2.j = ((actor *)a)->desired_facing_vector.y;
                hold2.i = *(float *)(a + 0x598);
                hold2.j = *(float *)(a + 0x59c);
                if (halo::math::vector2d_normalize_with_length(face2) != 0.0f && halo::math::vector2d_normalize_with_length(aim2) != 0.0f &&
                    halo::math::vector2d_normalize_with_length(hold2) != 0.0f) {
                    keep = hold2.j * face2.j + hold2.i * face2.i > limit && aim2.j * hold2.j + aim2.i * hold2.i > limit;
                }
            }
            if (!keep) {
                a[0x590] = 0;
                actor_update_facing_change_timer(actor_index);
            }
        }
    } else {
        a[0x590] = 0;
    }

    ULT_V3(a + 0x6fc) = ULT_V3(cache_a);
    ULT_V3(a + 0x708) = ULT_V3(cache_b);
    ULT_V3(a + 0x714) = ULT_V3(cache_c);
    if (a[0x591] != 0) {
        ((actor *)a)->control_flags |= 0x20;
    } else {
        ((actor *)a)->control_flags &= ~0x20u;
    }

    aim_speed_zero = 1;
    if (!flee_look && ((struct actor *)a)->look_posture != 4) {
        switch (((actor *)a)->vocalization_line) {
        case 3: case 6: case 10: case 11: case 12:
            break;
        default:
            aim_speed_zero = 0;
            break;
        }
    }
    *(int16_t *)(a + 0x6f8) = aim_speed_zero ? 0 : 1;
}

#undef ULT_V3

namespace actor_update_melee_combat_action_local {
extern "C" {
extern data_array *actor_data;
extern data_array *encounter_data;
extern data_array *prop_data;
extern uint8_t *actor_type_procs[];
extern actor_mode_definition actor_mode_definitions[16];
extern void ai_starting_location_derive_placement_flags(datum_index encounter_index, int16_t starting_location_index,
    uint8_t *out_a, int16_t *out_b, uint8_t *out_c, int16_t *out_d, int16_t *out_edx, int16_t *out_esi);
extern void encounter_evaluate_support_needs(datum_index encounter_index, datum_index self_actor_index, int16_t mode,
    uint8_t phase, uint8_t *out_crowded, uint8_t *out_flanked, uint8_t *out_a, uint8_t *out_b,
    uint8_t *out_reachable_a, uint8_t *out_reachable_b, uint8_t *out_any);
extern void actor_get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index,
    int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g,
    uint8_t *out_h, uint8_t *out_i);
extern int32_t actor_build_order_wait_byte(uint32_t actor_index, uint8_t byte_a, uint32_t *order);
extern void actor_set_target_alert_stage1(datum_index target_prop_index, datum_index actor_index);
extern int32_t actor_build_order_flee(uint32_t actor_index, uint8_t byte_a, uint32_t *order);
extern void actor_set_target_alert_stage2(datum_index target_prop_index, datum_index actor_index);
extern datum_index actor_get_target_prop_object_index(datum_index actor_index);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern int32_t actor_build_order_minimal_stop(uint32_t actor_index, uint32_t *order);
extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override);
extern uint32_t actor_find_best_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok);
extern uint32_t actor_build_order_face_seat_marker(uint32_t actor_index, int16_t firing_position_index, uint32_t *order);
extern uint32_t actor_build_order_face_seat_marker_committed(uint32_t actor_index, int16_t firing_position_index,
    uint8_t byte_a, uint32_t *order);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
extern uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type,
    int32_t min_last_tick);
extern int32_t actor_build_order_random_wait(uint32_t actor_index, uint8_t byte_a, uint32_t *order);
extern int32_t actor_build_order_search_wait(uint32_t actor_index, actor_order *order);
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern void actor_set_target_alert_stage3(datum_index target_prop_index, datum_index actor_index);
extern int32_t actor_build_order_guard(uint32_t actor_index, actor_order *order, int16_t guard_at_current_position);
#define W(p, o) (*(int16_t *)((p) + (o)))
#define D(p, o) (*(datum_index *)((p) + (o)))
static uint8_t actor_combat_commit_position(datum_index actor_index, uint8_t *a, uint8_t *target, int16_t position)
{
    datum_index object = target != 0 ? D(target, 0x7c) : k_datum_index_none;

    if (D(a, 0x34) != k_datum_index_none &&
        ai_pursuit_note_object(actor_index, D(a, 0x34), position, (int32_t)object)) {
        if (W(a, 0x3c4) == 0) {
            ai_communication_broadcast(0x10, D(a, 0x18), k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
        }
        W(a, 0x3c4) += 1;
    }
    return 1;
}
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
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[D(a, 0x58) & 0xffff].data;
    datum_index encounter_index = D(a, 0x34);
    uint8_t *encounter = encounter_index != k_datum_index_none
        ? (uint8_t *)encounter_data->data + (encounter_index & 0xffff) * 0x6c : 0;
    uint8_t result = 0;
    uint8_t regroup = 0;
    uint8_t searching = 0;
    uint8_t order[0x8c];

    memset(order, 0, sizeof(order));
    if (encounter != 0 && encounter[0x42] && W(a, 0x6e) <= 2 && W(a, 0x72) == 0 && W(a, 0x74) == 0) {
        regroup = 1;
    }
    if (W(a, 0x1e4) > 0 && W(a, 0x6e) <= 2 && W(a, 0x74) == 0) {
        searching = 1;
    }
    if (W(a, 0x6a) < 3 && actor_mode_definitions[W(a, 0x6c)].combat_grade == 0 ) {
        return 1;
    }
    if (a[0x160] || searching || regroup) {
        if (searching) {
            if (W(a, 0x6c) == 6 && a[0xa1]) {
                return 1;
            }
            if (actor_build_order_search_wait(actor_index, (actor_order *)order)) {
                actor_set_mode(actor_index, 6, order);
                return 1;
            }
        }
        if (regroup) {
            result = actor_process_order_request(actor_index, 0xffff);
            if (result) {
                return result;
            }
        }
        goto guard;
    }
    if (W(a, 0x6e) < 2) {
        goto guard;
    }

    {
        uint8_t *target = D(a, 0x270) != k_datum_index_none
            ? (uint8_t *)prop_data->data + (D(a, 0x270) & 0xffff) * 0x138 : 0;
        uint8_t hold = 0;
        uint8_t advance = 0;
        uint8_t pressed = 0;
        uint8_t retreat = 0;
        uint8_t reposition = 0;
        uint8_t move_ok = 0;
        uint8_t wait_ok = 0;

        if (target == 0 || !target[0xbb]) {
            uint8_t *type = actor_type_procs[W(a, 0x4)];
            int16_t ax_mode = W(type, 0x6);
            int16_t cx_mode = W(type, 0x8);
            int16_t mode_b = W(type, 0xa);
            uint8_t phase = type[0xc];
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
                ai_starting_location_derive_placement_flags(encounter_index, (int16_t)*(uint16_t *)&((actor *)a)->squad_index, &hold,
                                                            &support_mode, &phase, &ax_mode, &mode_b, &cx_mode);
                if (a[0x6]) {
                    retreat = 1;
                    pressed = 1;
                    advance = 1;
                    move_ok = 1;
                    reposition = 1;
                } else {
                    encounter_evaluate_support_needs(encounter_index, actor_index, support_mode, phase, &pressed,
                                                     &retreat, &reposition, &move_ok, &regroup, &reachable_b, &wait_ok);
                }
            }
            actor_get_target_state_flags(ax_mode, cx_mode, regroup, actor_index, mode_b, 0, (char)a[0x375], &advance,
                                         (char *)&pressed, &retreat, &reposition, &move_ok, &wait_ok);
        }

        if (D(a, 0x3c0) != D(a, 0x270)) {
            W(a, 0x3c4) = 0;
            D(a, 0x3c0) = D(a, 0x270);
            a[0x3bc] = 0;
            a[0x3bd] = 0;
        }
        if (advance && actor_build_order_wait_byte(actor_index, retreat, (uint32_t *)order)) {
            actor_set_mode(actor_index, 5, order);
            return 1;
        }
        actor_set_target_alert_stage1(D(a, 0x270), actor_index);
        if (retreat && actor_build_order_flee(actor_index, a[0x375], (uint32_t *)order)) {
            actor_set_mode(actor_index, 7, order);
            return 1;
        }
        actor_set_target_alert_stage2(D(a, 0x270), actor_index);
        if (a[0x3bc] && !a[0x3bd]) {
            ai_communication_broadcast(0xd, D(a, 0x18), actor_get_target_prop_object_index(actor_index), -1,
                                       k_datum_index_none, k_datum_index_none, 0);
            a[0x3bd] = 1;
        }

        if (reposition) {
            int16_t position = -1;
            uint8_t have_position = 0;

            a[0x98] = 1;
            if (a[0x6]) {
                if (move_ok && actor_build_order_minimal_stop(actor_index, (uint32_t *)order)) {
                    actor_set_mode(actor_index, 7, order);
                    return actor_combat_commit_position(actor_index, a, target, position);
                }
            } else {
                int16_t limit;

                if (W(a, 0x6c) == 5 && move_ok && W(a, 0xa4) == 1) {
                    position = W(a, 0xa6);
                    have_position = 1;
                }
                if (!(have_position && position != -1)) {
                    limit = D(a, 0x1d0) == k_datum_index_none ? W(actor_tag, 0x356) : W(actor_tag, 0x354);
                    if (!hold && D(a, 0x3c0) == D(a, 0x270) && W(a, 0x3c4) >= limit) {
                        goto no_position;
                    }
                    {
                        static uint8_t query[0x664];
                        static uint8_t candidate[0x3c];
                        static path_find_context path_context;
                        uint32_t previous_owner = 0;
                        uint8_t path_ok = 0;

                        memset(query, 0, sizeof(query));
                        W(query, 0x4) = 5;
                        D(query, 0x8) = D(a, 0x270);
                        D(query, 0xc) = target != 0 ? D(target, 0x7c) : k_datum_index_none;
                        query[0x43] = (uint8_t)(D(a, 0x270) != k_datum_index_none);
                        query[0x14] = hold;
                        *(uint32_t *)query = actor_get_firing_position_group_mask(actor_index, 5, 0);
                        ((struct actor_firing_position_query *)query)->search_radius = 20.0f;
                        position = (int16_t)actor_find_best_firing_position(actor_index, (actor_firing_position_query *)query,
                            (actor_firing_position_candidate *)candidate, &previous_owner,
                            &path_context, &path_ok);
                    }
                    if (position == -1) {
                        goto no_position;
                    }
                    if (!have_position &&
                        actor_build_order_face_seat_marker(actor_index, position, (uint32_t *)order)) {
                        actor_set_mode(actor_index, 5, order);
                        return actor_combat_commit_position(actor_index, a, target, position);
                    }
                }
                if (move_ok &&
                    actor_build_order_face_seat_marker_committed(actor_index, position, hold, (uint32_t *)order)) {
                    actor_set_mode(actor_index, 7, order);
                    return actor_combat_commit_position(actor_index, a, target, position);
                }
            }
        }

    no_position:
        if (W(a, 0x3c4) > 0 && D(a, 0x18) != k_datum_index_none) {
            ai_communication_broadcast(0x13, D(a, 0x18), k_datum_index_none, -1, k_datum_index_none,
                                       k_datum_index_none, 0);
        }
        if (!a[0x6] && wait_ok && actor_build_order_random_wait(actor_index, reposition, (uint32_t *)order)) {
            actor_set_mode(actor_index, 8, order);
            return 1;
        }
    }

guard:
    if (actor_mode_definitions[W(a, 0x6c)].combat_grade == 1 ) {
        return result;
    }
    {
        int16_t mode = W(a, 0x6c);
        int16_t guard_at = ((mode == 7 && !a[0x9d]) || mode == 8) ? 0 : 0x5a;

        actor_set_target_alert_stage3(D(a, 0x270), actor_index);
        actor_build_order_guard(actor_index, (actor_order *)order, guard_at);
        actor_set_mode(actor_index, 6, order);
    }
    return 1;
}

#undef W
#undef D

namespace actor_update_movement_destination_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern void *actor_get_actor_definition(datum_index actor_index);
extern uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point,
    int32_t start_surface_index, int16_t kind);
extern int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok);
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok);
extern void actor_movement_action_stop(datum_index actor_index);
extern void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t mark_engaged);
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
extern void actor_update_target_lead_position(datum_index actor_index);
extern float actor_compute_accuracy_scale(datum_index actor_index);
#define A_B(o) (actor[(o)])
#define A_W(o) (*(int16_t *)(actor + (o)))
#define A_D(o) (*(uint32_t *)(actor + (o)))
#define A_F(o) (*(float *)(actor + (o)))
static real_point3d *actor_held_firing_position(uint8_t *actor)
{
    uint8_t *encounter = (uint8_t *)halo::scenario::globals().scenario->encounters.pointer + (A_D(0x34) & 0xffff) * 0xb0;

    return (real_point3d *)(*(uint8_t **)(encounter + 0x9c) + A_W(0x3b8) * 0x18);
}
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
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag;
    uint8_t *definition;

    if (A_B(0x4c) == 0) {
        return 0;
    }
    actor_tag = (uint8_t *)halo::cache::globals().tag_instances[A_D(0x58) & 0xffff].data;
    definition = (uint8_t *)actor_get_actor_definition(actor_index);

    if (A_B(0x160) == 0) {
        uint8_t follow_lead = 0;

        if (A_B(0x358) != 0 && (actor_tag[0] & 0x20) != 0) {
            actor_update_target_lead_position(actor_index);
            follow_lead = actor_firing_position_near_point(actor_index, (real_point3d *)(actor + 0x168),
                (int32_t)A_D(0x164), 0);
        }
        if (follow_lead) {
            uint8_t at_position = 0;
            uint8_t drop = 0;

            if (A_D(0x34) != 0xffffffff && A_W(0x3b8) != -1) {
                float radius = actor_compute_accuracy_scale(actor_index);

                if (radius * radius > halo::math::vector3d_distance_squared(*actor_held_firing_position(actor),
                        *(real_point3d *)(actor + 0x12c))) {
                    at_position = 1;
                }
            }
            if (A_B(0x504) != 0 && A_B(0x1fc) == 0) {
                if (A_D(0x270) != 0xffffffff) {
                    uint8_t *target = (uint8_t *)prop_data->data + (A_D(0x270) & 0xffff) * 0x138;

                    drop = ((prop *)target)->distance < *(float *)(definition + 0xa0);
                }
            } else {
                drop = !at_position;
            }
            if (drop) {
                A_W(0x3b8) = -1;
                actor_movement_action_stop(actor_index);
            }
        } else {
            static actor_firing_position_query query;
            static path_find_context path_context;
            actor_firing_position_candidate candidate;
            uint32_t previous_owner = 0xffffffff;
            uint8_t path_ok = 0;
            int16_t previous = A_W(0x3b8);
            int16_t selected;
            int16_t claimed;

            memset(&query, 0, sizeof(query));
            memset(&candidate, 0, sizeof(candidate));
            selected = actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
                &path_ok);
            claimed = actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok);
            actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
            if (claimed == -1) {
                A_W(0x9c) = 0;
            } else if (claimed != previous) {
                float wait = halo::math::random_real_range(*(float *)(actor_tag + 0x3c0), *(float *)(actor_tag + 0x3c4));

                if (A_W(0x15e) > 0) {
                    uint8_t *vehicle = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[A_D(0x158) & 0xffff].data;
                    uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)vehicle & 0xffff].data;
                    float cap = *(float *)(vehicle_tag + 0x3a8);

                    if (cap > 0.0f && wait > cap) {
                        wait = cap;
                    }
                }
                A_W(0x9c) = (int16_t)(int32_t)(wait * 30.0f);
            }
        }
    }

    if (A_W(0x268) < 7) {
        return 0;
    }
    {
        uint8_t *target = (uint8_t *)prop_data->data + (A_D(0x270) & 0xffff) * 0x138;
        uint8_t engaged = 1;

        if (actor_has_unshielded_threat_weapon(actor_index)) {
            if (((prop *)target)->distance < A_F(0x608)) {
                engaged = 0;
            } else if (A_D(0x34) != 0xffffffff && A_W(0x3b8) != -1) {
                real_point3d *held = actor_held_firing_position(actor);
                float radius = actor_compute_accuracy_scale(actor_index);

                if (radius * radius < halo::math::vector3d_distance_squared(*held, *(real_point3d *)(actor + 0x12c))) {
                    float range = A_F(0x608);

                    if (range * range > halo::math::vector3d_distance_squared(*(real_point3d *)(target + 0xbc), *held)) {
                        engaged = 0;
                    }
                }
            }
        }
        actor_target_mark_engaged(A_D(0x270), actor_index, engaged);
    }
    return 0;
}

#undef A_B
#undef A_W
#undef A_D
#undef A_F

namespace actor_update_path_if_needed_local {
extern "C" {
extern data_array *actor_data;
extern int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok);
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok);
}
}

/**
 * Actor AI behaviour: update path if needed.
 *
 * @address 0x4017b0
 */
uint8_t ActorView::update_path_if_needed()
{
    using namespace actor_update_path_if_needed_local;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    if (actor[0x4c] != 0) {
        static actor_firing_position_query query;
        static path_find_context path_context;
        actor_firing_position_candidate candidate;
        uint32_t previous_owner = 0xffffffff;
        uint8_t path_ok = 0;
        int16_t selected;

        memset(&query, 0, sizeof(query));
        memset(&candidate, 0, sizeof(candidate));
        query.goal_kind = 6;
        selected = actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
            &path_ok);
        actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok);
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    }
    return ((struct actor *)actor)->danger_type == 0;
}

namespace actor_update_squad_link_state_local {
extern "C" {
extern data_array *actor_data;
extern data_array *encounter_data;
extern data_array *prop_data;
extern actor_mode_definition actor_mode_definitions[16];
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead);
}
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
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    encounter *enc = 0;
    uint8_t combined_flag;

    if (self->swarm != 0 && self->swarm_index == (datum_index)k_datum_index_none) {
        actor_delete_or_release_unit(actor_index, 0);
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
        enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
    }

    combined_flag = self->force_active;
    if (enc != 0) {
        combined_flag |= enc->force_active;
    }

    if (self->can_go_dormant == 0 || combined_flag != 0) {
        actor_set_units_active(actor_index, 0);
    } else if (self->keep_unit_alive == 0) {
        int16_t combat_grade = actor_mode_definitions[self->mode].combat_grade;
        int stale = 1;

        if (combat_grade != 2) {
            if (self->target_unit_index != (datum_index)k_datum_index_none) {
                prop *target = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
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
                    if (self->mode == 6 && enc != 0 && ((struct encounter *)enc)->follow_target_type == 1) {
                        return 1;
                    }
                } else if (self->active_movement.type == 5) {
                    prop *p = &((prop *)prop_data->data)[*(datum_index *)&((struct actor *)self)->active_movement.destination.x & 0xffff];
                    if (p->is_parented != 0) {
                        return 1;
                    }
                }
            }
            *(int16_t *)((uint8_t *)self + 0x14) = *(int16_t *)((uint8_t *)self + 0x14) + 1;
            if (*(int16_t *)((uint8_t *)self + 0x14) > 0x3b) {
                actor_set_units_active(actor_index, 1);
                return 1;
            }
        }
    }
    return 1;
}

namespace actor_update_swarm_component_position_local {
extern "C" {
extern data_array *swarm_component_data;
}
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
    object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[unit_index & 0xffff].data;
    swarm_component *component = &((swarm_component *)swarm_component_data->data)[component_index & 0xffff];
    datum_index marker;

    marker = (unit_object->type == 0) ? *(datum_index *)((uint8_t *)unit_object + 0x4d8) : (datum_index)k_datum_index_none;

    halo::objects::object_get_position(&component->position, unit_index);
    component->marker_index = marker;
}

namespace actor_update_target_combat_status_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->target_unit_index == k_datum_index_none) {
        self->target_combat_status = 0;
        self->target_last_seen_time = k_datum_index_none;
        self->target_alive = 0;
        return;
    }

    target = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
    target_obj = ((object_header *)halo::objects::globals().object_data->data)[target->object_index & 0xffff].data;

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
extern "C" {
extern data_array *actor_data;
}
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
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    real_point3d *point = (real_point3d *)(a + 0x168);
    datum_index vehicle;

    if (((struct actor *)a)->pathfinding_surface_index != -1) {
        return;
    }
    *point = *(real_point3d *)&((actor *)a)->body_position.x;
    if (a[0x99]) {
        return;
    }
    vehicle = ((actor *)a)->active_unit_index;
    if (vehicle != k_datum_index_none) {
        int16_t seat_kind = ((struct actor *)a)->vehicle_driving_type;

        if (seat_kind >= 2 && seat_kind <= 3) {
            ((struct actor *)a)->pathfinding_surface_index = halo::units::unit_predict_aim_target_position(vehicle, point);
        }
        return;
    }
    if (halo::objects::object_try_and_get(((actor *)a)->unit_index, 1) != 0) {
        ((struct actor *)a)->pathfinding_surface_index = (int32_t)halo::units::biped_get_cached_look_at_position(((actor *)a)->unit_index, point);
    }
}

}
