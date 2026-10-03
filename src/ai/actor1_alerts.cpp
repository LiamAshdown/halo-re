#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/ai/actor_alerts.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace c_actor_alert_from_damage {

static void actor_raise_alert(struct actor *actor, int16_t level, uint32_t source)
{
    if (actor->pending_panic_type == 0 || actor->pending_panic_prop_index == halo::k_dword_none) {
        actor->pending_panic_prop_index = source;
    }
    if (actor->pending_panic_type <= level) {
        actor->pending_panic_type = level;
    }
}

}


/**
 * actor_alert_from_damage: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_alert_from_damage.c.txt.
 *
 * @address 0x40a500
 */
uint8_t halo::ai::alert_ops::alert_from_damage()
{
    using namespace c_actor_alert_from_damage;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    uint32_t source = halo::k_dword_none;

    if (actor->unknown_1b4[1] == 0) {
        return 0;
    }
    if (actor->unit_index != halo::k_dword_none) {
        uint8_t *unit = (uint8_t *)halo::ai::object_at(actor->unit_index);
        datum_index attacker = static_cast<datum_index>(((struct unit_object *)unit)->unit.flaming_responsible_object);

        if (attacker != k_datum_index_none) {
            unit_object *attacker_unit = (unit_object *)halo::objects::object_try_and_get(attacker, 3);

            if (attacker_unit != 0) {
                if (attacker_unit->unit.gunner_unit_index != k_datum_index_none) {
                    attacker = attacker_unit->unit.gunner_unit_index;
                } else if (attacker_unit->unit.driver_unit_index != k_datum_index_none) {
                    attacker = attacker_unit->unit.driver_unit_index;
                }
                if (attacker != k_datum_index_none) {
                    source = halo::ai::actor_find_prop_for_object(attacker, actor_index);
                }
            }
        }
    }
    actor_raise_alert(actor, 0xc, source);
    return 1;
}

namespace halo::ai {
uint8_t actor_alert_from_damage(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_damage();
}
}


namespace c_actor_alert_from_disturbance {

static void actor_raise_alert(struct actor *actor, int16_t level, uint32_t source)
{
    if (actor->pending_panic_type == 0 || actor->pending_panic_prop_index == halo::k_dword_none) {
        actor->pending_panic_prop_index = source;
    }
    if (actor->pending_panic_type <= level) {
        actor->pending_panic_type = level;
    }
}
}


/**
 * actor_alert_from_disturbance: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_alert_from_disturbance.c.txt.
 *
 * @address 0x40a3d0
 */
uint8_t halo::ai::alert_ops::alert_from_disturbance()
{
    using namespace c_actor_alert_from_disturbance;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->unknown_2f0[0] == 0 || (*(uint32_t *)halo::cache::globals().tag_instances[actor->actor_definition_tag & halo::k_slot_mask].data & 0x400) == 0) {
        return 0;
    }
    actor_raise_alert(actor, 7, actor->look_at_reference);
    actor->unknown_2f0[0] = 0;
    return 1;
}

namespace halo::ai {
uint8_t actor_alert_from_disturbance(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_disturbance();
}
}


namespace c_actor_alert_from_flag_1b4 {
}


/**
 * actor_alert_from_flag_1b4: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_alert_from_flag_1b4.c.txt.
 *
 * @address 0x40a6b0
 */
uint8_t halo::ai::alert_ops::alert_from_flag_1b4()
{
    using namespace c_actor_alert_from_flag_1b4;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->unknown_1b4[0] == 0) {
        return 0;
    }
    if (actor->pending_panic_type <= 0xb) {
        actor->pending_panic_type = 0xb;
    }
    actor->pending_panic_prop_index = halo::k_dword_none;
    return 1;
}

namespace halo::ai {
uint8_t actor_alert_from_flag_1b4(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_flag_1b4();
}
}


namespace c_actor_alert_from_projectile {

static void actor_raise_alert(struct actor *actor, int16_t level, uint32_t source)
{
    if (actor->pending_panic_type == 0 || actor->pending_panic_prop_index == halo::k_dword_none) {
        actor->pending_panic_prop_index = source;
    }
    if (actor->pending_panic_type <= level) {
        actor->pending_panic_type = level;
    }
}

}


