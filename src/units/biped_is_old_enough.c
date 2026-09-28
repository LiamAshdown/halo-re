// biped_is_old_enough  (not a Ghidra function; the biped object type definition's +0x74 "is old enough" hook, only
//   reached through that table; no C existed, so it trapped as unlisted_55b780)
// address 0x55b780, size 57 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x55b780: never stamped (+0x0c == -1) or the game tick has reached the stamp plus the biped minimum age.
// blam-cc: stack -> object_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t k_biped_minimum_age_ticks; // 0x006893d0

uint8_t biped_is_old_enough(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    int32_t stamp = *(int32_t *)(obj + 0xc); // object.network_update_tick

    if (stamp == -1) {
        return 1;
    }
    return (uint8_t)(game_time->game_time >= stamp + k_biped_minimum_age_ticks);
}
