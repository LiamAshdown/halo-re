// actor_mode_converse_replace_reference  (not a Ghidra function: actor mode table 0x65524c, mode "converse" slot +0x28)
// address 0x402f00, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x402f00..0x402f34 (no C existed: a mode entered in game would have hit a trap).
//   Slot +0x28: an object reference changed; the conversation partner (+0xac) follows it.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_converse_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (*(datum_index *)(mode_data + 0x10) == old_reference) {
        *(datum_index *)(mode_data + 0x10) = new_reference;
    }
}
