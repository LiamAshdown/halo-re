// actor_mode_sleep_update  (not a Ghidra function: actor mode table 0x65524c, mode "sleep" slot +0x1c)
// address 0x408090, size 35 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408090..0x4080b3 (no C existed: a mode entered in game would have hit a trap).
//   A sleeping actor fires nothing: the firing request kind (+0x3fc) is cleared every update.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_sleep_update(datum_index actor_index)
{
    *(int16_t *)(ACTOR(actor_index) + 0x3fc) = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
