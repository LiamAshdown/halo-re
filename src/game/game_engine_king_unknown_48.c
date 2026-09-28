// game_engine_king_unknown_48  (not a Ghidra function; the king game engine definition's +0x48 slot (unknown_48); no C existed, so that
//   stored pointer trapped as unlisted_46aee0)
// address 0x46aee0, size 286 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46aee0..0x46affd: while the game runs, as the server and with variant +0x7c set
//   (moving hill), counts the hill move ticks down; at zero they restart at 0x708, a new hill location differing from
//   the current one is picked (0x46a1b0), the boundary rebuilt and sound 0x1e queued, repeating while the new hill
//   has no starting locations. Then registers the "crown_blue" waypoint (slot 0) at the hill centre, or prints FAILED
//   TO FIND HILL, and as the server updates the hill occupancy. UNSURE: 0x46a1b0's fallback is the caller's ECX,
//   which is not set here (whatever the engine update left); it is modeled as the current location, i.e. no move.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

extern game_engine_definition *current_game_engine;
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern int32_t game_engine_state_value; // 0x0087aa10
extern int32_t king_hill_move_ticks_006b1068; // 0x006b1068
extern int32_t king_starting_location_type; // 0x006b1064
extern int32_t king_starting_location_count; // 0x006b0f50
extern real_point3d king_hill_boundary_center; // 0x006b1044
extern int32_t game_engine_pick_random_recent_location(int32_t exclude_value, int32_t fallback); // 0x46a1b0, blam-cc: ECX fallback
extern void game_engine_koth_build_hill_boundary(void); // 0x46a240
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name,
    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260, blam-cc: EAX owner, CX slot, EBX position, EDI icon
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, blam-cc: AL clear_first
extern void game_engine_koth_update_hill_occupancy_state(void); // 0x46acb0

void game_engine_king_unknown_48(void)
{
    if ((current_game_engine == 0 || game_engine_state_value == 0) && network_game_mode == 2 &&
        game_engine_variant.ctf_option_7c != 0 && --king_hill_move_ticks_006b1068 == 0) {
        king_hill_move_ticks_006b1068 = 0x708;
        king_starting_location_type = game_engine_pick_random_recent_location(king_starting_location_type, king_starting_location_type);
        game_engine_koth_build_hill_boundary();
        game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);
        while (king_starting_location_count == 0 && king_starting_location_type != 0) {
            king_starting_location_type = game_engine_pick_random_recent_location(king_starting_location_type, king_starting_location_type);
            game_engine_koth_build_hill_boundary();
            game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);
        }
    }
    if (king_starting_location_count > 0) {
        real_point3d position = king_hill_boundary_center;

        custom_waypoint_register(0xffffffff, 0, &position, "crown_blue", 0.0f, 0xffffffff, -1);
    } else {
        console_print_error_va(0, "FAILED TO FIND HILL");
    }
    if (network_game_mode == 2) {
        game_engine_koth_update_hill_occupancy_state();
    }
}