/**
 * actor_alert_from_projectile: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_alert_from_projectile.c.txt.
 *
 * @address 0x40a5e0
 */
uint8_t halo::ai::alert_ops::alert_from_projectile()
{
    using namespace c_actor_alert_from_projectile;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    uint32_t source = halo::k_dword_none;
    uint8_t *noticed;

    if (actor->stuck_projectile_index == halo::k_dword_none) {
        return 0;
    }
    noticed = (uint8_t *)halo::objects::object_try_and_get((uint32_t)actor->stuck_projectile_index, halo::k_dword_none);
    if (noticed != 0 && *(datum_index *)(noticed + 0xc4) != k_datum_index_none) {
        datum_index creator = *(datum_index *)(noticed + 0xc4);
        unit_object *creator_unit = (unit_object *)halo::objects::object_try_and_get(creator, 3);

        if (creator_unit != 0) {
            datum_index who;

            if (creator_unit->unit.gunner_unit_index != k_datum_index_none) {
                who = creator_unit->unit.gunner_unit_index;
            } else if (creator_unit->unit.driver_unit_index != k_datum_index_none) {
                who = creator_unit->unit.driver_unit_index;
            } else {
                who = creator;
            }
            if (who != k_datum_index_none) {
                source = halo::ai::actor_find_prop_for_object(who, actor_index);
            }
        }
    }
    actor_raise_alert(actor, 0xa, source);
    return 1;
}

namespace halo::ai {
uint8_t actor_alert_from_projectile(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_projectile();
}
}


namespace c_actor_alert_from_squad_attack {

static void actor_raise_alert(struct actor *actor, int16_t level, uint32_t source)
{
    if (actor->pending_panic_type == 0 || actor->pending_panic_prop_index == halo::k_dword_none) {
        actor->pending_panic_prop_index = source;
    }
    if (actor->pending_panic_type <= level) {
        actor->pending_panic_type = level;
    }
}

}


/**
 * actor_alert_from_squad_attack: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_alert_from_squad_attack.c.txt.
 *
 * @address 0x40a460
 */
uint8_t halo::ai::alert_ops::alert_from_squad_attack()
{
    using namespace c_actor_alert_from_squad_attack;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    Actor *definition;

    if (actor->unknown_2e8[4] == 0) {
        return 0;
    }
    definition = halo::ai::tag_data<Actor>(actor->actor_definition_tag);
    if (!(actor->recent_body_damage > definition->panic_damage_threshold)) {
        return 0;
    }
    if (actor->pending_panic_type == 0 || actor->pending_panic_prop_index == halo::k_dword_none) {
        actor->pending_panic_prop_index = halo::ai::actor_get_squad_recent_attacker_target(actor_index, 1);
    }
    if (actor->pending_panic_type <= 1) {
        actor->pending_panic_type = 1;
    }
    actor->unknown_2e8[4] = 0;
    return 1;
}

namespace halo::ai {
uint8_t actor_alert_from_squad_attack(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_squad_attack();
}
}


namespace c_actor_check_pain_reaction {
}


/**
 * actor_check_pain_reaction: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_pain_reaction.c.txt.
 *
 * @address 0x40de20
 */
uint8_t halo::ai::alert_ops::check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code, datum_index actor_index)
{
    using namespace c_actor_check_pain_reaction;
    uint16_t order_buffer[66];

    if (halo::ai::actor_build_order_grenade_or_melee(resolved_target, use_alt_base, actor_index, order_code,
            0, 0, order_buffer) != 0) {
        halo::ai::actor_set_mode(actor_index, 4, order_buffer);
        return 1;
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code, datum_index actor_index)
{
    return halo::ai::alert_ops::check_pain_reaction(resolved_target, use_alt_base, order_code, actor_index);
}
}

namespace c_actor_combat_status_should_hold {
}


/**
 * actor_combat_status_should_hold: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_combat_status_should_hold.c.txt.
 *
 * @address 0x40d520
 */
