// game_engine_oddball_player_killed  (not a Ghidra function; the oddball game engine definition's +0x68 slot (unknown_68); no C existed, so that
//   stored pointer trapped as unlisted_46c940)
// address 0x46c940, size 374 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46c940..0x46cab5: fired by game_engine_on_player_death with (killer, death
//   object, victim, suicide); with variant +0x8c in 1..2 and as the server. For a real killer that is not a suicide:
//   killing a carrier counts in the killer's +0xc6, a carrier's kill in its +0xc8 (both then score through
//   game_engine_koth_alt_scorer_tick only when +0x8c is 2 and game_engine_is_inactive), otherwise the killer scores
//   when a ball lies free; a killer with a unit then takes the victim's ball (or the first free one), with a kill
//   feed (-1 / 0x24 / 0x25, BL 0). Finally every ball still held by the victim is dropped (-1). The helpers 0x46c8e0
//   / 0x46c910 / 0x46cef0 exist only for this function and are statics / inlined here.
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
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c
extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc
extern uint8_t game_engine_is_inactive(void); // 0x461610
extern void game_engine_koth_alt_scorer_tick(uint32_t player_index); // 0x46c230, blam-cc: EAX player_index
extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message,
    int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast); // 0x460c10, blam-cc: BL broadcast

// 0x46c8e0 (ESI player): whether the player holds one of the variant +0x90 balls.
static uint8_t oddball_is_carrier(datum_index player_index)
{
    int32_t i;

    for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
        if (king_hill_occupant_table[i] == player_index) {
            return 1;
        }
    }
    return 0;
}

// 0x46c910: whether some ball has run out its timer and has no carrier.
static uint8_t oddball_any_ball_free(void)
{
    int32_t i;

    for (i = 0; i < game_engine_variant.engine.oddball.ball_count; i++) {
        if (oddball_ball_timers_006b11cc[i] == 0 && king_hill_occupant_table[i] == 0xffffffff) {
            return 1;
        }
    }
    return 0;
}

void game_engine_oddball_player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    int32_t count;
    int32_t found = -1;
    int32_t i;

    (void)death_object;
    if (game_engine_variant.engine.oddball.ball_type <= 0 || game_engine_variant.engine.oddball.ball_type > 2 || network_game_mode != 2) {
        return;
    }
    count = game_engine_variant.engine.oddball.ball_count;
    if (killer != 0xffffffff && is_suicide == 0) {
        uint8_t *killer_player = ((uint8_t *)player_data->data + ((killer) & 0xffff) * 0x200);
        uint8_t score;

        if (oddball_is_carrier(victim) || oddball_is_carrier(killer)) {
            if (oddball_is_carrier(victim)) {
                (*(int16_t *)(killer_player + 0xc6))++;
            } else {
                (*(int16_t *)(killer_player + 0xc8))++;
            }
            score = game_engine_variant.engine.oddball.ball_type == 2 ? game_engine_is_inactive() : 0; // 0x46cef0
        } else {
            score = oddball_any_ball_free();
        }
        if (score != 0) {
            game_engine_koth_alt_scorer_tick(killer);
        }
        if (*(datum_index *)(killer_player + 0x34) != 0xffffffff) {
            for (i = 0; i < count; i++) {
                if (oddball_ball_timers_006b11cc[i] == 0 && found == -1 && king_hill_occupant_table[i] == 0xffffffff) {
                    found = i;
                }
                if (king_hill_occupant_table[i] == victim) {
                    found = i;
                    break;
                }
            }
            if (found != -1) {
                int32_t message = (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type <= 2) ? -1 : 0x23;

                game_engine_broadcast_kill_feed_by_relationship(killer, message, 0x24, 0x25, killer, 0);
                king_hill_occupant_table[found] = killer;
            }
        }
    }
    for (i = 0; i < count; i++) {
        if (king_hill_occupant_table[i] == victim) {
            king_hill_occupant_table[i] = 0xffffffff;
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
