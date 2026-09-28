// game_engine_oddball_player_new_life  (not a Ghidra function; the oddball game engine definition's +0x14 slot (player_new_life); no C existed, so that
//   stored pointer trapped as unlisted_46c150)
// address 0x46c150, size 70 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46c150..0x46c195: as the server clears the player score and, without teams, the
//   score of the player's team.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)

void game_engine_oddball_player_new_life(datum_index player_index)
{
    if (network_game_mode != 2) {
        return;
    }
    king_alt_player_score[player_index & 0xffff] = 0;
    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {
        king_alt_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20)] = 0;
    }
}
