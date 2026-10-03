#include "halo/hs/records.hpp"
#include "halo/hs/hs2_commands.hpp"

#include "interface.h"
#include "halo/hs/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &hud_messaging = halo::link::ref<hud_messaging_globals *>(halo::ui::vars().hud_messaging);

namespace halo::hs {

/**
 * Evaluate handler of hs function "hud_set_timer_warning_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480da0
 */
void HudCommands::evaluate_hud_set_timer_warning_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint16_t seconds = (uint16_t)(halo::hs::argument_ushort(arguments[0]) * 0x3c + halo::hs::argument_ushort(arguments[1]));

    *(uint16_t *)&halo::interface::globals().hud_messaging->timer_warning_ticks = (uint16_t)((uint32_t)seconds * 0x1e);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

}
