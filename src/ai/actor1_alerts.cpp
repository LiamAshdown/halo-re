#include "halo/ai/actor_alerts.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"

namespace c_actor_alert_from_damage {
extern "C" {
extern data_array *actor_data;

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == halo::k_dword_none) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
}
}

extern "C" uint8_t actor_alert_from_damage(datum_index actor_index);

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
    uint8_t *actor = ACTOR(actor_index);
    uint32_t source = halo::k_dword_none;

    if (B(0x1b5) == 0) {
        return 0;
    }
    if (D(0x18) != halo::k_dword_none) {
        uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[D(0x18) & halo::k_slot_mask].data;
        datum_index attacker = *(datum_index *)&((struct unit_object *)unit)->unit.flaming_responsible_object;

        if (attacker != k_datum_index_none) {
            uint8_t *attacker_unit = (uint8_t *)halo::objects::object_try_and_get(attacker, 3);

            if (attacker_unit != 0) {
                if (*(datum_index *)(attacker_unit + 0x328) != k_datum_index_none) {
                    attacker = *(datum_index *)(attacker_unit + 0x328);
                } else if (*(datum_index *)(attacker_unit + 0x324) != k_datum_index_none) {
                    attacker = *(datum_index *)(attacker_unit + 0x324);
                }
                if (attacker != k_datum_index_none) {
                    source = actor_find_prop_for_object(attacker, actor_index);
                }
            }
        }
    }
    actor_raise_alert(actor, 0xc, source);
    return 1;
}

extern "C" uint8_t actor_alert_from_damage(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_damage();
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

namespace c_actor_alert_from_disturbance {
extern "C" {
extern data_array *actor_data;

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == halo::k_dword_none) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}
}
}

extern "C" uint8_t actor_alert_from_disturbance(datum_index actor_index);

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
    uint8_t *actor = ACTOR(actor_index);

    if (B(0x2f0) == 0 || (*(uint32_t *)halo::cache::globals().tag_instances[D(0x58) & halo::k_slot_mask].data & 0x400) == 0) {
        return 0;
    }
    actor_raise_alert(actor, 7, D(0x2f4));
    B(0x2f0) = 0;
    return 1;
}

extern "C" uint8_t actor_alert_from_disturbance(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_disturbance();
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

namespace c_actor_alert_from_flag_1b4 {
extern "C" {
extern data_array *actor_data;

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))
}
}

extern "C" uint8_t actor_alert_from_flag_1b4(datum_index actor_index);

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
    uint8_t *actor = ACTOR(actor_index);

    if (B(0x1b4) == 0) {
        return 0;
    }
    if (W(0x308) <= 0xb) {
        W(0x308) = 0xb;
    }
    D(0x30c) = halo::k_dword_none;
    return 1;
}

extern "C" uint8_t actor_alert_from_flag_1b4(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_flag_1b4();
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

namespace c_actor_alert_from_projectile {
extern "C" {
extern data_array *actor_data;

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == halo::k_dword_none) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
}
}

extern "C" uint8_t actor_alert_from_projectile(datum_index actor_index);

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
    uint8_t *actor = ACTOR(actor_index);
    uint32_t source = halo::k_dword_none;
    uint8_t *noticed;

    if (D(0x1b0) == halo::k_dword_none) {
        return 0;
    }
    noticed = (uint8_t *)halo::objects::object_try_and_get(D(0x1b0), halo::k_dword_none);
    if (noticed != 0 && *(datum_index *)(noticed + 0xc4) != k_datum_index_none) {
        datum_index creator = *(datum_index *)(noticed + 0xc4);
        uint8_t *creator_unit = (uint8_t *)halo::objects::object_try_and_get(creator, 3);

        if (creator_unit != 0) {
            datum_index who;

            if (*(datum_index *)(creator_unit + 0x328) != k_datum_index_none) {
                who = *(datum_index *)(creator_unit + 0x328);
            } else if (*(datum_index *)(creator_unit + 0x324) != k_datum_index_none) {
                who = *(datum_index *)(creator_unit + 0x324);
            } else {
                who = creator;
            }
            if (who != k_datum_index_none) {
                source = actor_find_prop_for_object(who, actor_index);
            }
        }
    }
    actor_raise_alert(actor, 0xa, source);
    return 1;
}

