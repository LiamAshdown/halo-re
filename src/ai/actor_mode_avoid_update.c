// actor_mode_avoid_update  (not a Ghidra function: actor mode table 0x65524c, mode "avoid" slot +0x1c)
// address 0x401850, size 138 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x401850..0x4018da (no C existed: a mode entered in game would have hit a trap).
//   Firing request while avoiding danger: with morale (+0x268) of 5 or more a suppressing burst (+0x454, kind 7,
//   style 2); otherwise kind 5 with style 5 while the danger (+0x280) lasts, else style 2. The request fire kind is 4
//   and +0x426 copies the actor's +0x358; the remaining request flags are cleared.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_avoid_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->target_combat_status >= 5) {
        act[0x454] = 1;
        ((actor *)act)->vocalization_unknown_3e8 = 7;
        ((actor *)act)->vocalization_unknown_3ec = 2;
    } else {
        ((actor *)act)->vocalization_unknown_3e8 = 5;
        ((actor *)act)->vocalization_unknown_3ec = ((actor *)act)->danger_type > 0 ? 5 : 2;
    }
    ((struct actor *)act)->look_posture = 4;
    act[0x426] = act[0x358];
    act[0x427] = 0;
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
