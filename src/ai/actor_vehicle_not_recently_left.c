// actor_vehicle_not_recently_left  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40ac30, size 58 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40ac30..0x40ac6a.
//   Whether the actor may take this vehicle: any vehicle but the one it last left (+0x390) until the time held at
//   +0x394 has passed.
// blam-cc: EAX -> actor_index, stack -> vehicle_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

uint8_t actor_vehicle_not_recently_left(datum_index actor_index, datum_index vehicle_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (vehicle_index != ((struct actor *)act)->exited_vehicle_index) {
        return 1;
    }
    return (uint8_t)(game_time->game_time >= *(int32_t *)&((struct actor *)act)->exited_vehicle_reentry_time);
}
