// actor_mode_flee_enter  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x10)
// address 0x403740, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x403740..0x40379b (no C existed: a mode entered in game would have hit a trap).
//   Fleeing starts: +0xb4 cleared; with a flee kind (+0xa8) +0x98 cleared; a panicking actor (kinds 9..12) that is
//   not already cowering (+0x9e) starts its unit's random look-around (0x570650, EAX unit).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern void unit_initialize_random_turn_angle(uint32_t object_index); // 0x570650, EAX

void actor_mode_flee_enter(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    int16_t kind = *(int16_t *)(act + 0xa8);

    *(int32_t *)(act + 0xb4) = 0;
    if (kind > 0) {
        act[0x98] = 0;
    }
    if (*(int16_t *)(act + 0x9e) == 0 && *(datum_index *)(act + 0x18) != k_datum_index_none && kind >= 9 && kind <= 12) {
        unit_initialize_random_turn_angle(*(datum_index *)(act + 0x18));
    }
}