extern "C" uint8_t actor_alert_from_projectile(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_projectile();
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

namespace c_actor_alert_from_squad_attack {
extern "C" {
extern data_array *actor_data;

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == halo::k_dword_none) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}

extern datum_index actor_get_squad_recent_attacker_target(datum_index actor_index, char require_is_unit);
}
}

extern "C" uint8_t actor_alert_from_squad_attack(datum_index actor_index);

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
    uint8_t *actor = ACTOR(actor_index);
    uint8_t *definition;

    if (B(0x2ec) == 0) {
        return 0;
    }
    definition = (uint8_t *)halo::cache::globals().tag_instances[D(0x58) & halo::k_slot_mask].data;
    if (!(F(0x1c0) > *(float *)(definition + 0x2ac))) {
        return 0;
    }
    if (W(0x308) == 0 || D(0x30c) == halo::k_dword_none) {
        D(0x30c) = actor_get_squad_recent_attacker_target(actor_index, 1);
    }
    if (W(0x308) <= 1) {
        W(0x308) = 1;
    }
    B(0x2ec) = 0;
    return 1;
}

extern "C" uint8_t actor_alert_from_squad_attack(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).alert_from_squad_attack();
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

namespace c_actor_check_pain_reaction {
extern "C" {
extern int32_t actor_build_order_grenade_or_melee(uint32_t resolved_target, uint8_t use_alt_base,
    uint32_t actor_index, uint16_t order_code, uint8_t byte_a, uint8_t byte_b, uint16_t *order);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
}
}

extern "C" uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code, datum_index actor_index);

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

    if (actor_build_order_grenade_or_melee(resolved_target, use_alt_base, actor_index, order_code,
            0, 0, order_buffer) != 0) {
        actor_set_mode(actor_index, 4, order_buffer);
        return 1;
    }
    return 0;
}

extern "C" uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code, datum_index actor_index)
{
    return halo::ai::alert_ops::check_pain_reaction(resolved_target, use_alt_base, order_code, actor_index);
}

namespace c_actor_combat_status_should_hold {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);

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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    if (self->mode_data.raw[8] != 0) {
        return (uint8_t)(threshold_b <= self->combat_status);
    }
    if (0 < *(int16_t *)&self->mode_data.raw[0] && self->combat_status < threshold_a &&
        (self->post_combat_action < 1 || self->mode_data.raw[5] != 0)) {
        return 0;
    }
    return 1;
}

extern "C" uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b)
{
    return halo::ai::alert_ops(actor_index).combat_status_should_hold(threshold_a, threshold_b);
}

namespace c_actor_conditional_state_transition_check {
extern "C" {
extern data_array *actor_data;
extern char actor_evaluate_combat_state_transition(uint32_t actor_index);
}
}

extern "C" uint8_t actor_conditional_state_transition_check(datum_index actor_index);

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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    if (self->mode != _actor_mode_vehicle) {
        return 0;
    }
    sub_state = *(int16_t *)&self->mode_data.raw[4];
    if (sub_state == 2 || sub_state == 3) {
        if (self->mode_data.raw[7] != 0 || self->mode_data.raw[8] != 0) {
            return actor_evaluate_combat_state_transition(actor_index);
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
    return actor_evaluate_combat_state_transition(actor_index);
}

extern "C" uint8_t actor_conditional_state_transition_check(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).conditional_state_transition_check();
}

namespace c_actor_consider_combat_mode {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

extern float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode, actor_combat_consideration *consideration);
extern int32_t actor_grenade_trace_from_source(uint32_t actor_index, real_point3d *target_point);
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
    float radius);
extern void actor_movement_actions_cancel(datum_index actor_index);
}
}

