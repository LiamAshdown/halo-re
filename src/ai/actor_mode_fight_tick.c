// actor_mode_fight_tick  (not a Ghidra function; AI "fight" mode)
// address 0x403540, size 100 bytes
// name confidence: 0.4  rewrite confidence: 0.9
// evidence: actor_mode_definitions[3] ("fight") +0x10 slot.
// objdump 0x403540..0x4035a3: the fight mode's countdown (+0x9c) runs down while the actor is moving under its own
//   control (+0x484); when it reaches 0 the current firing position (+0x3b8), unless already recorded (+0x3ba),
//   goes into the recognition history as type 0 (tail call 0x4141a0: EAX actor, CX position, DL 0).
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360

extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL

void actor_mode_fight_tick(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    int16_t countdown = *(int16_t *)&((struct actor *)actor)->mode_data;

    if (countdown <= 0 || actor[0x484] == 0) {
        return;
    }
    countdown = (int16_t)(countdown - 1);
    *(int16_t *)&((struct actor *)actor)->mode_data = countdown;
    if (countdown == 0 && *(uint16_t *)&((struct actor *)actor)->firing_position_index != 0xffff && actor[0x3ba] == 0) {
        actor_push_recognition_entry(actor_index, ((struct actor *)actor)->firing_position_index, 0);
    }
}
