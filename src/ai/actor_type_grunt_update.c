// actor_type_grunt_update  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x424590, size 632 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x424590..0x424808.
//   The grunt actor type's update (actor type table 0x6852d8 +0x14). As marine (0x4261d0) without the escalation
//   checks, with the disturbance alert, and the jump gate (1, may broadcast when +0x203 > 0, 7). Fleeing: a grunt
//   able to panic (+0x247 > 0) turns a flee kind that is not a grenade throw (0x404340) into panic (+0xab); calm
//   and fighting it may throw a grenade (0x40dc30) instead of looking away.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code); // 0x409ea0


extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index); // 0x40ab80, EAX


#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)


void actor_type_grunt_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t may_broadcast = (uint8_t)((int8_t)act[0x203] > 0);
    uint8_t panics = (uint8_t)((int8_t)act[0x247] > 0);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, 0xffff);
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
    case 3:  // fight
    case 10: // charge
        if (actor_update_combat_behavior(actor_index, 1, 0) || actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6: // guard
        actor_update_combat_behavior(actor_index, actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4: // flee: a grunt that can panic (+0x247) panics for real unless its flee kind is a grenade throw
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
        if (((struct actor *)act)->mode_data.flee.panic == 0 && ((struct actor *)act)->alert_level >= 5) {
            actor_consider_grenade_throw(actor_index);
        }
        return;
    case 5: // uncover
    case 7: // search
    case 8: // wait
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9: // vehicle
        if (act[0xa5]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act[0xa6]) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11: // obey
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        return;
    case 12: // converse
        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act[0xa0] || ((actor *)act)->conversation_index == k_datum_index_none));
        return;
    case 13: // avoid
        if (((actor *)act)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}
