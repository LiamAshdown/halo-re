// actor_mode_guard_replace_reference  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x28)
// address 0x405270, size 71 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x405270..0x4052b7 (no C existed: a mode entered in game would have hit a trap).
//   Slot +0x28: an object reference changed; the guarded object (+0xd8) and the watched one (+0xac, whose flag
//   +0xab goes when it becomes none) follow it.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_guard_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (*(datum_index *)(mode_data + 0x3c) == old_reference) {
        *(datum_index *)(mode_data + 0x3c) = new_reference;
    }
    if (*(datum_index *)(mode_data + 0x10) == old_reference) {
        *(datum_index *)(mode_data + 0x10) = new_reference;
        if (new_reference == k_datum_index_none) {
            mode_data[0xf] = 0;
        }
    }
}
