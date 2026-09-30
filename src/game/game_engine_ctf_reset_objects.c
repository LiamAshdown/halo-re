// game_engine_ctf_reset_objects  (not a Ghidra function; the ctf game engine definition's +0xac slot (reset_objects); no C existed, so that
//   stored pointer trapped as unlisted_46a010)
// address 0x46a010, size 282 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46a010..0x46a129: as the server: resets the return credit of each existing team
//   flag object, the auto-return ticks take variant +0x80, and the touch counts, return credits, notify throttle and
//   the first four custom waypoints (0x80 bytes) are cleared.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>
#include <string.h>

extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0 (team 0 / team 1 touch counts, active team)
extern uint8_t ctf_active_team; // 0x006b0eb8
extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0
extern datum_index ctf_team_flag_object[2]; // 0x006b0e90

extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4
extern int32_t ctf_team_return_credit_ticks[2]; // 0x006b0ea8
extern int32_t ctf_notify_throttle_tick; // 0x006b0eb4
extern uint8_t custom_waypoints[]; // 0x006f1888

void game_engine_ctf_reset_objects(void)
{
    if (network_game_mode != 2) {
        return;
    }
    if (ctf_team_flag_object[0] != 0xffffffff) {
        game_engine_ctf_reset_team_return_credit(ctf_team_flag_object[0]);
    }
    if (ctf_team_flag_object[1] != 0xffffffff) {
        game_engine_ctf_reset_team_return_credit(ctf_team_flag_object[1]);
    }
    ctf_flag_auto_return_ticks = game_engine_variant.ctf_value_80;
    ctf_team_flag_touch_count[0] = 0;
    ctf_team_flag_touch_count[1] = 0;
    ctf_team_return_credit_active[0] = 0;
    ctf_team_return_credit_active[1] = 0;
    ctf_team_return_credit_ticks[0] = 0;
    ctf_team_return_credit_ticks[1] = 0;
    ctf_notify_throttle_tick = 0;
    memset(custom_waypoints, 0, 0x80);
}
