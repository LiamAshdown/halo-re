#include "halo/hs/hs2_commands.hpp"

#include "interface.h"
#include "halo/hs/api.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern hud_messaging_globals *hud_messaging;
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "hud_set_timer_warning_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480da0
 */
void HudCommands::evaluate_hud_set_timer_warning_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint16_t seconds = (uint16_t)(*(uint16_t *)&arguments[0] * 0x3c + *(uint16_t *)&arguments[1]);

    *(uint16_t *)&hud_messaging->timer_warning_ticks = (uint16_t)((uint32_t)seconds * 0x1e);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

}