extern "C" uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode, actor_combat_consideration *out);

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
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((struct actor *)actor)->actor_definition_tag & halo::k_slot_mask].data;
    uint8_t *record = (uint8_t *)out;
    int16_t mode = consideration_mode;
    uint8_t result = 1;

    memset(out, 0, 0x38);
    *(int32_t *)record = game_time->game_time;

    if (mode == 5 || mode == 4) {
        ((struct actor_combat_consideration *)record)->mode = mode;
        return ((struct actor *)actor)->vehicle_driving_type > 1;
    }
    if (mode == 2) {
        uint8_t *unit;
        uint8_t *target;
        uint8_t leap = 0;
        int16_t frame_count = 0;
        int16_t key_frame = 0;
        float dx_to_key_frame = 0.0f;
        float dx_total = 0.0f;
        float wait;
        float limit;

        result = 0;
        if (actor[6] != 0) {
            goto done;
        }
        unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[((struct actor *)actor)->unit_index & halo::k_slot_mask].data;
        if ((unit[0x106] & 0x80) != 0 || ((struct actor *)actor)->target_unit_index == k_datum_index_none) {
            goto done;
        }
        target = (uint8_t *)prop_data->data + (((struct actor *)actor)->target_unit_index & halo::k_slot_mask) * k_prop_size;
        if (*(float *)(actor_tag + 0x388) == 0.0f || ((Actor *)actor_tag)->melee_leap_chance == 0.0f) {
            record[0xa] = 0;
        } else if (target[0x130] != 0 || *(int16_t *)(target + 0x9c) > 0) {
            record[0xa] = 1;
            leap = 1;
            mode = 3;
        } else {
            leap = halo::math::random_real() < ((Actor *)actor_tag)->melee_leap_chance;
            record[0xa] = leap;
            if (*(float *)(target + 0x11c) < *(float *)(actor_tag + 0x384)) {
                leap = 0;
            } else if (leap) {
                mode = 3;
            }
        }
        if (!halo::units::unit_get_weapon_marker_indices(((struct actor *)actor)->unit_index, leap, (uint32_t)&dx_to_key_frame,
                (uint32_t)&dx_total, &frame_count, &key_frame)) {
            goto done;
        }
        if ((*(uint32_t *)actor_tag & 0x8000000) != 0) {
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
        wait = actor_get_consideration_wait_threshold(actor_index, mode, out);
        ((struct actor_combat_consideration *)record)->wait_threshold = wait;
        limit = mode == 3 ? 4.0f : 1.5f;
        if (limit > wait) {
            wait = limit;
        }
        if (actor_movement_set_destination_near_target(((struct actor *)actor)->target_unit_index, actor_index, wait)) {
            actor_movement_actions_cancel(actor_index);
            if (actor_grenade_trace_from_source(actor_index, (real_point3d *)(target + 0xc8))) {
                result = 1;
            }
        }
        goto done;
    }
    if (mode == 0 && (*(uint32_t *)actor_tag & 0x20000) != 0 && ((struct actor *)actor)->combat_status >= 5 && actor[0x378] == 0) {
        mode = 1;
    }

done:
    ((struct actor_combat_consideration *)record)->mode = mode;
    return result;
}

extern "C" uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode, actor_combat_consideration *out)
{
    return halo::ai::alert_ops(actor_index).consider_combat_mode(consideration_mode, out);
}

namespace c_actor_escalate_apply {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag);
extern char actor_evaluate_combat_state_transition(uint32_t actor_index);
}
}

extern "C" uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);

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
    uint8_t *act = ACTOR(actor_index);
    uint8_t result = 0;

    if (*(int16_t *)(act + 0x310) >= threshold && !act[0x378]) {
        actor_set_combat_alert_flag(actor_index, 1);
        if (((struct actor *)act)->combat_status >= 4) {
            result = (uint8_t)actor_evaluate_combat_state_transition(actor_index);
        }
    }
    *(int16_t *)(act + 0x310) = 0;
    return result;
}

