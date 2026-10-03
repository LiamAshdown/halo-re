#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

namespace halo::ai {

namespace actor_type_crew_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_alert_from_disturbance(datum_index actor_index);
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index);
extern uint8_t actor_alert_from_projectile(datum_index actor_index);
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index);
extern uint8_t actor_alert_from_damage(datum_index actor_index);
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
}
}

/**
 * Actor AI behaviour: type crew update.
 *
 * @address 0x423890
 */
void ActorView::type_crew_update()
{
    using namespace actor_type_crew_update_local;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    if (((struct actor *)actor)->mode == 0 && ((struct actor *)actor)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_alert_from_disturbance(actor_index);
        actor_alert_from_squad_attack(actor_index);
        actor_alert_from_projectile(actor_index);
        actor_alert_from_flag_1b4(actor_index);
        actor_alert_from_damage(actor_index);
        actor_gate_jump_traversal(actor_index, 1, 0, 9);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((struct actor *)actor)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (actor[0xaa] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9:
        if (actor[0xa5] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        if (actor[0xa6] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        actor_update_combat_behavior(actor_index, actor[0x9e], actor[0xa1]);
        return;
    case 12: {
        uint8_t forced = (actor[0xa0] != 0 || *(uint32_t *)&((struct actor *)actor)->conversation_index == halo::k_dword_none) ? 1 : 0;

        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index), forced);
        return;
    }
    case 13:
        if (((struct actor *)actor)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}

namespace actor_type_elite_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_alert_from_disturbance(datum_index actor_index);
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index);
extern uint8_t actor_alert_from_projectile(datum_index actor_index);
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index);
extern uint8_t actor_alert_from_damage(datum_index actor_index);
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_seek_vehicle_to_board(datum_index actor_index);
extern uint8_t actor_process_vehicle_seat_exit(datum_index actor_index);
extern uint8_t actor_update_grenade_throw_decision(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type elite update.
 *
 * @address 0x423a90
 */
void ActorView::type_elite_update()
{
    using namespace actor_type_elite_update_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((actor *)act)->actor_definition_tag & halo::k_slot_mask].data;

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 4);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_check_leader_flag(actor_index);
        actor_escalate_check_shield_damage(actor_index);
        actor_escalate_check_target_close(actor_index);
        actor_escalate_check_weapon_range(actor_index);
        actor_escalate_apply(actor_index, 1);
        if (!act[0x378]) {
            actor_alert_from_projectile(actor_index);
            actor_alert_from_flag_1b4(actor_index);
            actor_gate_jump_traversal(actor_index, 9, 0, 11);
        }
        actor_escalate_to_guard_or_combat(actor_index);
        actor_try_grenade_evasion(actor_index, 0, 0);
        actor_seek_vehicle_to_board(actor_index);
        actor_process_vehicle_seat_exit(actor_index);
        actor_update_grenade_throw_decision(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        if (act[0xa4] && !act[0xa5] && !act[0xa6]) {
            float limit = ((struct actor *)act)->combat_status >= 4 ? ((Actor *)actor_tag)->attack_shield_fraction : ((Actor *)actor_tag)->pursue_shield_fraction;

            if (limit > *(float *)(act + 0x1bc)) {
                act[0xa4] = 1;
                ((struct actor *)act)->mode_data.flee.panic = 30;
            } else {
                act[0xa4] = 0;
                ((struct actor *)act)->mode_data.flee.panic = 0;
            }
        }
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (act[0xaa]) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9:
        if (act[0xa5]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act[0xa6]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        return;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act[0xa0] || ((actor *)act)->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}

#undef ACTOR

namespace actor_type_engineer_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index);
extern uint8_t actor_alert_from_projectile(datum_index actor_index);
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index);
extern uint8_t actor_alert_from_damage(datum_index actor_index);
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_grenade_throw_decision(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type engineer update.
 *
 * @address 0x423d40
 */
void ActorView::type_engineer_update()
{
    using namespace actor_type_engineer_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_alert_from_squad_attack(actor_index);
        actor_alert_from_projectile(actor_index);
        actor_alert_from_flag_1b4(actor_index);
        actor_alert_from_damage(actor_index);
        actor_gate_jump_traversal(actor_index, 1, 0, 9);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_grenade_throw_decision(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act[0xaa] == 0) {
            actor_flee_look_away(actor_index);
        } else {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 9:
        if (act[0xa5] != 0 || act[0xa6] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
            (uint8_t)(act[0xa0] != 0 || ((actor *)act)->conversation_index == k_datum_index_none));
        break;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}

#undef ACTOR

namespace actor_type_flood_carrier_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type flood carrier update.
 *
 * @address 0x423740
 */
void ActorView::type_flood_carrier_update()
{
    using namespace actor_type_flood_carrier_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 4);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_check_leader_flag(actor_index);
        actor_escalate_check_shield_damage(actor_index);
        actor_escalate_check_target_close(actor_index);
        actor_escalate_apply(actor_index, 1);
        actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act[0xaa] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        } else {
            actor_flee_look_away(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    default:
        break;
    }
}

#undef ACTOR

namespace actor_type_flood_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type flood update.
 *
 * @address 0x423f30
 */
void ActorView::type_flood_update()
{
    using namespace actor_type_flood_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 4);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_check_shield_damage(actor_index);
        actor_escalate_apply(actor_index, 3);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act[0xaa] == 0) {
            actor_flee_look_away(actor_index);
        } else {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}

