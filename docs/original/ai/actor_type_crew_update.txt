// actor_type_crew_update  (not a Ghidra function; the "crew" actor type's update proc)
// address 0x423890, size 454 bytes
// name confidence: 0.5  rewrite confidence: 0.85
// evidence: actor type table 0x006853b8[8] ("crew", 0x00685258) +0x14 holds 0x423890.
// objdump 0x423890..0x423a55 (jump table 0x423a58, modes 3..13):
//   - an idle (mode 0) aware actor processes a pending order (actor_process_order_request(actor, -1)); a queued
//     command list may start (0x40a140) and a disturbance of at least 1 may turn the actor (0x40a1e0);
//   - unless it wants to reload: the alert checks (disturbance, squad attack, projectile, flag 0x1b4, damage), the
//     jump gate (actor_gate_jump_traversal 1, 0, 9), guard/combat escalation and danger avoidance;
//   - then per mode: fight/charge run the combat behaviour (1, 0), else a state transition, else the grenade and
//     morale reactions; guard runs it with (should_hold(3, 6), 0); flee (unless +0xaa) looks away; uncover, search
//     and wait run (1, 0) then the special mode update; vehicle runs (1, 1) when +0xa6 unless +0xa5 forces it;
//     obey runs (+0x9e, +0xa1); converse runs (permits_escalation, +0xa0 set or no conversation +0x1dc);
//     avoid runs (1, 1) once the danger (+0x280) is over.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
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
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index); // 0x40aaf0, EDI
extern uint8_t actor_update_danger_avoidance(datum_index actor_index); // 0x40c040
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2); // 0x40d610, EDI
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index); // 0x40d7a0, ESI
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index); // 0x40b920, EAX
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b); // 0x40d520
extern uint32_t actor_flee_look_away(datum_index actor_index); // 0x40d4c0, EDI
extern uint8_t actor_update_special_mode(datum_index actor_index); // 0x40d820, EAX
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index); // 0x40d580, EAX

void actor_type_crew_update(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    if (((struct actor *)actor)->mode == 0 && ((struct actor *)actor)->awareness_level != 0) {
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
        actor_gate_jump_traversal(actor_index, 1, 0, 9);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (((struct actor *)actor)->mode) {
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
    case 4: // flee
        if (actor[0xaa] != 0) {
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
    case 9: // vehicle
        if (actor[0xa5] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        if (actor[0xa6] != 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11: // obey
        actor_update_combat_behavior(actor_index, actor[0x9e], actor[0xa1]);
        return;
    case 12: { // converse
        uint8_t forced = (actor[0xa0] != 0 || *(uint32_t *)&((struct actor *)actor)->conversation_index == 0xffffffff) ? 1 : 0;

        actor_update_combat_behavior(actor_index, actor_command_list_permits_escalation(actor_index), forced);
        return;
    }
    case 13: // avoid
        if (((struct actor *)actor)->danger_type == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
