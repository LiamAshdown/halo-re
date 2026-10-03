/**
 * @file src/shaders/globals.cpp
 * Binds halo::shaders::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/shaders/shaders.hpp"
#include "halo/shaders/api.hpp"
#include <stdarg.h>
#include "halo/core/crt.hpp"
#include "halo/shaders/api.hpp"
#include "link/shaders.hpp"

namespace halo::shaders {

Globals &Service::instance()
{
    static Globals state{
        ::numeric_countdown_timer_remaining_ms,
        ::numeric_countdown_timer_running,
        ::game_time,
        ::numeric_countdown_timer_last_update_ms,
    };
    return state;
}

}  // namespace halo::shaders
