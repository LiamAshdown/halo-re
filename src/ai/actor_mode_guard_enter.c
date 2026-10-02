// actor_mode_guard_enter  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x10)
// address 0x404820, size 66 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x404820..0x404862 (no C existed: a mode entered in game would have hit a trap).
//   Guarding starts: the targets' seen flags reset (0x41f9d0, EAX actor), +0x98 cleared, and when the guard is an
//   ambush (+0xa6) the shot counters too (0x41fa20).
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

extern void actor_target_reset_seen_flags(datum_index actor_index);    // 0x41f9d0, EAX
extern void actor_target_reset_shot_counters(datum_index actor_index); // 0x41fa20, EAX

void actor_mode_guard_enter(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    actor_target_reset_seen_flags(actor_index);
    act[0x98] = 0;
    if (act[0xa6]) {
        actor_target_reset_shot_counters(actor_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
