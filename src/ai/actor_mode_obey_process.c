// actor_mode_obey_process  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x407340, size 190 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: actor_mode_definitions[11] ("obey") +0x0c slot (actor_update_activation_state 0x4291f5 calls it).
// objdump 0x407340..0x4073fd: every member runs its command list (actor_squad_action_list_process 0x406e30 as the
//   swarm callback, extra = a flag byte that starts at 1; EDI = mode data +0x9c). When the flag survived and the
//   list is not yet marked done (mode data +5): unless the list (scenario +0x43c, 0x60 each) has flag 0x10 and the
//   actor has +0x15c set with a variant (+0x58) lacking flag 0x200000, the done time (+0x94) takes the game time
//   and +5 is set. Returns 1 when the actor is in mode 11 and the list is done.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern uint8_t *global_scenario;     // 0x00746f8c
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback,
    uint32_t callback_extra, uint16_t *caller_record); // 0x407040, EDI caller_record
extern void actor_squad_action_list_process(uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index,
    uint8_t *state, uint8_t *aim_state, uint8_t *out); // 0x406e30

uint8_t actor_mode_obey_process(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *mode_data = actor + 0x9c;
    uint8_t still_running = 1;

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)actor_squad_action_list_process,
        (uint32_t)&still_running, (uint16_t *)mode_data);
    if (still_running && mode_data[5] == 0) {
        uint8_t *list = *(uint8_t **)(global_scenario + 0x43c) + *(int16_t *)mode_data * 0x60;
        int mark = 1;

        if ((list[0x20] & 0x10) && actor[0x15c] != 0) {
            uint8_t *variant = (uint8_t *)tag_instances[*(datum_index *)(actor + 0x58) & 0xffff].data;

            if ((*(uint32_t *)variant & 0x200000) == 0) {
                mark = 0;
            }
        }
        if (mark) {
            *(int32_t *)(actor + 0x94) = game_time->game_time;
            mode_data[5] = 1;
        }
    }
    return (uint8_t)(*(int16_t *)(actor + 0x6c) == 0xb && mode_data[5] != 0);
}
