#pragma once

#include <cstdint>

extern "C" {
extern uint8_t DAT_0087ab18;
extern uint8_t g_006f1d25;
extern int32_t g_006f1d28;
extern uint8_t unknown_00699f40[];
extern uint8_t unknown_0071cc20[];
extern float unknown_0069c530;
extern float unknown_0069c534;
}

namespace halo::game::fields {

/**
 * Index of the accept action in local_player_input_state::buttons (the input module's _input_action_accept).
 */
inline constexpr int32_t k_input_action_accept = 8;

/**
 * Non-zero while GameLifecycle::simulate_tick is running a simulation tick.
 *
 * @address 0x0087ab18
 */
inline uint8_t &simulation_tick_in_progress = ::DAT_0087ab18;

/**
 * Outgoing network event feed whose pending count is network_scenario_round_counter_a (the dword at 0x00699f44);
 * flushed by the server at the end of every simulation tick.
 *
 * @address 0x00699f40
 */
inline uint8_t *const network_event_feed_a = ::unknown_00699f40;

/**
 * Second outgoing network event feed, paired with network_scenario_round_counter_b (0x0071cc24).
 *
 * @address 0x0071cc20
 */
inline uint8_t *const network_event_feed_b = ::unknown_0071cc20;

/**
 * Water density in the unit the tag data uses (1.0); game_start_new_map scales it by 118613.34 into k_water_density.
 *
 * @address 0x0069c530
 */
inline float &water_density_base = ::unknown_0069c530;

/**
 * Air density in the unit the tag data uses (0.0011); game_start_new_map scales it by 118613.34 into k_air_density.
 *
 * @address 0x0069c534
 */
inline float &air_density_base = ::unknown_0069c534;

/**
 * Set by the server console command sv_end_game before it begins the end game sequence; the post game stage then
 * cancels the dedicated server idle timer instead of arming it.
 *
 * @address 0x006f1d25
 */
inline uint8_t &server_end_game_requested = ::g_006f1d25;

/**
 * The console global sv_mapcycle_timeout: seconds the dedicated server idles in the post game state before it moves
 * on to the next map, 0 to disable.
 *
 * @address 0x006f1d28
 */
inline int32_t &mapcycle_timeout = ::g_006f1d28;

}
