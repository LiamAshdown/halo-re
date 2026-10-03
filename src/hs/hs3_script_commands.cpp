#include "halo/hs/hs3_commands.hpp"
#include "halo/scenario/api.hpp"
#include "win32.h"
#include "halo/input/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/main/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/hs/vars.hpp"

static auto &ui_widget_show_path_flag = halo::link::ref<uint8_t>(halo::hs::vars().ui_widget_show_path_flag);

namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `structure_bsp_index`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47f670
 */
void ScriptCommands::evaluate_structure_bsp_index(int16_t function_index, uint32_t thread_index, char first) const
{
    halo::hs::hs_thread_return((int32_t)(uint16_t)halo::scenario::globals().structure_bsp_index, thread_index);
}

/**
 * Evaluate handler of the hs script function `switch_bsp`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47f620
 */
void ScriptCommands::evaluate_switch_bsp(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::scenario::structure_bsp_switcher::switch_to(*(int16_t *)&arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `thread_sleep`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482640
 */
void ScriptCommands::evaluate_thread_sleep(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        Sleep((uint32_t)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `track_remote_player_position_updates`: reads its typed arguments
 * from the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482560
 */
void ScriptCommands::evaluate_track_remote_player_position_updates(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::networking::player_update_history_log_set_name_filter((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `ui_widget_show_path`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4814d0
 */
void ScriptCommands::evaluate_ui_widget_show_path(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ui_widget_show_path_flag = *(uint8_t *)&arguments[0];
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unbind`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482430
 */
void ScriptCommands::evaluate_unbind(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::input::hs_unbind_control((const char *)arguments[0], (const char *)arguments[1]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `version`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47f6a0
 */
void ScriptCommands::evaluate_version(int16_t function_index, uint32_t thread_index, char first) const
{
    halo::main::console_print_error_va(0, "halo pc 01.00.10.0621 Apr 16 2014 15:54:48");
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `wake`: reads its typed arguments from the calling thread and hands
 * the result back through the thread, exactly as the original handler did.
 *
 * @address 0x489a20
 */
void ScriptCommands::evaluate_wake(int16_t function_index, uint32_t thread_index, char first) const
{
    uint8_t *syntax = (uint8_t *)halo::hs::globals().syntax_data->data;
    uint8_t *frame = *(uint8_t **)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread) + 0x10);
    uint32_t call_node = *(uint32_t *)(frame + 4) & halo::k_slot_mask;
    uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & halo::k_slot_mask;
    uint32_t argument = *(uint32_t *)(syntax + name_node * 0x14 + 8) & halo::k_slot_mask;
    datum_index thread = halo::hs::hs_thread_find_by_script_index(*(int16_t *)(syntax + argument * 0x14 + 0x10));

    (void)function_index;
    (void)first;
    if (thread != k_datum_index_none) {
        halo::hs::hs_thread_restart(thread);
    }
    halo::hs::hs_thread_return(0, thread_index);
}

}
