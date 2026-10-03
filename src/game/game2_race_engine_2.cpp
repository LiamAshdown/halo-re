#include "halo/game/game2_engines.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern uint32_t ctf_globals_live;
}

namespace halo::game {

/**
 * When the team's bit is set in the flag mask (0x006b1290), whether its flag is eligible for capture (0x46df30
 * with the team as both team and flag); else false.
 *
 * @address 0x46e9d0
 */
uint8_t RaceEngine::waypoint_filter(datum_index player, int32_t team)
{
    (void)player;
    if ((ctf_globals_live & (1u << (team & 0x1f))) == 0) {
        return 0;
    }
    return halo::game::game_engine_ctf_is_flag_eligible_for_capture((uint32_t)team, team);
}

}
