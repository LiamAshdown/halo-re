// actor_mode_guard_get_look_weights  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x24)
// address 0x4051c0, size 161 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x4051c0..0x405261 (no C existed: a mode entered in game would have hit a trap).
//   Slot +0x24: the four look weights -- 0x685208's record unless ambushing (+0xa4); ambushing, 0x686b00's when
//   +0xa6, 0x686afc's when +0xa5, else 0x68521c's.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern const float *actor_mode_guard_look_weights_idle;    // 0x00685208
extern const float *actor_mode_guard_look_weights_a6;      // 0x00686b00
extern const float *actor_mode_guard_look_weights_a5;      // 0x00686afc
extern const float *actor_mode_guard_look_weights_ambush;  // 0x0068521c

void actor_mode_guard_get_look_weights(datum_index actor_index, float *out_weights)
{
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;
    const float *source;

    if (!mode_data[0x8]) {
        source = actor_mode_guard_look_weights_idle;
    } else if (mode_data[0xa]) {
        source = actor_mode_guard_look_weights_a6;
    } else if (mode_data[0x9]) {
        source = actor_mode_guard_look_weights_a5;
    } else {
        source = actor_mode_guard_look_weights_ambush;
    }
    out_weights[0] = source[0];
    out_weights[1] = source[1];
    out_weights[2] = source[2];
    out_weights[3] = source[3];
}
