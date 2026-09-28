// vehicle_is_old_enough  (not a Ghidra function; the vehicle object type definition's +0x74 "is old enough" hook, only
//   reached through that table; no C existed, so it trapped as unlisted_572a30)
// address 0x572a30, size 66 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x572a30: never stamped, or old enough by the vehicle minimum age; otherwise only when byte +0x524 is 1.
// blam-cc: stack -> object_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t k_vehicle_minimum_age_ticks; // 0x006893c8

uint8_t vehicle_is_old_enough(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    int32_t stamp = ((unit_object *)obj)->base.network_update_tick; // object.network_update_tick

    if (stamp == -1) {
        return 1;
    }
    if (game_time->game_time >= stamp + k_vehicle_minimum_age_ticks) {
        return 1;
    }
    return (uint8_t)(obj[0x524] == 1);
}
