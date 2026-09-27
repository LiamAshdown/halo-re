// actor_mode_flee_get_look_weights  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x24)
// address 0x403d50, size 100 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x403d50..0x403db4 (no C existed: a mode entered in game would have hit a trap).
//   Slot +0x24: the four look weights for this mode -- the record 0x685200 points at while fleeing (+0xa8 > 0),
//   else the one 0x686af8 points at.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern const float *actor_mode_flee_look_weights_fleeing; // 0x00685200
extern const float *actor_mode_default_look_weights;      // 0x00686af8

void actor_mode_flee_get_look_weights(datum_index actor_index, float *out_weights)
{
    const float *source = *(int16_t *)(ACTOR(actor_index) + 0xa8) > 0 ? actor_mode_flee_look_weights_fleeing
                                                                        : actor_mode_default_look_weights;

    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}