uint8_t halo::ai::alert_ops::combat_status_should_hold(int16_t threshold_a, int16_t threshold_b)
{
    using namespace c_actor_combat_status_should_hold;
    datum_index actor_index = datum;
    actor *self;

    self = halo::ai::actor_at(actor_index);

    if (self->mode_data.raw[8] != 0) {
        return (uint8_t)(threshold_b <= self->combat_status);
    }
    if (0 < *(int16_t *)&self->mode_data.raw[0] && self->combat_status < threshold_a &&
        (self->post_combat_action < 1 || self->mode_data.raw[5] != 0)) {
        return 0;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b)
{
    return halo::ai::alert_ops(actor_index).combat_status_should_hold(threshold_a, threshold_b);
}
}

namespace c_actor_conditional_state_transition_check {
}


/**
 * actor_conditional_state_transition_check: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_conditional_state_transition_check.c.txt.
 *
 * @address 0x40d7a0
 */
uint8_t halo::ai::alert_ops::conditional_state_transition_check()
{
    using namespace c_actor_conditional_state_transition_check;
    datum_index actor_index = datum;
    actor *self;
    int16_t sub_state;
    uint8_t flag;

    self = halo::ai::actor_at(actor_index);

    if (self->mode != _actor_mode_vehicle) {
        return 0;
    }
    sub_state = *(int16_t *)&self->mode_data.raw[4];
    if (sub_state == 2 || sub_state == 3) {
        if (self->mode_data.raw[7] != 0 || self->mode_data.raw[8] != 0) {
            return halo::ai::actor_evaluate_combat_state_transition(actor_index);
        }
        flag = self->mode_data.raw[0x29];
    } else {
        if (sub_state != 4 && sub_state != 5) {
            return 0;
        }
        flag = self->mode_data.raw[0x29];
    }
    if (flag == 0) {
        return 0;
    }
    return halo::ai::actor_evaluate_combat_state_transition(actor_index);
}

namespace halo::ai {
uint8_t actor_conditional_state_transition_check(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).conditional_state_transition_check();
}
}

namespace c_actor_consider_combat_mode {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_consider_combat_mode: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_consider_combat_mode.c.txt.
 *
 * @address 0x401a60
 */
uint8_t halo::ai::alert_ops::consider_combat_mode(int16_t consideration_mode, actor_combat_consideration *out)
{
    using namespace c_actor_consider_combat_mode;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(actor->actor_definition_tag);
    uint8_t *record = (uint8_t *)out;
    int16_t mode = consideration_mode;
    uint8_t result = 1;

    memset(out, 0, 0x38);
    *(int32_t *)record = halo::game::globals().game_time->game_time;

    if (mode == 5 || mode == 4) {
        ((struct actor_combat_consideration *)record)->mode = mode;
        return actor->vehicle_driving_type > 1;
    }
    if (mode == 2) {
        object *unit;
        prop *target;
        uint8_t leap = 0;
        int16_t frame_count = 0;
        int16_t key_frame = 0;
        float dx_to_key_frame = 0.0f;
        float dx_total = 0.0f;
        float wait;
        float limit;

        result = 0;
        if (actor->swarm != 0) {
            ((struct actor_combat_consideration *)record)->mode = mode;
            return result;
        }
        unit = (object *)halo::ai::object_at(actor->unit_index);
        if ((static_cast<uint8_t>(unit->vitality_flags) & 0x80) != 0 || actor->target_unit_index == k_datum_index_none) {
            ((struct actor_combat_consideration *)record)->mode = mode;
            return result;
        }
        target = halo::ai::prop_at(actor->target_unit_index);
        if (actor_tag->melee_leap_range[1] == 0.0f || actor_tag->melee_leap_chance == 0.0f) {
            record[0xa] = 0;
        } else if (target->flying != 0 || target->engaged_ticks > 0) {
            record[0xa] = 1;
            leap = 1;
            mode = 3;
        } else {
            leap = halo::math::random_real() < actor_tag->melee_leap_chance;
            record[0xa] = leap;
            if (target->distance < actor_tag->melee_leap_range[0]) {
                leap = 0;
            } else if (leap) {
                mode = 3;
            }
        }
        if (!halo::units::unit_get_weapon_marker_indices(actor->unit_index, leap, (uint32_t)&dx_to_key_frame,
                (uint32_t)&dx_total, &frame_count, &key_frame)) {
            ((struct actor_combat_consideration *)record)->mode = mode;
            return result;
        }
        if (halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::suicidal_melee_attack)) {
            ((struct actor_combat_consideration *)record)->position_index = frame_count;
            ((struct actor_combat_consideration *)record)->distance_delta = 0.0f;
            record[0x30] = 1;
        } else if (key_frame == 0) {
            ((struct actor_combat_consideration *)record)->position_index = (int16_t)(frame_count / 2);
            ((struct actor_combat_consideration *)record)->distance_delta = dx_total - dx_total * 0.5f;
        } else {
            ((struct actor_combat_consideration *)record)->position_index = key_frame;
            ((struct actor_combat_consideration *)record)->distance_delta = dx_total - dx_to_key_frame;
        }
        wait = halo::ai::actor_get_consideration_wait_threshold(actor_index, mode, out);
        ((struct actor_combat_consideration *)record)->wait_threshold = wait;
        limit = mode == 3 ? 4.0f : 1.5f;
        if (limit > wait) {
            wait = limit;
        }
        if (halo::ai::actor_movement_set_destination_near_target(actor->target_unit_index, actor_index, wait)) {
            halo::ai::actor_movement_actions_cancel(actor_index);
            if (halo::ai::actor_grenade_trace_from_source(actor_index, &target->center_of_mass)) {
                result = 1;
            }
        }
        ((struct actor_combat_consideration *)record)->mode = mode;
        return result;
    }
    if (mode == 0 && halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::use_stalking_behavior) && actor->combat_status >= 5 && actor->berserking == 0) {
        mode = 1;
    }

    ((struct actor_combat_consideration *)record)->mode = mode;
    return result;
}

