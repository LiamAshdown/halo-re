#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * The main loop, its frame pacer and shutdown, plus per-frame game timing helpers.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct MainLoop {
    static void engine_flush_pending_simulation_ticks(void);
    static uint32_t frame_rate_average_update(void);
    static void timer_reset(void);
    static void ensure_local_players(void);
    static void loop(void);
    static void loop_begin(void);
    static bool loop_frame(void);
    static void loop_frame_pacer(void);
    static void loop_shutdown_cleanup(void);
    static void menu_music_stop(void);
    static void menu_return_and_reset(void);
};

}
