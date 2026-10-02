// actor_mode_guard_movement_cancelled  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x2c)
// address 0x405100, size 119 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x405100..0x405177 (no C existed: a mode entered in game would have hit a trap).
//   The actor's movement was cancelled: an ambush walking to its spot (+0xa4, guard kind 3 at +0xc0) gives the
//   ambush up; guard kinds 3 -- and 1 unless the order is committed (+0x160) -- fall back to kind 0 with no spot
//   (+0xc4 = -1) and ask for a new one (+0xaa).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_guard_movement_cancelled(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    int16_t kind;

    if (act[0xa4] && ((struct actor *)act)->mode_data.guard.stage == 3) {
        act[0xa4] = 0;
        ((struct actor *)act)->mode_data.guard.countdown_0c = 0;
        act[0xa6] = 0;
    }
    kind = ((struct actor *)act)->mode_data.guard.stage;
    if (kind == 3 || (kind == 1 && act[0x160] == 0)) {
        ((struct actor *)act)->mode_data.guard.stage = 0;
        ((struct actor *)act)->mode_data.guard.firing_position = -1;
        act[0xaa] = 1;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
