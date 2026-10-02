// game_engine_ctf_unknown_48  (not a Ghidra function; the ctf game engine definition's +0x48 slot (unknown_48); no C existed, so that
//   stored pointer trapped as unlisted_469180)
// address 0x469180, size 236 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469180..0x46926b: as the server: when either team's touch count reached the
//   limit at 0x6b0ea0 and the game has not ended, marks the server (+0xa0f), sets the ending state and a 7 s timer,
//   queues sound 1, closes all widgets and sends end-game notification 1. Then for each team whose return credit is
//   active the credit ticks count up, and past 0x258 queue sound 8 (team 0) / 0xb (team 1) and restart at 1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode; // 0x00719720
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t game_engine_state_value; // 0x0087aa10
extern float game_engine_end_game_timer; // 0x0087aa08
extern void widget_close_all(void); // 0x498650
extern void game_engine_send_end_game_notification(uint32_t reason); // 0x4671d0, blam-cc: EAX reason
extern int32_t ctf_flag_capture_limit_006b0ea0; // 0x006b0ea0, UNSURE name: the captures that end the game
extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4
extern int32_t ctf_team_return_credit_ticks[2]; // 0x006b0ea8

void game_engine_ctf_unknown_48(void)
{
    int32_t limit = ctf_flag_capture_limit_006b0ea0;

    if (network_game_mode != 2) {
        return;
    }
    if ((ctf_team_flag_touch_count[0] >= limit || ctf_team_flag_touch_count[1] >= limit) && game_engine_state_value == 0) {
        network_server->game_over = 1;
        game_engine_state_value = 1;
        game_engine_end_game_timer = 7.0f;
        game_engine_queue_multiplayer_sound(1, 0xffffffff, 0);
        widget_close_all();
        game_engine_send_end_game_notification(1);
    }
    if (ctf_team_return_credit_active[0] != 0) {
        int32_t ticks = ctf_team_return_credit_ticks[0];

        if (ticks > 0x258) {
            game_engine_queue_multiplayer_sound(8, 0xffffffff, 1);
            ticks = 0;
        }
        ctf_team_return_credit_ticks[0] = ticks + 1;
    }
    if (ctf_team_return_credit_active[1] != 0) {
        int32_t ticks = ctf_team_return_credit_ticks[1];

        if (ticks > 0x258) {
            game_engine_queue_multiplayer_sound(0xb, 0xffffffff, 1);
            ticks = 0;
        }
        ctf_team_return_credit_ticks[1] = ticks + 1;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
