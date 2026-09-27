// actor_mode_uncover_get_look_weights  (not a Ghidra function: actor mode table 0x65524c, mode "uncover" slot +0x24)
// address 0x408840, size 100 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408840..0x4088a4 (no C existed: a mode entered in game would have hit a trap).
//   Slot +0x24: the four look weights -- the record 0x685204 points at once uncovering (+0x9c), else 0x6851f4's.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern const float *actor_mode_uncover_look_weights_active; // 0x00685204
extern const float *actor_mode_uncover_look_weights_idle;   // 0x006851f4

void actor_mode_uncover_get_look_weights(datum_index actor_index, float *out_weights)
{
    const float *source = ACTOR(actor_index)[0x9c] ? actor_mode_uncover_look_weights_active
                                                    : actor_mode_uncover_look_weights_idle;

    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}
