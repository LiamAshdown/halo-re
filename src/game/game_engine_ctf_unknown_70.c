// game_engine_ctf_unknown_70  (not a Ghidra function; the ctf game engine definition's +0x70 slot (unknown_70); no C existed, so that
//   stored pointer trapped as unlisted_469ae0)
// address 0x469ae0, size 263 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469ae0..0x469be6: a respawn weighting for a position: 1.0 unless variant +0x7c
//   is set; else the squared distance to the enemy flag stand, clamped to 0.5..10, inverted; after the first second
//   (tick > 30) a distance above 1 raises it to the power 0.33 and the result is clamped to 0.5..2.0.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c
extern game_variant game_engine_variant; // 0x006f1c88
extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88
extern double pow(double x, double y); // C runtime (libcmt; the retail copy is the CRT pow)

float game_engine_ctf_unknown_70(datum_index player_index, real_point3d *position)
{
    real_point3d *stand;
    float dx, dy, dz, distance_squared, weight;
    int32_t other_team;

    if (game_engine_variant.engine.ctf.assault == 0) {
        return 1.0f;
    }
    other_team = (*(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20) + 1) % 2;
    stand = ctf_team_flag_stand_position[other_team];
    dx = stand->x - position->x;
    dy = stand->y - position->y;
    dz = stand->z - position->z;
    distance_squared = dz * dz + dx * dx + dy * dy;
    if (distance_squared < 0.5f) {
        distance_squared = 0.5f;
    } else if (distance_squared > 10.0f) {
        distance_squared = 10.0f;
    }
    weight = 1.0f / distance_squared;
    if (game_time->game_time <= 0x1e) {
        return weight;
    }
    if (distance_squared > 1.0f) {
        weight = (float)pow((double)weight, (double)0.33f);
    }
    if (weight < 0.5f) {
        return 0.5f;
    }
    if (weight > 2.0f) {
        return 2.0f;
    }
    return weight;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
