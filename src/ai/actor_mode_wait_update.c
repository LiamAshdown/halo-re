// actor_mode_wait_update  (not a Ghidra function: actor mode table 0x65524c, mode "wait" slot +0x1c)
// address 0x409dc0, size 165 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x409dc0..0x409e65 (no C existed: a mode entered in game would have hit a trap).
//   Firing request while waiting: walking (+0x504) kind 3 / style 0; with a remembered target (+0x1d0, not
//   forgotten at +0x1cc) and a wait kind (+0xa8) kind 5 / style 1 at that target; else kind 1. Fire kind 3, request
//   flags cleared.
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

void actor_mode_wait_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (act[0x504]) {
        ((actor *)act)->vocalization_unknown_3e8 = 3;
        ((actor *)act)->vocalization_unknown_3ec = 0;
    } else if (!act[0x1cc] && *(int32_t *)&((struct actor *)act)->unknown_1d0 != -1 && ((struct actor *)act)->mode_data.wait.countdown_0c > 0) {
        ((actor *)act)->vocalization_unknown_3e8 = 5;
        ((actor *)act)->vocalization_unknown_3ec = 1;
        *(int32_t *)(act + 0x3f0) = *(int32_t *)&((struct actor *)act)->unknown_1d0;
    } else {
        ((actor *)act)->vocalization_unknown_3e8 = 1;
    }
    ((struct actor *)act)->idle_stance = 3;
    act[0x454] = 0;
    act[0x426] = 0;
    act[0x427] = 0;
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;
}
