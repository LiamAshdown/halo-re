// actor_mode_vehicle_update  (not a Ghidra function: actor mode table 0x65524c, mode "vehicle" slot +0x1c)
// address 0x408e80, size 170 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408e80..0x408f2a (no C existed: a mode entered in game would have hit a trap).
//   Firing request while boarding: at the entry point (+0xc8) kind 4 / style 4 aimed along the approach direction
//   (+0xd8); still walking (+0x4a8) kind 3 / style 0; otherwise nothing. Fire kind 4, request flags cleared.
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

void actor_mode_vehicle_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (act[0xc8]) {
        ((actor *)act)->vocalization_unknown_3e8 = 4;
        ((actor *)act)->vocalization_unknown_3ec = 4;
        *(real_vector3d *)(act + 0x3f0) = *(real_vector3d *)(act + 0xd8);
    } else if (act[0x4a8]) {
        ((actor *)act)->vocalization_unknown_3e8 = 3;
        ((actor *)act)->vocalization_unknown_3ec = 0;
    } else {
        ((actor *)act)->vocalization_unknown_3e8 = 0;
    }
    ((struct actor *)act)->look_posture = 4;
    act[0x454] = 0;
    act[0x426] = 0;
    act[0x427] = 0;
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;
}
