/**
 * @file src/input/globals.cpp
 * Binds halo::input::Globals to the engine variables the data image defines under their original link names.
 */

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <wchar.h>
#include "ai.h"
#include "crt.h"
#include <string.h>
#include "win32.h"
#include <stdarg.h>
#include "objects.h"
#include "units.h"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/input/system.hpp"
#include "halo/input/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/input/api.hpp"
#include "link/input.hpp"

namespace halo::input {

Globals &Service::instance()
{
    static Globals state{
        ::joystick_slot_devices,
        ::input_suppressed,
        ::g_control_binding_state,
        ::g_control_binding_secondary_active,
    };
    return state;
}

}  // namespace halo::input