namespace halo::ai {
uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode, actor_combat_consideration *out)
{
    return halo::ai::alert_ops(actor_index).consider_combat_mode(consideration_mode, out);
}
}

namespace c_actor_escalate_apply {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);


}


/**
 * actor_escalate_apply: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_escalate_apply.c.txt.
 *
 * @address 0x40aa70
 */
uint8_t halo::ai::alert_ops::escalate_apply(int16_t threshold)
{
    using namespace c_actor_escalate_apply;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t result = 0;

    if (act->escalation_level >= threshold && !act->berserking) {
        halo::ai::actor_set_combat_alert_flag(actor_index, 1);
        if (act->combat_status >= 4) {
            result = (uint8_t)halo::ai::actor_evaluate_combat_state_transition(actor_index);
        }
    }
    act->escalation_level = 0;
    return result;
}

namespace halo::ai {
uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold)
{
    return halo::ai::alert_ops(actor_index).escalate_apply(threshold);
}
}


namespace c_actor_escalate_check_leader_flag {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

}


/**
 * actor_escalate_check_leader_flag: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_escalate_check_leader_flag.c.txt.
 *
 * @address 0x40a7f0
 */
uint8_t halo::ai::alert_ops::escalate_check_leader_flag()
{
    using namespace c_actor_escalate_check_leader_flag;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);

    if (!halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::always_berserk_in_attacking_mode) || act->platoon_defending || act->combat_status < 5) {
        return 0;
    }
    if (act->escalation_level <= 1) {
        act->escalation_level = 1;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_escalate_check_leader_flag(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_leader_flag();
}
}


namespace c_actor_escalate_check_shield_damage {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
}


/**
 * actor_escalate_check_shield_damage: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_escalate_check_shield_damage.c.txt.
 *
 * @address 0x40a9e0
 */
uint8_t halo::ai::alert_ops::escalate_check_shield_damage()
{
    using namespace c_actor_escalate_check_shield_damage;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = TAG_DATA(act->actor_definition_tag);

    if (!act->unknown_2e8[4] || !(act->recent_body_damage > ((Actor *)actor_tag)->berserk_damage_amount) ||
        !(act->body_vitality < ((Actor *)actor_tag)->berserk_damage_threshold)) {
        return 0;
    }
    if (act->escalation_level <= 3) {
        act->escalation_level = 3;
    }
    act->unknown_2e8[4] = 0;
    return 1;
}