#undef ACTOR

namespace actor_type_grunt_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_alert_from_disturbance(datum_index actor_index);
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index);
extern uint8_t actor_alert_from_projectile(datum_index actor_index);
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index);
extern uint8_t actor_alert_from_damage(datum_index actor_index);
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_seek_vehicle_to_board(datum_index actor_index);
extern uint8_t actor_process_vehicle_seat_exit(datum_index actor_index);
extern uint8_t actor_update_grenade_throw_decision(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
extern int32_t actor_order_code_is_grenade_throw(int16_t order_code);
extern uint8_t actor_consider_grenade_throw(datum_index actor_index);
}
}

/**
 * Actor AI behaviour: type grunt update.
 *
 * @address 0x424590
 */
void ActorView::type_grunt_update()
{
    using namespace actor_type_grunt_update_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t may_broadcast = (uint8_t)((int8_t)act[0x203] > 0);
    uint8_t panics = (uint8_t)((int8_t)act[0x247] > 0);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_alert_from_disturbance(actor_index);
        actor_alert_from_squad_attack(actor_index);
        actor_alert_from_projectile(actor_index);
        actor_alert_from_flag_1b4(actor_index);
        actor_alert_from_damage(actor_index);
        actor_gate_jump_traversal(actor_index, 1, (char)may_broadcast, 7);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_seek_vehicle_to_board(actor_index);
        actor_process_vehicle_seat_exit(actor_index);
        actor_update_grenade_throw_decision(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (panics && ((struct actor *)act)->mode_data.flee.panic > 0 && !actor_order_code_is_grenade_throw(((struct actor *)act)->mode_data.flee.panic)) {
            act[0xab] = 1;
        }
        if (act[0xaa]) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        if (actor_flee_look_away(actor_index)) {
            return;
        }
        if (((struct actor *)act)->mode_data.flee.panic == 0 && ((struct actor *)act)->combat_status >= 5) {
            actor_consider_grenade_throw(actor_index);
        }
        return;
    case 5:
    case 7:
    case 8:
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9:
        if (act[0xa5]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act[0xa6]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        return;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act[0xa0] || ((actor *)act)->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}

#undef ACTOR

namespace actor_type_hunter_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type hunter update.
 *
 * @address 0x424810
 */
void ActorView::type_hunter_update()
{
    using namespace actor_type_hunter_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_check_shield_damage(actor_index);
        actor_escalate_check_weapon_range(actor_index);
        actor_escalate_apply(actor_index, 3);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 4:
    case 6:
    case 10:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_conditional_state_transition_check(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
            (uint8_t)(act[0xa0] != 0 || ((actor *)act)->conversation_index == k_datum_index_none));
        break;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}

#undef ACTOR

namespace actor_type_infection_swarm_update_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern game_time_globals *game_time;
extern const real_point3d *global_origin3d_pointer;
extern int32_t actor_pick_dialogue_variant_a(int16_t category);
extern int32_t actor_pick_dialogue_variant_b(int16_t category);
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
#define OBJECT(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & halo::k_slot_mask].data)
#define SWARM(h) ((uint8_t *)swarm_data->data + ((h) & halo::k_slot_mask) * k_swarm_size)
#define COMPONENT(h) ((uint8_t *)swarm_component_data->data + ((h) & halo::k_slot_mask) * k_swarm_component_size)
#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))
#define U16(p, o) (*(uint16_t *)((uint8_t *)(p) + (o)))
#define I16(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))
#define U32(p, o) (*(uint32_t *)((uint8_t *)(p) + (o)))
static uint32_t swarm_random_next(void)
{
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    return halo::math::globals().random_seed_global >> 16;
}
static void copy3(real_vector3d *out, const void *in)
{
    out->i = ((const float *)in)[0];
    out->j = ((const float *)in)[1];
    out->k = ((const float *)in)[2];
}
}
}

