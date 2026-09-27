// actor_mode_vehicle_enter  (not a Ghidra function: actor mode table 0x65524c, mode "vehicle" slot +0x10)
// address 0x408b50, size 78 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408b50..0x408b9e (no C existed: a mode entered in game would have hit a trap).
//   Boarding starts: the stuck test (0x408ba0) restarts from here -- no stuck checks yet (+0xaa), the time (+0xac)
//   and the actor's position (+0xb0).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data; // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_vehicle_enter(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    *(int16_t *)(act + 0xaa) = 0;
    *(int32_t *)(act + 0xac) = game_time->game_time;
    *(real_point3d *)(act + 0xb0) = *(real_point3d *)(act + 0x12c);
}
