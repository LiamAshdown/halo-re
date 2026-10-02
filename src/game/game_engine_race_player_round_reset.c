// game_engine_race_player_round_reset  (not a Ghidra function; the race game engine definition's +0x98 slot (player_round_reset); no C existed, so that
//   stored pointer trapped as unlisted_46ee60)
// address 0x46ee60, size 207 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46ee60..0x46ef2e: as the server, for a live player (index in range, salt 0 or
//   matching): with variant +0x80 == 2 its word +0xc6 is added to the extra bucket score of its team (or, when the
//   flag argument equals the team, of team flag != 1); its words +0xc4/+0xc6/+0xc8 are cleared, +0x88 takes the game
//   tick and its 0x6b12d4 entry is cleared. Every server call ends with
//   game_engine_check_bucket_scores_and_end_round.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t game_engine_bucket_scores_extra[16]; // 0x006b1358
extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4
extern void game_engine_check_bucket_scores_and_end_round(void); // 0x46db70

void game_engine_race_player_round_reset(datum_index player_index, uint8_t team_flag)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);

    if (network_game_mode != 2) {
        return;
    }
    if (player_index != 0xffffffff && index >= 0 && index < *(int16_t *)((uint8_t *)player_data + 0x20)) {
        uint8_t *player = (uint8_t *)player_data->data + index * *(int16_t *)((uint8_t *)player_data + 0x22);
        int16_t player_salt = *(int16_t *)player;

        if (player_salt != 0 && (salt == 0 || player_salt == salt)) {
            if (game_engine_variant.engine.race.team_scoring == 2) {
                uint32_t team = *(uint32_t *)&((struct player *)player)->team;

                if ((uint32_t)team_flag == team) {
                    team = team_flag != 1;
                }
                game_engine_bucket_scores_extra[team] += *(int16_t *)(player + 0xc6);
            }
            *(int16_t *)&((struct player *)player)->objective_time = 0;
            *(int16_t *)(player + 0xc6) = 0;
            ((struct player *)player)->objective_score = 0;
            ((struct player *)player)->slayer_target = game_time->game_time;
            ctf_team_captured_flags_mask[player_index & 0xffff] = 0;
        }
    }
    game_engine_check_bucket_scores_and_end_round();
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
