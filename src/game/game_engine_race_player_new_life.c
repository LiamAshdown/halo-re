// game_engine_race_player_new_life  (not a Ghidra function; the race game engine definition's +0x14 slot (player_new_life); no C existed, so that
//   stored pointer trapped as unlisted_46dab0)
// address 0x46dab0, size 68 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46dab0..0x46daf3: stamps the player's +0x88 with the game tick, clears its
//   captured-flags mask (0x006b12d4) and, as the server, checks the bucket scores for the end of the round (tail
//   call).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode; // 0x00719720
extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4
extern void game_engine_check_bucket_scores_and_end_round(void); // 0x46db70

void game_engine_race_player_new_life(datum_index player)
{
    *(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x88) = game_time->game_time;
    ctf_team_captured_flags_mask[player & 0xffff] = 0;
    if (network_game_mode == 2) {
        game_engine_check_bucket_scores_and_end_round();
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
