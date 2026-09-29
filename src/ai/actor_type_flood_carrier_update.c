// actor_type_flood_carrier_update  (not a Ghidra function; the "flood carrier" actor type's update procedure, actor
//   type table record 0x685238 +0x14; no C existed, so a carrier form trapped as unlisted_423740)
// address 0x423740, size 299 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x423740..0x42386a (jump table 0x42386c, modes 3..11; 12/13 do nothing). A
//   pending order request when idle, the command list and react-to-disturbance(4); unless reloading or swapping,
//   the leader-flag, shield-damage and target-close escalation checks, escalate(1) and guard-or-combat (no danger
//   avoidance). Per mode: 3/10 combat (1, 0) else the conditional transition else the grenade and morale
//   reactions; 4 flee: combat (1, 1) when +0xaa is set, else look away; 5/7/8 combat (1, 0) else the special-mode
//   update; 6 guard: combat (should hold (3, 6), 0); 11 obey: combat (+0x9e, +0xa1); 9 vehicle: nothing.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code); // 0x409ea0


extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index); // 0x40ab80, EAX


#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_type_flood_carrier_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, 0xffff);
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
