// game_engine_slayer_player_killed  (not a Ghidra function; the slayer game engine definition's +0x68 slot (unknown_68); no C existed, so that
//   stored pointer trapped as unlisted_46f580)
// address 0x46f580, size 204 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f580..0x46f64b: fired first by game_engine_on_player_death with (killer,
//   death object, victim, suicide). Unless the victim is marked for deletion or there is no killer: a suicide takes a
//   point from the killer; otherwise the pulse icons animate (killer fading, victim growing) and the killer scores a
//   point, except that with variant +0x7e set as the server the kill only counts (and a new target is picked) when
//   the victim was the killer's +0x88 target. The score helper 0x46f540 (EAX player, EDX delta) is not a separate
//   function in the repo and is inlined here as a static.
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
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
extern void game_engine_animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player); // 0x46f450, blam-cc: EAX fading, ECX growing
extern void game_engine_player_select_random_target(datum_index player_or_all); // 0x46f1a0

// 0x46f540 (EAX player, EDX delta): unless a client, adds delta to the player's team score and its own score.
static void game_engine_slayer_add_score(datum_index player_index, int32_t delta)
{
    if (network_game_mode == 1) {
        return;
    }
    slayer_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20)] += delta;
    slayer_player_score[player_index & 0xffff] += delta;
}

void game_engine_slayer_player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)
{
    uint8_t *killer_player;

    (void)death_object;
    if (*(((uint8_t *)player_data->data + ((victim) & 0xffff) * 0x200) + 0xd5) != 0 || killer == 0xffffffff) {
        return;
    }
    killer_player = ((uint8_t *)player_data->data + ((killer) & 0xffff) * 0x200);
    if (is_suicide != 0) {
        game_engine_slayer_add_score(killer, -1);
        return;
    }
    game_engine_animate_hill_pulse_icons(killer, victim);
    if (game_engine_variant.engine.slayer.kill_in_order != 0 && network_game_mode == 2) {
        if (*(datum_index *)(killer_player + 0x88) != victim) {
            return;
        }
        game_engine_player_select_random_target(killer);
    }
    game_engine_slayer_add_score(killer, 1);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
