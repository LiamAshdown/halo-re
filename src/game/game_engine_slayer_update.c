// game_engine_slayer_update  (not a Ghidra function; the slayer game engine definition's +0x38 slot (update); no C existed, so that
//   stored pointer trapped as unlisted_46f7f0)
// address 0x46f7f0, size 397 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f7f0..0x46f97c: with variant +0x7d the player's speed (+0x6c) above 1 decays
//   by 1/9000 a tick down to 1; with +0x7c a speed below 1 grows by 1/90000 up to 1. With +0x7e (targets) the
//   player's waypoint slot is cleared and, when its +0x88 target has a unit, re-registered "target_blue" over that
//   unit for this player only; as the server a player with a unit and no target, or whose target passes 0x463100,
//   gets a new random target. A team reaching the score limit begins the end-game sequence.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>
#include "objects.h"
#include "units.h"

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
extern int32_t game_engine_state_value; // 0x0087aa10
extern uint8_t custom_waypoints[]; // 0x006f1888 (0x20 bytes each)
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name,
    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260, blam-cc: EAX owner, CX slot, EBX position, EDI icon
extern void game_engine_player_select_random_target(datum_index player_or_all); // 0x46f1a0
extern uint8_t game_engine_player_respawn_priority_gate(uint32_t player_index); // 0x463100, blam-cc: EDX player_index
extern void game_engine_begin_end_game_sequence(void); // 0x45fd90

void game_engine_slayer_update(datum_index player_index)
{
    uint8_t *player = ((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200);
    datum_index target;

    if (game_engine_variant.ctf_option_7d != 0 && ((struct player *)player)->speed > 1.0f) {
        float speed = ((struct player *)player)->speed - 0.000111111112f;

        ((struct player *)player)->speed = speed > 1.0f ? speed : 1.0f;
    }
    if (game_engine_variant.ctf_option_7c != 0 && ((struct player *)player)->speed < 1.0f) {
        float speed = ((struct player *)player)->speed + 0.0000111111112f;

        ((struct player *)player)->speed = speed <= 1.0f ? speed : 1.0f;
    }
    if (game_engine_variant.ctf_option_7e != 0) {
        memset(custom_waypoints + (int16_t)player_index * 0x20, 0, 0x20);
        target = *(datum_index *)&((struct player *)player)->unknown_88;
        if (target != 0xffffffff) {
            datum_index unit_index = *(datum_index *)(((uint8_t *)player_data->data + ((target) & 0xffff) * 0x200) + 0x34);

            if (unit_index != 0xffffffff) {
                uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);

                custom_waypoint_register(0xffffffff, (int16_t)player_index, (real_point3d *)(unit + 0xa0), "target_blue", 0.0f,
                    player_index, -1);
            }
        }
        if (network_game_mode == 2) {
            if (((struct player *)player)->unit != 0xffffffff && *(datum_index *)&((struct player *)player)->unknown_88 == 0xffffffff) {
                game_engine_player_select_random_target(player_index);
            }
            target = *(datum_index *)&((struct player *)player)->unknown_88;
            if (target != 0xffffffff && game_engine_player_respawn_priority_gate(target) != 0) {
                game_engine_player_select_random_target(player_index);
            }
        }
    }
    if (slayer_team_score[((struct player *)player)->team] >= game_engine_variant.score_limit) {
        game_engine_begin_end_game_sequence();
    }
}
