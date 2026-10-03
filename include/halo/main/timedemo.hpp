#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * Timedemo benchmark update.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct Timedemo {
    static void benchmark_update(void);
};

}
