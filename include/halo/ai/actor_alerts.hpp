#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#include <string.h>

namespace halo::ai {

/**
 * Behaviour group "alert_ops" of the actor AI: 18 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class alert_ops {
public:
    explicit alert_ops(datum_index value) : datum(value) {}

    uint8_t alert_from_damage();
    uint8_t alert_from_disturbance();
    uint8_t alert_from_flag_1b4();
    uint8_t alert_from_projectile();
    uint8_t alert_from_squad_attack();
    static uint8_t check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code, datum_index actor_index);
    uint8_t combat_status_should_hold(int16_t threshold_a, int16_t threshold_b);
    uint8_t conditional_state_transition_check();
    uint8_t consider_combat_mode(int16_t consideration_mode, actor_combat_consideration *out);
    uint8_t escalate_apply(int16_t threshold);
    uint8_t escalate_check_leader_flag();
    uint8_t escalate_check_shield_damage();
    uint8_t escalate_check_target_close();
    uint8_t escalate_check_weapon_range();
    uint8_t escalate_to_guard_or_combat();
    char evaluate_combat_state_transition();
    static uint8_t is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index);
    int32_t investigate_disturbance_update();

    datum_index datum;
};

}
