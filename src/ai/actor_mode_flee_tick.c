// actor_mode_flee_tick  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x18)
// address 0x403af0, size 155 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x403af0..0x403b8b (no C existed: a mode entered in game would have hit a trap).
//   Per tick while fleeing: time fled (+0xb4) counts up, the flee delay (+0x9c) and the cower time (+0x9e) count
//   down; when cowering ends a panicking actor (kinds 9..12 at +0xa8) starts looking around (0x570650, EAX unit);
//   with a flee kind the no-return-to-combat time (+0x39c) is pushed 750 ticks ahead.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern game_time_globals *game_time; // 0x006f1d6c
extern void unit_initialize_random_turn_angle(uint32_t object_index); // 0x570650, EAX

void actor_mode_flee_tick(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    *(int32_t *)(act + 0xb4) += 1;
    if (*(int16_t *)(act + 0x9c) > 0) {
        *(int16_t *)(act + 0x9c) -= 1;
    }
    if (*(int16_t *)(act + 0x9e) > 0) {
        *(int16_t *)(act + 0x9e) -= 1;
        if (*(int16_t *)(act + 0x9e) == 0 && ((actor *)act)->unit_index != k_datum_index_none &&
            *(int16_t *)(act + 0xa8) >= 9 && *(int16_t *)(act + 0xa8) <= 12) {
            unit_initialize_random_turn_angle(((actor *)act)->unit_index);
        }
    }
    if (*(int16_t *)(act + 0xa8) > 0) {
        *(int32_t *)(act + 0x39c) = game_time->game_time + 750;
    }
}
