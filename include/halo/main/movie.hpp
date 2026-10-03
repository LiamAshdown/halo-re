#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * Movie frame capture.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct MoviePlayer {
    static void capture_frame_export(void);
};

}
