// actor_type_hunter_update  (not a Ghidra function; the "hunter" actor type's update procedure, actor type table
//   record 0x6852f8 +0x14; no C existed, so the first hunter (b30 onwards) trapped as unlisted_424810)
// address 0x424810, size 319 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x424810..0x42494e (jump table 0x424950, mode byte map 0x424968). As the other
//   type updates: a pending order request when idle (mode 0 with +0x6a set) and the command list; unless reloading
//   or swapping, the shield-damage and weapon-range escalation checks, escalate(3), guard-or-combat and danger
//   avoidance. Then per mode (3..13 -> {0,0,1,0,1,1,5,0,2,3,4}): 0 = fight/flee/guard/charge: combat (1, 0) else
//   the conditional transition; 1 = uncover/search/wait: combat (1, 0) else the special-mode update; 2 = obey:
//   combat with +0x9e / +0xa1; 3 = converse: combat (command list permits escalation, +0xa0 set or no +0x1dc);
//   4 = avoid: combat (1, 1) while +0x280 is 0; 5 = vehicle: nothing.
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
extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index); // 0x40ab80, EAX
extern uint8_t actor_escalate_check_shield_damage(datum_index actor_index); // 0x40a9e0, EAX
extern uint8_t actor_escalate_check_weapon_range(datum_index actor_index); // 0x40a860, EAX
extern uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold); // 0x40aa70, EDI, stack
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index); // 0x40aaf0, EDI
extern uint8_t actor_update_danger_avoidance(datum_index actor_index); // 0x40c040
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2); // 0x40d610, EDI
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index); // 0x40d7a0, ESI
extern uint8_t actor_update_special_mode(datum_index actor_index); // 0x40d820, EAX
extern uint8_t actor_command_list_permits_escalation(datum_index actor_index); // 0x40d580, EAX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_type_hunter_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (*(int16_t *)(act + 0x6c) == 0 && *(int16_t *)(act + 0x6a) != 0) {
        actor_process_order_request(actor_index, 0xffff);
    }
    actor_process_pending_command_list(actor_index);
    if (!actor_wants_reload_or_swap(actor_index)) {
        actor_escalate_check_shield_damage(actor_index);
        actor_escalate_check_weapon_range(actor_index);
        actor_escalate_apply(actor_index, 3);
        actor_escalate_to_guard_or_combat(actor_index);
        actor_update_danger_avoidance(actor_index);
    }

    switch (*(int16_t *)(act + 0x6c)) {
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
            actor_update_special_mode(actor_index); // 0x4248d2: tail jump
        }
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
