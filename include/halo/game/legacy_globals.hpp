#pragma once

#include <cstdint>

extern "C" {
extern uint8_t DAT_0087ab18;
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

}
