// actor_mode_flee_movement_cancelled  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x2c)
// address 0x403d20, size 42 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x403d20..0x403d4a (no C existed: a mode entered in game would have hit a trap).
//   The actor's movement was cancelled: no flee destination (+0xa4 = -1), a new one is wanted (+0xa2 = 1).
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

void actor_mode_flee_movement_cancelled(datum_index actor_index)
{
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    ((actor_mode_flee_data *)mode_data)->destination = -1;
    ((actor_mode_flee_data *)mode_data)->movement_cancelled = 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
