// actor_type_jackal_update  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x425f70, size 562 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x425f70..0x4261a2.
//   The jackal actor type's update (actor type table 0x685338 +0x14). As grunt, with the jump gate (1, 0, 4), grenade
//   evasion (0x40c530 1, 1) instead of grenade decisions, a shield-holding guard ambush and no vehicle reaction.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code); // 0x409ea0
extern uint8_t actor_process_pending_command_list(datum_index actor_index); // 0x40a140
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold); // 0x40a1e0
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index); // 0x40ab80, EAX
extern uint8_t actor_alert_from_disturbance(datum_index actor_index); // 0x40a3d0, EAX
extern uint8_t actor_alert_from_squad_attack(datum_index actor_index); // 0x40a460, EDX
extern uint8_t actor_alert_from_projectile(datum_index actor_index); // 0x40a5e0
extern uint8_t actor_alert_from_flag_1b4(datum_index actor_index); // 0x40a6b0, EAX
extern uint8_t actor_alert_from_damage(datum_index actor_index); // 0x40a500
extern uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast,
    int16_t broadcast_threshold); // 0x40a700, EAX
extern uint8_t actor_escalate_check_leader_flag(datum_index actor_index); // 0x40a7f0, EAX
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index); // 0x40a860, EAX
extern uint8_t actor_escalate_check_target_close(datum_index actor_index); // 0x40a950, EAX
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index); // 0x40a9e0, EAX
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold); // 0x40aa70, EDI, stack
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index); // 0x40aaf0, EDI
extern uint8_t actor_seek_vehicle_to_board(datum_index actor_index); // 0x40ac70
extern uint8_t actor_process_vehicle_seat_exit(datum_index actor_index); // 0x40b080, EAX
extern uint8_t actor_update_grenade_throw_decision(datum_index actor_index); // 0x40b770, EDI
extern uint8_t actor_update_danger_avoidance(datum_index actor_index); // 0x40c040
extern uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base); // 0x40c530, EAX, stack
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2); // 0x40d610, EDI
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index); // 0x40d7a0, ESI
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index); // 0x40b920, EAX
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b); // 0x40d520
extern uint32_t actor_flee_look_away(datum_index actor_index); // 0x40d4c0, EDI
extern uint8_t actor_update_special_mode(datum_index actor_index); // 0x40d820, EAX
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index); // 0x40d580, EAX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern tag_instance *tag_instances; // 0x0087bc14

void actor_type_jackal_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = (uint8_t *)tag_instances[((actor *)act)->actor_definition_tag & 0xffff].data;

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
        actor_gate_jump_traversal(actor_index, 1, 0, 4);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_try_grenade_evasion(actor_index, 1, 1);
        actor_seek_vehicle_to_board(actor_index);
        actor_process_vehicle_seat_exit(actor_index);
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
    case 6: // guard: an ambushing jackal (+0xa4, not +0xa5 / +0xa6) keeps its shield up while its damage (+0x1bc)
        // stays under the Actor tag's limit (+0x2e0 in combat, else +0x2e4), 30 more ticks at a time
        if (act[0xa4] && !act[0xa5] && !act[0xa6]) {
            float limit = ((struct actor *)act)->alert_level >= 4 ? ((Actor *)actor_tag)->attack_shield_fraction : ((Actor *)actor_tag)->pursue_shield_fraction;

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
    case 4: // flee
        if (act[0xaa]) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        actor_flee_look_away(actor_index);
        return;
    case 5: // uncover
    case 7: // search
    case 8: // wait
        if (actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        actor_update_special_mode(actor_index);
        return;
    case 9: // vehicle: nothing for jackals
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