namespace halo::ai {
uint8_t actor_escalate_check_shield_damage(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_shield_damage();
}
}

#undef TAG_DATA

namespace c_actor_escalate_check_target_close {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
}


/**
 * actor_escalate_check_target_close: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_escalate_check_target_close.c.txt.
 *
 * @address 0x40a950
 */
uint8_t halo::ai::alert_ops::escalate_check_target_close()
{
    using namespace c_actor_escalate_check_target_close;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->combat_status < 5) {
        return 0;
    }
    if (!(((struct prop *)PROP(act->target_unit_index))->distance <
          *(float *)(TAG_DATA(act->actor_definition_tag) + 0x3a0))) {
        return 0;
    }
    if (act->escalation_level <= 2) {
        act->escalation_level = 2;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_escalate_check_target_close(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_target_close();
}
}

#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_check_weapon_range {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

}


/**
 * actor_escalate_check_weapon_range: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_escalate_check_weapon_range.c.txt.
 *
 * @address 0x40a860
 */
uint8_t halo::ai::alert_ops::escalate_check_weapon_range()
{
    using namespace c_actor_escalate_check_weapon_range;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = TAG_DATA(act->actor_definition_tag);
    ActorVariant *definition = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);

    if (static_cast<datum_index>(act->stuck_projectile_index) == k_datum_index_none || act->combat_status < 5) {
        return 0;
    }
    if (!(((struct prop *)PROP(act->target_unit_index))->distance < definition->berserk_firing_ranges[1])) {
        return 0;
    }
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    if (!((float)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f < ((Actor *)actor_tag)->berserk_grenade_chance)) {
        return 0;
    }
    if (act->escalation_level <= 4) {
        act->escalation_level = 4;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_escalate_check_weapon_range(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_weapon_range();
}
}

#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_to_guard_or_combat {

}


/**
 * actor_escalate_to_guard_or_combat: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_escalate_to_guard_or_combat.c.txt.
 *
 * @address 0x40aaf0
 */
uint8_t halo::ai::alert_ops::escalate_to_guard_or_combat()
{
    using namespace c_actor_escalate_to_guard_or_combat;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    uint8_t record[0x84];

    if (!(actor->awareness_level < 3) || actor->search_priority == 0) {
        return 0;
    }
    actor->awareness_level = 3;
    if (halo::ai::actor_build_guard_mode_data(actor_index, record)) {
        halo::ai::actor_set_mode(actor_index, 6, record);
    } else {
        halo::ai::actor_evaluate_combat_state_transition(actor_index);
    }
    actor->search_priority = 0;
    return 1;
}

namespace halo::ai {
uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_to_guard_or_combat();
}
}


namespace c_actor_evaluate_combat_state_transition {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);


#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define OBJECT_DATA(h) ((uint8_t *)halo::ai::object_at((h)))
}


/**
 * actor_evaluate_combat_state_transition: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_evaluate_combat_state_transition.c.txt.
 *
 * @address 0x40c620
 */
