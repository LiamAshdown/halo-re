// game_engine_race_player_changed_object  (not a Ghidra function; the race game engine definition's +0x18 slot (player_changed_object); no C existed, so that
//   stored pointer trapped as unlisted_46db00)
// address 0x46db00, size 106 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46db00..0x46db69: as the server: for a valid player when the variant dword
//   +0x80 is 2, adds its short +0xc6 to the extra bucket score of its team; then checks the bucket scores for the end
//   of the round (tail call).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern int32_t game_engine_bucket_scores_extra[16]; // 0x006b1358
extern void game_engine_check_bucket_scores_and_end_round(void); // 0x46db70

void game_engine_race_player_changed_object(datum_index player_index)
{
    uint8_t *player;

    if (network_game_mode != 2) {
        return;
    }
    player = (uint8_t *)datum_get(player_index, player_data);
    if (player != 0 && game_engine_variant.ctf_value_80 == 2) {
        game_engine_bucket_scores_extra[*(int32_t *)(player + 0x20)] += *(int16_t *)(player + 0xc6);
    }
    game_engine_check_bucket_scores_and_end_round();
}
