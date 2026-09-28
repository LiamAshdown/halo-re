// actor_mode_wait_tick  (not a Ghidra function: actor mode table 0x65524c, mode "wait" slot +0x18)
// address 0x409cc0, size 241 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x409cc0..0x409db1 (no C existed: a mode entered in game would have hit a trap).
//   Per tick while waiting: when the chatter timer (+0xac) runs out the actor says it is waiting (event 0x11) and
//   the timer restarts at 300..599 ticks; when the wait (+0xaa) runs out it says so if asked to (+0x9d, event 0x14)
//   and marks the wait over (+0x9c); the wait kind countdown (+0xa8) runs unless held (+0x9f).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern uint32_t random_seed_global; // 0x00719cd0

void actor_mode_wait_tick(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    datum_index unit_index = ((actor *)act)->unit_index;

    if (*(int16_t *)(act + 0xac) > 0) {
        *(int16_t *)(act + 0xac) -= 1;
        if (*(int16_t *)(act + 0xac) == 0) {
            if (unit_index != k_datum_index_none) {
                ai_communication_broadcast(0x11, unit_index, -1, -1, -1, -1, 0);
            }
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            *(int16_t *)(act + 0xac) = (int16_t)((((random_seed_global >> 16) * 300) >> 16) + 300);
        }
    }
    if (*(int16_t *)(act + 0xaa) > 0) {
        *(int16_t *)(act + 0xaa) -= 1;
        if (*(int16_t *)(act + 0xaa) == 0) {
            if (act[0x9d] && unit_index != k_datum_index_none) {
                ai_communication_broadcast(0x14, unit_index, -1, -1, -1, -1, 0);
            }
            act[0x9c] = 1;
        }
    }
    if (!act[0x9f] && *(int16_t *)(act + 0xa8) > 0) {
        *(int16_t *)(act + 0xa8) -= 1;
    }
}