extern "C" uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold)
{
    return halo::ai::alert_ops(actor_index).escalate_apply(threshold);
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_check_leader_flag {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
}
}

extern "C" uint8_t actor_escalate_check_leader_flag(datum_index actor_index);

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
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);

    if ((*(uint32_t *)actor_tag & 0x80000) == 0 || act[0x1c9] || ((struct actor *)act)->combat_status < 5) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 1) {
        *(int16_t *)(act + 0x310) = 1;
    }
    return 1;
}

extern "C" uint8_t actor_escalate_check_leader_flag(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_leader_flag();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_check_shield_damage {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
}
}

extern "C" uint8_t actor_escalate_check_shield_damage(datum_index actor_index);

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
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);

    if (!act[0x2ec] || !(((struct actor *)act)->recent_body_damage > ((Actor *)actor_tag)->berserk_damage_amount) ||
        !(((struct actor *)act)->body_vitality < ((Actor *)actor_tag)->berserk_damage_threshold)) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 3) {
        *(int16_t *)(act + 0x310) = 3;
    }
    act[0x2ec] = 0;
    return 1;
}

extern "C" uint8_t actor_escalate_check_shield_damage(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_shield_damage();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_check_target_close {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
}
}

extern "C" uint8_t actor_escalate_check_target_close(datum_index actor_index);

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
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->combat_status < 5) {
        return 0;
    }
    if (!(((struct prop *)PROP(((actor *)act)->target_unit_index))->distance <
          *(float *)(TAG_DATA(((actor *)act)->actor_definition_tag) + 0x3a0))) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 2) {
        *(int16_t *)(act + 0x310) = 2;
    }
    return 1;
}

extern "C" uint8_t actor_escalate_check_target_close(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_target_close();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_check_weapon_range {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

extern void *actor_get_actor_definition(datum_index actor_index);
}
}

extern "C" uint8_t actor_escalate_check_weapon_range(datum_index actor_index);

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
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint8_t *definition = (uint8_t *)actor_get_actor_definition(actor_index);

    if (*(datum_index *)&((struct actor *)act)->stuck_projectile_index == k_datum_index_none || ((struct actor *)act)->combat_status < 5) {
        return 0;
    }
    if (!(((struct prop *)PROP(((actor *)act)->target_unit_index))->distance < *(float *)(definition + 0x16c))) {
        return 0;
    }
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    if (!((float)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f < ((Actor *)actor_tag)->berserk_grenade_chance)) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 4) {
        *(int16_t *)(act + 0x310) = 4;
    }
    return 1;
}

extern "C" uint8_t actor_escalate_check_weapon_range(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_check_weapon_range();
}

#undef ACTOR
#undef PROP
#undef TAG_DATA

namespace c_actor_escalate_to_guard_or_combat {
extern "C" {
extern data_array *actor_data;

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

extern uint8_t actor_build_guard_mode_data(datum_index actor_index, uint8_t *out);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
extern char actor_evaluate_combat_state_transition(uint32_t actor_index);
}
}

extern "C" uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);

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
    uint8_t *actor = ACTOR(actor_index);
    uint8_t record[0x84];

    if (!(W(0x6a) < 3) || W(0x312) == 0) {
        return 0;
    }
    W(0x6a) = 3;
    if (actor_build_guard_mode_data(actor_index, record)) {
        actor_set_mode(actor_index, 6, record);
    } else {
        actor_evaluate_combat_state_transition(actor_index);
    }
    W(0x312) = 0;
    return 1;
}

extern "C" uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index)
{
    return halo::ai::alert_ops(actor_index).escalate_to_guard_or_combat();
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

namespace c_actor_evaluate_combat_state_transition {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;

extern void *actor_get_actor_definition(datum_index actor_index);
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3);
extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag);
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);
extern uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode,
    actor_combat_consideration *out);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);

#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & halo::k_slot_mask].data)
}
}

