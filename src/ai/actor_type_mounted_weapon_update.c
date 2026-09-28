// actor_type_mounted_weapon_update  (not a Ghidra function; no C existed, so a call hit the unlisted_4263f0 trap)
// address 0x4263f0, size 190 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4263f0..0x4264ad.
//   The mounted_weapon actor type's update (actor type table 0x685378 +0x14, name "mounted_weapon"). As the other
//   type updates: a pending order request when idle (mode 0 with +0x6a set), the command list, then reload/swap or
//   escalation, and a per-mode step through the (mode - 3) byte table at 0x4264c0 {0,0,1,0,1,1,3,0,2}:
//   modes 3/4/6/10 combat behaviour (1, 0) else the conditional state transition; modes 5/7/8 combat behaviour
//   (1, 0) else the special-mode update; mode 11 combat behaviour with the actor's +0x9e / +0xa1 bytes.
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
extern uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index); // 0x40aaf0, EDI
extern uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2); // 0x40d610, EDI
extern uint8_t actor_conditional_state_transition_check(datum_index actor_index); // 0x40d7a0, ESI
extern uint8_t actor_update_special_mode(datum_index actor_index); // 0x40d820, EAX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_type_mounted_weapon_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->mode == 0 && ((actor *)act)->awareness_level != 0) {
        actor_process_order_request(actor_index, 0xffff);
    }
    actor_process_pending_command_list(actor_index);
    if (actor_wants_reload_or_swap(actor_index) == 0) {
        actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (((actor *)act)->mode) { // re-read after the calls above (0x426442)
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
            actor_update_special_mode(actor_index); // 0x42648c: tail jump
        }
        break;
    case 11:
        actor_update_combat_behavior(actor_index, act[0x9e], act[0xa1]);
        break;
    default:
        break;
    }
}