/**
 * Actor AI behaviour: type infection swarm update.
 *
 * @address 0x424c20
 */
void ActorView::type_infection_swarm_update()
{
    using namespace actor_type_infection_swarm_update_local;
    uint8_t *actor = ACTOR(actor_index);
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[U32(actor, 0x5c) & halo::k_slot_mask].data;
    uint8_t *swarm = SWARM(U32(actor, 0x28));
    int32_t picked = -1;
    int16_t member;

    if (I16(swarm, 0x8) > 0) {
        I16(swarm, 0x8)--;
    } else if (I16(actor, 0x6c) == 7 || I16(actor, 0x6c) == 10) {
        float delay = ((float)(int32_t)swarm_random_next() * 1.5259022e-05f);
        int16_t count = I16(swarm, 0x2);

        delay = (delay + delay + 6.0f) / (float)(int32_t)count * 30.0f;
        if (!(delay > 6.0f)) {
            delay = 6.0f;
        }
        I16(swarm, 0x8) = (int16_t)(int32_t)delay;
        picked = (int32_t)(((uint32_t)(int32_t)count * swarm_random_next()) >> 16);
    }

    for (member = 0; member < I16(swarm, 0x2); member++) {
        uint32_t unit = U32(swarm, 0x18 + member * 4);
        uint8_t *object = OBJECT(unit);
        uint8_t *component = COMPONENT(U32(swarm, 0x58 + member * 4));
        uint8_t *best_prop = 0;
        datum_index best_handle = k_datum_index_none;
        datum_index target = k_datum_index_none;
        int16_t behaviour = 0;
        uint8_t speed = 3;
        uint8_t control_byte_1 = 1;
        uint8_t aligned = 0;
        uint8_t target_close = 0;
        uint8_t moving = 0;
        uint8_t riding = 0;
        uint8_t fire = 0;
        real_vector3d up;
        real_vector3d desired;
        uint16_t flags;
        unit_control_data control;

        copy3(&up, object + 0x80);
        if (I16(object, 0xb4) == 0) {
            if (U32(object, 0x4d8) != (uint32_t)k_datum_index_none) {
                copy3(&up, object + 0x514);
            }
            riding = object[0x4cc] & 1;
        }

        if (I16(actor, 0x6e) >= 3) {
            float radius = F(definition, 0xa0);
            float best_score = 0.0f;
            float best_distance = 0.0f;
            datum_index prop_handle = U32(actor, 0x50);

            while (prop_handle != k_datum_index_none) {
                uint8_t *prop = PROP(prop_handle);
                datum_index this_handle = prop_handle;

                prop_handle = U32(prop, 0x8);
                if (F(prop, 0x50) > 0.0f) {
                    float dx = F(component, 0x4) - F(prop, 0xbc);
                    float dy = F(component, 0x8) - F(prop, 0xc0);
                    float dz = F(component, 0xc) - F(prop, 0xc4);
                    float distance = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
                    float score = 0.0f;

                    if (distance < radius) {
                        score = (1.0f - distance / radius) * 10.0f;
                    }
                    if (I16(prop, 0x24) >= 2 && I16(prop, 0x24) <= 3) {
                        score += (this_handle == U32(component, 0x14)) ? 7.0f : 5.0f;
                        if (prop[0x125] == 0) {
                            score += 5.0f;
                        }
                    }
                    if (score > best_score) {
                        best_score = score;
                        best_prop = prop;
                        best_handle = this_handle;
                        best_distance = distance;
                    }
                }
            }
            U32(component, 0x14) = best_handle;
            if (best_handle != k_datum_index_none && best_distance < F(definition, 0x160) &&
                I16(best_prop, 0x24) >= 2 && I16(best_prop, 0x24) <= 3) {
                target_close = 1;
            }
        } else {
            U32(component, 0x14) = best_handle;
        }

        switch (I16(actor, 0x6c)) {
        case 1:
            behaviour = 0;
            speed = 0;
            break;
        case 2:
            behaviour = 1;
            speed = 1;
            break;
        case 4:
            speed = (uint8_t)((I16(actor, 0xa8) > 0) * 2 + 3);
            if (U32(actor, 0xb8) != (uint32_t)k_datum_index_none) {
                behaviour = 5;
                target = U32(actor, 0xb8);
            }
            break;
        case 6:
            behaviour = 2;
            speed = 1;
            break;
        case 7:
            if (I16(actor, 0xa4) == 0 && U32(actor, 0x270) != (uint32_t)k_datum_index_none) {
                behaviour = 4;
                target = U32(actor, 0x270);
            } else {
                behaviour = 3;
            }
            speed = 3;
            break;
        case 10:
        case 11:
            speed = 3;
            behaviour = 3;
            if (I16(actor, 0x6c) == 11 && (component[0x2] & 0x8) != 0) {
                behaviour = 6;
            } else if (U32(component, 0x14) != (uint32_t)k_datum_index_none) {
                behaviour = (int16_t)((component[0x1a] != 0) + 4);
                control_byte_1 = 0;
                target = U32(component, 0x14);
            }
            break;
        default:
            break;
        }

        if (U32(object, 0x11c) == (uint32_t)k_datum_index_none) {
            component[0x18] = 0;
            if (component[0x1a] != 0) {
                component[0x1a]--;
            }
        } else {
            uint8_t *parent = OBJECT(U32(object, 0x11c));
            uint8_t parent_dead = (uint8_t)((parent[0x106] >> 2) & 1);
            uint8_t detach = 0;

            if (component[0x18] != 0xff) {
                component[0x18]++;
            }
            if (parent_dead) {
                if (U32(parent, 0x41c) != (uint32_t)k_datum_index_none &&
                    (int32_t)(U32(parent, 0x41c) + 0x4b) < *(int32_t *)((uint8_t *)game_time + 0xc) &&
                    best_prop != 0 && U32(best_prop, 0x18) != U32(object, 0x11c) &&
                    I16(best_prop, 0x24) >= 2 && I16(best_prop, 0x24) <= 3) {
                    detach = 1;
                }
            } else {
                uint8_t *parent_tag = (uint8_t *)halo::cache::globals().tag_instances[U32(parent, 0x0) & halo::k_slot_mask].data;

                if ((I16(parent, 0xb4) != 0 || (int8_t)parent_tag[0x17d] < 0) && component[0x18] > 0x2d) {
                    component[0x1a] = 0x2d;
                    detach = 1;
                }
            }
            if (detach) {
                halo::units::unit_detach_reposition_and_nudge(unit);
                component[0x2] &= 0xfc;
            } else {
                component[0x2] |= 2;
                if (parent_dead) {
                    U16(component, 0x2) &= 0xfffe;
                } else {
                    U16(component, 0x2) |= 1;
                }
            }
        }

        if (U32(object, 0x11c) != (uint32_t)k_datum_index_none) {
            goto flags;
        }
        if (riding) {
            component[0x2] &= 0xfd;
            component[0x19] = 0;
            goto flags;
        }
        if (component[0x19] != 0xff) {
            component[0x19]++;
        }
        component[0x2] &= 0xfc;
        flags = U16(component, 0x2);

        switch (behaviour) {
        case 1:
        case 2:
        case 3:
            if ((flags & 4) == 0) {
                memset(component + 0x1c, 0, 0x14);
                U16(component, 0x2) = (uint16_t)((flags & 0xfff7) | 4);
            }
            if (component[0x1d] != 0) {
                component[0x1d]--;
                if (component[0x1d] == 0) {
                    component[0x1c] = (uint8_t)actor_pick_dialogue_variant_a(behaviour);
                } else {
                    float damping = F(component, 0x2c) * -0.06666667f;
                    float angle = halo::math::random_real_range(-0.020943951f, 0.020943951f) + F(component, 0x2c) + damping;

                    F(component, 0x2c) = angle;
                    halo::math::vector3d_rotate_about_axis(*(real_vector3d *)(component + 0x20), up, (float)sin((double)angle),
                        (float)cos((double)angle));
                }
            } else {
                if (component[0x1c] != 0) {
                    component[0x1c]--;
                }
                if (component[0x1c] == 0) {
                    real_vector3d to_goal;
                    float distance_squared;
                    float angle;

                    component[0x1d] = (uint8_t)actor_pick_dialogue_variant_b(behaviour);
                    to_goal.i = F(swarm, 0xc) - F(component, 0x4);
                    to_goal.j = F(swarm, 0x10) - F(component, 0x8);
                    to_goal.k = F(swarm, 0x14) - F(component, 0xc);
                    distance_squared = to_goal.k * to_goal.k + to_goal.j * to_goal.j + to_goal.i * to_goal.i;
                    if (!(distance_squared < 0.25f)) {
                        float spread = 0.5f / (float)sqrt((double)distance_squared) * 3.1415927f;

                        angle = halo::math::random_real_range(-spread, spread);
                        *(real_vector3d *)(component + 0x20) = to_goal;
                    } else {
                        angle = halo::math::random_real_range(-3.1415927f, 3.1415927f);
                        copy3((real_vector3d *)(component + 0x20), object + 0x74);
                    }
                    halo::math::vector3d_rotate_about_axis(*(real_vector3d *)(component + 0x20), up, (float)sin((double)angle),
                        (float)cos((double)angle));
                    F(component, 0x2c) = 0.0f;
                }
            }
            if (component[0x1d] == 0) {
                goto flags;
            }
            copy3(&desired, component + 0x20);
            moving = 1;
            break;
        case 4:
        case 5: {
            uint8_t *prop = PROP(target);

            desired.i = F(prop, 0xbc) - F(component, 0x4);
            desired.j = F(prop, 0xc0) - F(component, 0x8);
            desired.k = F(prop, 0xc4) - F(component, 0xc);
            if (behaviour == 5) {
                desired.i = -desired.i;
                desired.j = -desired.j;
                desired.k = -desired.k;
            }
            moving = 1;
            break;
        }
        case 6: {
            uint8_t script_flags = component[0x21];

            if (script_flags & 1) {
                int16_t kind = I16(component, 0x24);
                uint8_t negate;

                moving = 1;
                if (kind >= 2 && kind <= 3) {
                    halo::math::vector3d_cross_product(desired, *(real_vector3d *)(component + 0x28), up);
                    negate = (uint8_t)(kind == 3);
                } else {
                    copy3(&desired, component + 0x28);
                    negate = (uint8_t)(kind == 1);
                }
                if (negate) {
                    desired.i = -desired.i;
                    desired.j = -desired.j;
                    desired.k = -desired.k;
                }
            }
            if (script_flags & 4) {
                if ((script_flags & 8) == 0 && I16(component, 0x24) == 0 && !halo::units::unit_is_in_busy_animation_state(unit)) {
                    U16(component, 0x2) = (uint16_t)(flags | 0x10);
                    component[0x21] = (uint8_t)(script_flags | 8);
                }
                copy3(&desired, object + 0x74);
                moving = 1;
            } else if (!moving) {
                goto flags;
            }
            break;
        }
        default:
            goto flags;
        }

        {
            float length = (float)sqrt((double)(desired.k * desired.k + desired.j * desired.j + desired.i * desired.i));
            float along_up;

            if (!(fabs((double)length) < 0.0001)) {
                float inverse = 1.0f / length;

                desired.i *= inverse;
                desired.j *= inverse;
                desired.k *= inverse;
            }
            along_up = up.k * desired.k + up.j * desired.j + up.i * desired.i;
            if (along_up > 0.9f) {
                aligned = 1;
            }
            if (along_up < -0.9f) {
                copy3(&desired, object + 0x74);
            } else {
                real_vector3d side;

                side.i = up.j * desired.k - up.k * desired.j;
                side.j = up.k * desired.i - desired.k * up.i;
                side.k = desired.j * up.i - up.j * desired.i;
                desired.i = side.j * up.k - side.k * up.j;
                desired.j = side.k * up.i - up.k * side.i;
                desired.k = up.j * side.i - side.j * up.i;
                if (halo::math::vector3d_normalize_with_length(desired) == 0.0f) {
                    copy3(&desired, object + 0x74);
                }
            }
        }

        if (behaviour != 6) {
            float turn = 0.0f;
            real_point3d behind;
            real_vector3d side;
            int16_t other;

            behind.x = F(component, 0x4) - desired.i * 0.2f;
            behind.y = F(component, 0x8) - desired.j * 0.2f;
            behind.z = F(component, 0xc) - desired.k * 0.2f;
            side.i = up.j * desired.k - up.k * desired.j;
            side.j = up.k * desired.i - desired.k * up.i;
            side.k = desired.j * up.i - up.j * desired.i;
            if (I16(swarm, 0x2) > 0) {
                for (other = 0; other < I16(swarm, 0x2); other++) {
                    uint8_t *other_component;
                    float dx, dy, dz, distance_squared, facing;

                    if (other == member) {
                        continue;
                    }
                    other_component = COMPONENT(U32(swarm, 0x58 + other * 4));
                    dx = F(other_component, 0x4) - behind.x;
                    dy = F(other_component, 0x8) - behind.y;
                    dz = F(other_component, 0xc) - behind.z;
                    distance_squared = dz * dz + dy * dy + dx * dx;
                    if (!(distance_squared < 0.64000005f)) {
                        continue;
                    }
                    facing = (dz * desired.k + dy * desired.j + dx * desired.i) / (float)sqrt((double)distance_squared);
                    if (!(facing > 0.5f)) {
                        continue;
                    }
                    if (side.k * dz + side.j * dy + side.i * dx > 0.0f) {
                        turn = turn - (facing - 0.5f) * 0.5f;
                    } else {
                        turn = turn + (facing - 0.5f) * 0.5f;
                    }
                }
                if (turn != 0.0f) {
                    if (!(turn <= 1.0f)) {
                        turn = 1.5707964f;
                    } else if (turn < -1.0f) {
                        turn = -1.5707964f;
                    } else {
                        turn = turn * 1.5707964f;
                    }
                    halo::math::vector3d_rotate_about_axis(desired, up, (float)sin((double)turn), (float)cos((double)turn));
                }
            }
        }

        {
            real_vector3d side;
            float length;

            side.i = up.j * desired.k - up.k * desired.j;
            side.j = up.k * desired.i - desired.k * up.i;
            side.k = desired.j * up.i - up.j * desired.i;
            length = (float)sqrt((double)(side.k * side.k + side.j * side.j + side.i * side.i));
            if (!(fabs((double)length) < 0.0001) && length != 0.0f) {
                float inverse = 1.0f / length;
                float side_k = inverse * side.k;

                side.i *= inverse;
                side.j *= inverse;
                desired.i = side.j * up.k - side_k * up.j;
                desired.j = side_k * up.i - up.k * side.i;
                desired.k = up.j * side.i - side.j * up.i;
            } else {
                copy3(&desired, object + 0x74);
            }
        }

    flags:
        flags = U16(component, 0x2);
        if (flags & 0x10) {
            fire = 1;
        } else if (moving && component[0x19] >= 0x2d && (member == (int16_t)picked || target_close || aligned)) {
            fire = 1;
        }
        if (flags & 2) {
            object[0x289] = (uint8_t)((flags & 1) << 2);
        } else {
            if (target_close && component[0x1a] == 0) {
                flags |= 1;
            } else {
                flags &= 0xfffe;
            }
            U16(component, 0x2) = flags;
            if (flags & 1) {
                object[0x289] = 3;
                U32(object, 0x4f4) = best_prop != 0 ? U32(best_prop, 0x18) : (uint32_t)k_datum_index_none;
            } else {
                object[0x289] = 0;
            }
        }

        memset(&control, 0, sizeof control);
        ((uint8_t *)&control)[0x0] = speed;
        ((uint8_t *)&control)[0x1] = control_byte_1;
        U16(&control, 0x2) = (uint16_t)(fire ? 2 : 0);
        I16(&control, 0x4) = -1;
        I16(&control, 0x6) = -1;
        I16(&control, 0x8) = -1;
        if (moving) {
            F(&control, 0xc) = 1.0f;
            F(&control, 0x10) = 0.0f;
            F(&control, 0x14) = 0.0f;
        } else {
            copy3((real_vector3d *)((uint8_t *)&control + 0xc), global_origin3d_pointer);
            copy3(&desired, object + 0x74);
        }
        *(real_vector3d *)((uint8_t *)&control + 0x1c) = desired;
        *(real_vector3d *)((uint8_t *)&control + 0x28) = desired;
        *(real_vector3d *)((uint8_t *)&control + 0x34) = desired;
        halo::units::unit_apply_control_block(unit, &control, -1);
    }
}