extern "C" char actor_evaluate_combat_state_transition(uint32_t actor_index);

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
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *actor_tag = TAG_DATA(((actor *)a)->actor_definition_tag);
    uint8_t *variant = TAG_DATA(((actor *)a)->actor_variant_tag);
    uint8_t *definition = (uint8_t *)actor_get_actor_definition(actor_index);
    uint8_t changed = 0;
    uint8_t fallback = 0;
    uint8_t *p = 0;
    float distance = 3.4028235e+38f;
    actor_combat_consideration consideration;
    int16_t mode;
    uint8_t hold;

    if (((actor *)a)->target_unit_index != k_datum_index_none) {
        p = (uint8_t *)prop_data->data + (((actor *)a)->target_unit_index & halo::k_slot_mask) * k_prop_size;
        distance = *(float *)(p + 0x11c);

        if (((actor *)a)->mode == 0xa && *(int16_t *)(a + 0xa0) == 1) {
            uint8_t engaged = p[0x74] || (p[0x12f] && (int8_t)p[0x121] <= 1);

            if (!engaged && ((Actor *)actor_tag)->stalking_discovery_time > 0.0f &&
                !(*(int16_t *)(a + 0xc2) < (int16_t)(int32_t)(((Actor *)actor_tag)->stalking_discovery_time * 30.0f) )) {
                engaged = 1;
            }
            if (engaged) {
                if (!(distance <= *(float *)(definition + 0xa0))) {
                    changed = actor_handle_death(actor_index, 0, 0);
                    if (!changed) {
                        actor_set_combat_alert_flag(actor_index, 1);
                    }
                } else {
                    actor_set_combat_alert_flag(actor_index, 1);
                }
            }
        }

        if (!(actor_has_unshielded_threat_weapon(actor_index) &&
              (*(datum_index *)(p + 0x110) != k_datum_index_none || p[0x14])) &&
            !(((actor *)a)->mode == 0xa && (*(int16_t *)(a + 0xa0) == 2 || *(int16_t *)(a + 0xa0) == 3)) &&
            !changed && !a[0x6] && ((actor *)a)->active_unit_index == k_datum_index_none &&
            ((struct actor *)a)->firing_state != 2) {
            int32_t now = game_time->game_time;
            uint8_t wide = a[0x378];
            float base_delay;
            float delay;
            float range;
            int16_t difficulty = (int16_t)(uint16_t)halo::main::globals().game_globals->difficulty;

            if (!actor_has_unshielded_threat_weapon(actor_index) && !(*(uint32_t *)actor_tag & 0x20000)) {
                wide = 1;
            }
            base_delay = a[0x378] ? 0.0f : ((Actor *)actor_tag)->melee_attack_delay;
            delay = halo::game::weapon_get_zoom_fov(0x14, difficulty) + halo::game::weapon_get_zoom_fov(0x15, difficulty) * base_delay;
            range = wide ? ((ActorVariant *)variant)->berserk_melee_range : ((ActorVariant *)variant)->melee_range;
            if (!(*(int32_t *)&((actor *)a)->search_wait_time != -1 && *(int32_t *)&((actor *)a)->search_wait_time + 0xa >= now) &&
                distance <= range) {
                uint8_t near_enough = 1;

                if (a[0x1cb]) {
                    float extra = ((Actor *)actor_tag)->melee_fudge_factor;

                    if (!(0.0f <= extra)) {
                        extra = 0.0f;
                    }
                    near_enough = distance <= 0.8f + extra;
                }
                if (near_enough &&
                    (*(int32_t *)&((struct actor *)a)->last_melee_time == -1 || (float)now > delay * 30.0f + (float)*(int32_t *)&((struct actor *)a)->last_melee_time)) {
                    actor_has_unshielded_threat_weapon(actor_index);
                    *(int32_t *)&((actor *)a)->search_wait_time = now;
                    if (actor_consider_combat_mode(actor_index, 2, &consideration)) {
                        actor_set_mode(actor_index, 0xa, &consideration);
                        changed = 1;
                    }
                }
            }
        }

        if (((actor *)a)->mode != 0xa && !a[0x1cb]) {
            int16_t seat_kind = ((struct actor *)a)->vehicle_driving_type;

            if (changed) {
                return changed;
            }
            if (seat_kind > 0) {
                uint8_t ready = 1;

                if (*(int32_t *)&((struct actor *)a)->last_vehicle_charge_time != -1) {
                    uint8_t *vehicle_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(((actor *)a)->active_unit_index));

                    ready = (float)game_time->game_time >
                        *(float *)(vehicle_tag + 0x390) * 30.0f + (float)*(int32_t *)&((struct actor *)a)->last_vehicle_charge_time;
                }
                if (ready && seat_kind == 4 && distance > *(float *)(definition + 0x160) &&
                    *(int16_t *)(p + 0x38) == 0 &&
                    actor_consider_combat_mode(actor_index, 4, &consideration)) {
                    actor_set_mode(actor_index, 0xa, &consideration);
                    return 1;
                }
            }
        } else if (changed) {
            return changed;
        }
    }

    fallback = (a[0x375] && !a[0x1cb]) ? 1 : 0;
    hold = 0;
    if (!a[0x1cb] && !actor_has_unshielded_threat_weapon(actor_index) && (*(uint32_t *)actor_tag & 0x1000000)) {
        fallback = 1;
    }
    mode = ((actor *)a)->mode;
    if (mode == 0xa) {
        int16_t state = *(int16_t *)(a + 0xa0);

        if (state == 2 || state == 3) {
            if (!a[0xa3] && !a[0xa4] && !a[0xc5]) {
                fallback = 1;
                goto consider_zero;
            }
            hold = 1;
            goto decide;
        }
        if (a[0x1cb]) {
            goto guard;
        }
        if (state == 4 || state == 5) {
            if (a[0xc5] || ((struct actor *)a)->vehicle_driving_type <= 1) {
                hold = 1;
                goto decide;
            }
            fallback = 1;
            if (state != 4) {
                goto consider_zero;
            }
            {
                uint8_t *vehicle_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(((actor *)a)->active_unit_index));
                float vehicle_range = *(float *)(vehicle_tag + 0x394);

                if (a[0x484] && ((actor *)a)->active_movement.type == 5 &&
                    *(datum_index *)&((actor *)a)->active_movement.destination.x == ((actor *)a)->target_unit_index) {
                    goto guard;
                }
                if (distance < vehicle_range) {
                    goto guard;
                }
                if (!(vehicle_range + vehicle_range > distance)) {
                    goto consider_zero;
                }
                if (((actor *)a)->facing.k * *(float *)(p + 0xe8) + ((actor *)a)->facing.j * *(float *)(p + 0xe4) +
                        *(float *)(p + 0xe0) * ((actor *)a)->facing.i >= 0.5f) {
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
    if (actor_consider_combat_mode(actor_index, 0, &consideration)) {
        actor_set_mode(actor_index, 0xa, &consideration);
        changed = 1;
    } else {
        goto guard;
    }
settle:
    if (fallback || changed) {
        return changed;
    }
guard:
    if (((actor *)a)->mode == 3) {
        return changed;
    }
    *(uint32_t *)&consideration = 0;
    actor_set_mode(actor_index, 3, &consideration);
    return 1;
}

extern "C" char actor_evaluate_combat_state_transition(uint32_t actor_index)
{
    return halo::ai::alert_ops(actor_index).evaluate_combat_state_transition();
}

#undef OBJECT_DATA
#undef TAG_DATA

namespace c_actor_is_within_alert_range {
extern "C" {
extern data_array *actor_data;

}
}

extern "C" uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index);

/**
 * actor_is_within_alert_range: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_is_within_alert_range.c.txt.
 *
 * @address 0x408f30
 */
uint8_t halo::ai::alert_ops::is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index)
{
    using namespace c_actor_is_within_alert_range;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
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

extern "C" uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index)
{
    return halo::ai::alert_ops::is_within_alert_range(always_in_range, radius_a, radius_b, vitality_only, use_radius_b, actor_index, object_index);
}