char halo::ai::alert_ops::evaluate_combat_state_transition()
{
    using namespace c_actor_evaluate_combat_state_transition;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(a->actor_definition_tag);
    uint8_t *variant = TAG_DATA(a->actor_variant_tag);
    ActorVariant *definition = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);
    uint8_t changed = 0;
    uint8_t fallback = 0;
    prop *p = 0;
    float distance = 3.4028235e+38f;
    actor_combat_consideration consideration;
    int16_t mode;
    uint8_t hold;

    if (a->target_unit_index != k_datum_index_none) {
        p = halo::ai::prop_at(a->target_unit_index);
        distance = p->distance;

        if (a->mode == 0xa && a->mode_data.charge.stage == 1) {
            uint8_t engaged = p->seen || (p->shooting && (int8_t)p->distance_class <= 1);

            if (!engaged && actor_tag->stalking_discovery_time > 0.0f &&
                !(a->mode_data.charge.weak_target_ticks < (int16_t)(int32_t)(actor_tag->stalking_discovery_time * 30.0f) )) {
                engaged = 1;
            }
            if (engaged) {
                if (!(distance <= definition->desired_combat_range[1])) {
                    changed = halo::ai::actor_handle_death(actor_index, 0, 0);
                    if (!changed) {
                        halo::ai::actor_set_combat_alert_flag(actor_index, 1);
                    }
                } else {
                    halo::ai::actor_set_combat_alert_flag(actor_index, 1);
                }
            }
        }

        if (!(halo::ai::actor_has_unshielded_threat_weapon(actor_index) &&
              (p->relationship_object_index != k_datum_index_none || p->swarm_owned)) &&
            !(a->mode == 0xa && (a->mode_data.charge.stage == 2 || a->mode_data.charge.stage == 3)) &&
            !changed && !a->swarm && a->active_unit_index == k_datum_index_none &&
            a->firing_state != 2) {
            int32_t now = game_time->game_time;
            uint8_t wide = a->berserking;
            float base_delay;
            float delay;
            float range;
            int16_t difficulty = (int16_t)(uint16_t)halo::main::globals().game_globals->difficulty;

            if (!halo::ai::actor_has_unshielded_threat_weapon(actor_index) && !halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::use_stalking_behavior)) {
                wide = 1;
            }
            base_delay = a->berserking ? 0.0f : actor_tag->melee_attack_delay;
            delay = halo::game::weapon_get_zoom_fov(0x14, difficulty) + halo::game::weapon_get_zoom_fov(0x15, difficulty) * base_delay;
            range = wide ? ((ActorVariant *)variant)->berserk_melee_range : ((ActorVariant *)variant)->melee_range;
            if (!(a->search_wait_time != -1 && a->search_wait_time + 0xa >= now) &&
                distance <= range) {
                uint8_t near_enough = 1;

                if (a->charge_disallowed) {
                    float extra = actor_tag->melee_fudge_factor;

                    if (!(0.0f <= extra)) {
                        extra = 0.0f;
                    }
                    near_enough = distance <= 0.8f + extra;
                }
                if (near_enough &&
                    (a->last_melee_time == -1 || (float)now > delay * 30.0f + (float)a->last_melee_time)) {
                    halo::ai::actor_has_unshielded_threat_weapon(actor_index);
                    a->search_wait_time = now;
                    if (halo::ai::actor_consider_combat_mode(actor_index, 2, &consideration)) {
                        halo::ai::actor_set_mode(actor_index, 0xa, &consideration);
                        changed = 1;
                    }
                }
            }
        }

        if (a->mode != 0xa && !a->charge_disallowed) {
            int16_t seat_kind = a->vehicle_driving_type;

            if (changed) {
                return changed;
            }
            if (seat_kind > 0) {
                uint8_t ready = 1;

                if (a->last_vehicle_charge_time != -1) {
                    Vehicle *vehicle_tag = halo::ai::tag_data<Vehicle>(*(datum_index *)OBJECT_DATA(a->active_unit_index));

                    ready = (float)game_time->game_time >
                        vehicle_tag->ai_charge_repeat_timeout * 30.0f + (float)a->last_vehicle_charge_time;
                }
                if (ready && seat_kind == 4 && distance > definition->melee_range &&
                    p->obstruction == 0 &&
                    halo::ai::actor_consider_combat_mode(actor_index, 4, &consideration)) {
                    halo::ai::actor_set_mode(actor_index, 0xa, &consideration);
                    return 1;
                }
            }
        } else if (changed) {
            return changed;
        }
    }

    fallback = (a->always_charge && !a->charge_disallowed) ? 1 : 0;
    hold = 0;
    if (!a->charge_disallowed && !halo::ai::actor_has_unshielded_threat_weapon(actor_index) && halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::always_charge_in_attacking_mode)) {
        fallback = 1;
    }
    mode = a->mode;
    if (mode == 0xa) {
        int16_t state = a->mode_data.charge.stage;

        if (state == 2 || state == 3) {
            if (!a->mode_data.flee.unknown_07 && !a->mode_data.charge.done && !a->mode_data.charge.approach_failed) {
                fallback = 1;
                goto consider_zero;
            }
            hold = 1;
            goto decide;
        }
        if (a->charge_disallowed) {
            goto guard;
        }
        if (state == 4 || state == 5) {
            if (a->mode_data.charge.approach_failed || a->vehicle_driving_type <= 1) {
                hold = 1;
                goto decide;
            }
            fallback = 1;
            if (state != 4) {
                goto consider_zero;
            }
            {
                Vehicle *vehicle_tag = halo::ai::tag_data<Vehicle>(*(datum_index *)OBJECT_DATA(a->active_unit_index));
                float vehicle_range = vehicle_tag->ai_strafing_abort_range;

                if (a->movement_completed && a->active_movement.type == 5 &&
                    halo::bit_cast<datum_index>(a->active_movement.destination.x) == a->target_unit_index) {
                    goto guard;
                }
                if (distance < vehicle_range) {
                    goto guard;
                }
                if (!(vehicle_range + vehicle_range > distance)) {
                    goto consider_zero;
                }
                if (a->facing.k * p->direction.z + a->facing.j * p->direction.y +
                        p->direction.x * a->facing.i >= 0.5f) {
                    goto consider_zero;
                }
                goto guard;
            }
        }
    }
