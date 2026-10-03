#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * Input system start-up, state reset and the per-frame tick.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct InputSystem {
    static void state_initialize(void);
    static uint32_t system_initialize(void);
    static void time_base_resync(void);
    static void update_tick(void);
};

}
