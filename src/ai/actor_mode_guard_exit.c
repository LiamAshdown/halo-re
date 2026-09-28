// actor_mode_guard_exit  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x20)
// address 0x404870, size 57 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x404870..0x4048a9 (no C existed: a mode entered in game would have hit a trap).
//   Guarding ends: a guard that held a firing position (+0xa1) releases it (+0x1e4 = 0, +0x1e8 = -1).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_guard_exit(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (act[0xa1]) {
        ((struct actor *)act)->unknown_1e4 = 0;
        *(int32_t *)&((struct actor *)act)->unknown_1e8 = -1;
    }
}