decide:
    if (!fallback) {
        goto guard;
    }
    if (hold) {
        goto consider;
    }
consider_zero:
    if (mode == 0xa) {
        goto settle;
    }
consider:
    if (halo::ai::actor_consider_combat_mode(actor_index, 0, &consideration)) {
        halo::ai::actor_set_mode(actor_index, 0xa, &consideration);
        changed = 1;
    } else {
        goto guard;
    }
settle:
    if (fallback || changed) {
        return changed;
    }
guard:
    if (a->mode == 3) {
        return changed;
    }
    *(uint32_t *)&consideration = 0;
    halo::ai::actor_set_mode(actor_index, 3, &consideration);
    return 1;
}

namespace halo::ai {
char actor_evaluate_combat_state_transition(uint32_t actor_index)
{
    return halo::ai::alert_ops(actor_index).evaluate_combat_state_transition();
}
}

#undef OBJECT_DATA
#undef TAG_DATA

namespace c_actor_is_within_alert_range {
}


/**
 * actor_is_within_alert_range: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_is_within_alert_range.c.txt.
 *
 * @address 0x408f30
 */
uint8_t halo::ai::alert_ops::is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index)
{
    using namespace c_actor_is_within_alert_range;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    object *obj = halo::ai::object_at(object_index);
    uint8_t result = 0;

    if ((obj->vitality_flags & _object_health_frozen_bit) == 0) {
        if (always_in_range == 0) {
            float radius = (use_radius_b == 0) ? radius_b : radius_a;
            real_point3d obj_position;

            halo::objects::object_get_position(&obj_position, object_index);

            if (vitality_only != 0 ||

                ((obj_position.z - a->body_position.z) * (obj_position.z - a->body_position.z) +
                 (obj_position.x - a->body_position.x) * (obj_position.x - a->body_position.x) +
                 (obj_position.y - a->body_position.y) * (obj_position.y - a->body_position.y) < radius * radius)) {
                result = 1;
                if (vitality_only != 0) {
                    return 1;
                }
                if (use_radius_b == 0 &&
                    obj->velocity.i * obj->velocity.i + obj->velocity.j * obj->velocity.j + obj->velocity.k * obj->velocity.k > 0.00027777778f) {
                    result = 0;
                }
            }
            goto check_upright;
        }
        result = 1;
    }
    if (vitality_only != 0) {
        return result;
    }
check_upright:
    if (obj->up.k < 0.5f) {
        return 0;
    }
    return result;
}

namespace halo::ai {
uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index)
{
    return halo::ai::alert_ops::is_within_alert_range(always_in_range, radius_a, radius_b, vitality_only, use_radius_b, actor_index, object_index);
}
}

