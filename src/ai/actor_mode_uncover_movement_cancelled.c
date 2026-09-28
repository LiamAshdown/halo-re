// actor_mode_uncover_movement_cancelled  (not a Ghidra function: actor mode table 0x65524c, mode "uncover" slot +0x2c)
// address 0x408800, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408800..0x408834 (no C existed: a mode entered in game would have hit a trap).
//   The actor's movement was cancelled: while walking to its uncover point (+0xa4 == 1) the point is forgotten
//   (+0xa6 = -1) and a new one wanted (+0x9d).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_uncover_movement_cancelled(datum_index actor_index)
{
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (((actor_mode_uncover_data *)mode_data)->stage == 1) {
        *(int16_t *)(mode_data + 0xa) = -1;
        mode_data[0x1] = 1;
    }
}
