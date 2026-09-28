// actor_type_sentinel_update  (not a Ghidra function; the "sentinel" actor type's update procedure, actor type table
//   record 0x685398 +0x14; no C existed, so the first sentinel (343 Guilty Spark onwards) trapped as unlisted_4264d0)
// address 0x4264d0, size 357 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4264d0..0x426634 (jump table 0x426638, modes 3..13). A pending order request
//   when idle, the command list and react-to-disturbance(1); unless reloading or swapping, guard-or-combat and
//   danger avoidance. Per mode: 3/10 combat (1, 0) else the conditional transition else the grenade and morale
//   reactions (tail jump); 4 flee: look away unless +0xaa is set (then combat (1, 1)); 5/7/8 combat (1, 0) else the
//   special-mode update; 6 guard: combat (should hold (3, 6), 0); 11 obey: combat (+0x9e, +0xa1); 12 converse:
//   combat (command list permits escalation, +0xa0 set or no +0x1dc); 13 avoid: combat (1, 1) while +0x280 is 0;
//   9 vehicle: nothing.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code); // 0x409ea0
extern uint8_t actor_process_pending_command_list(datum_index actor_index); // 0x40a140
extern uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold); // 0x40a1e0
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index); // 0x40ab80, EAX
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index); // 0x40aaf0, EDI
extern uint8_t actor_update_danger_avoidance(datum_index actor_index); // 0x40c040
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2); // 0x40d610, EDI
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index); // 0x40d7a0, ESI
extern char actor_update_grenade_and_morale_reactions(uint32_t actor_index); // 0x40b920, EAX
extern uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b); // 0x40d520
extern uint32_t actor_flee_look_away(datum_index actor_index); // 0x40d4c0, EDI
extern uint8_t actor_update_special_mode(datum_index actor_index); // 0x40d820, EAX
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index); // 0x40d580, EAX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_type_sentinel_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (*(int16_t *)(act + 0x6c) == 0 && *(int16_t *)(act + 0x6a) != 0) {
        actor_process_order_request(actor_index, 0xffff);
    }
    actor_process_pending_command_list(actor_index);
    actor_react_to_disturbance(actor_index, 1);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (*(int16_t *)(act + 0x6c)) {
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
            (uint8_t)(act[0xa0] != 0 || *(datum_index *)(act + 0x1dc) == k_datum_index_none));
        break;
    case 13:
        if (*(int16_t *)(act + 0x280) == 0) {
            actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}
