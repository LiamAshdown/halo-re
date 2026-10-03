#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Fixed-rate simulation tick accounting, tick records, time scale and the multiplayer clock. Stateless service
 * class: every function is a static member and the state it acts on lives in the engine globals.
 */
class SimulationClock {
public:
    static void effects_update(real delta_time);
    static int32_t accumulate_simulation_ticks(float elapsed_seconds, char keep_remainder);
    static void advance_simulation_ticks(float delta_time);
    static void allocate_tick_record(void);
    static int32_t announce_time_remaining(void);
    static void apply_catchup_speed_boost(void);
    static float compute_time_scale(int32_t param_a, int32_t param_b);
    static int32_t get_current_tick(void);
    static int32_t get_time_remaining(void);
    static float get_time_scale(void);
    static void init_tick_record_for_mode(void);
};

}