#undef ACTOR
#undef PROP
#undef OBJECT
#undef SWARM
#undef COMPONENT
#undef F
#undef U16
#undef I16
#undef U32

namespace actor_type_infection_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type infection update.
 *
 * @address 0x424980
 */
void ActorView::type_infection_update()
{
    using namespace actor_type_infection_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_conditional_state_transition_check(actor_index);
        }
        break;
    case 4:
        if (act[0xaa] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        } else {
            actor_flee_look_away(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    default:
        break;
    }
}

#undef ACTOR

namespace actor_type_jackal_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_alert_from_disturbance(datum_index actor_index);
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index);
extern uint8_t actor_alert_from_projectile(datum_index actor_index);
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index);
extern uint8_t actor_alert_from_damage(datum_index actor_index);
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_seek_vehicle_to_board(datum_index actor_index);
extern uint8_t actor_process_vehicle_seat_exit(datum_index actor_index);
extern uint8_t actor_update_grenade_throw_decision(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type jackal update.
 *
 * @address 0x425f70
 */
void ActorView::type_jackal_update()
{
    using namespace actor_type_jackal_update_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((actor *)act)->actor_definition_tag & halo::k_slot_mask].data;

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_alert_from_disturbance(actor_index);
        actor_alert_from_squad_attack(actor_index);
        actor_alert_from_projectile(actor_index);
        actor_alert_from_flag_1b4(actor_index);
        actor_alert_from_damage(actor_index);
        actor_gate_jump_traversal(actor_index, 1, 0, 4);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_try_grenade_evasion(actor_index, 1, 1);
        actor_seek_vehicle_to_board(actor_index);
        actor_process_vehicle_seat_exit(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        if (act[0xa4] && !act[0xa5] && !act[0xa6]) {
            float limit = ((struct actor *)act)->combat_status >= 4 ? ((Actor *)actor_tag)->attack_shield_fraction : ((Actor *)actor_tag)->pursue_shield_fraction;

            if (limit > *(float *)(act + 0x1bc)) {
                act[0xa4] = 1;
                ((struct actor *)act)->mode_data.flee.panic = 30;
            } else {
                act[0xa4] = 0;
                ((struct actor *)act)->mode_data.flee.panic = 0;
            }
        }
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (act[0xaa]) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9:
        return;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        return;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act[0xa0] || ((actor *)act)->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}

#undef ACTOR

namespace actor_type_marine_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_alert_from_disturbance(datum_index actor_index);
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index);
extern uint8_t actor_alert_from_projectile(datum_index actor_index);
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index);
extern uint8_t actor_alert_from_damage(datum_index actor_index);
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold);
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index);
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index);
extern uint8_t actor_escalate_check_target_close(datum_index actor_index);
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index);
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_seek_vehicle_to_board(datum_index actor_index);
extern uint8_t actor_process_vehicle_seat_exit(datum_index actor_index);
extern uint8_t actor_update_grenade_throw_decision(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type marine update.
 *
 * @address 0x4261d0
 */
void ActorView::type_marine_update()
{
    using namespace actor_type_marine_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_alert_from_squad_attack(actor_index);
        actor_alert_from_projectile(actor_index);
        actor_alert_from_flag_1b4(actor_index);
        actor_alert_from_damage(actor_index);
        actor_gate_jump_traversal(actor_index, 1, 0, 14);
        actor_escalate_check_shield_damage(actor_index);
        actor_escalate_apply(actor_index, (int16_t)((int8_t)act[0x20a] > 2 ? 5 : 3));
        actor_escalate_to_guard_or_combat(actor_index);
        actor_seek_vehicle_to_board(actor_index);
        actor_process_vehicle_seat_exit(actor_index);
        actor_update_grenade_throw_decision(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (act[0xaa]) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9:
        if (act[0xa5]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act[0xa6]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        return;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act[0xa0] || ((actor *)act)->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}

#undef ACTOR

namespace actor_type_mounted_weapon_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type mounted weapon update.
 *
 * @address 0x4263f0
 */
void ActorView::type_mounted_weapon_update()
{
    using namespace actor_type_mounted_weapon_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    if (actor_wants_reload_or_swap(actor_index) == 0) {
        actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 4:
    case 6:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) == 0) {
            actor_conditional_state_transition_check(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (actor_update_combat_behavior(actor_index, 1, 0) == 0) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    default:
        break;
    }
}

#undef ACTOR

namespace actor_type_sentinel_update_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern uint8_t actor_process_pending_command_list(datum_index actor_index);
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold);
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index);
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index);
extern uint8_t actor_update_danger_avoidance(datum_index actor_index);
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2);
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index);
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index);
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b);
extern uint32_t actor_flee_look_away(datum_index actor_index);
extern uint8_t actor_update_special_mode(datum_index actor_index);
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index);
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: type sentinel update.
 *
 * @address 0x4264d0
 */
void ActorView::type_sentinel_update()
{
    using namespace actor_type_sentinel_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, halo::k_word_none);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((actor *)act)->mode) {
    case 3:
    case 10:
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act[0xaa] == 0) {
            actor_flee_look_away(actor_index);
        } else {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!actor_update_combat_behavior(actor_index, 1, 0)) {
            actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    case 12:
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
            (uint8_t)(act[0xa0] != 0 || ((actor *)act)->conversation_index == k_datum_index_none));
        break;
    case 13:
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}

#undef ACTOR

}
