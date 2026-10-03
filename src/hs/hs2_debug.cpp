#include "halo/rasterizer/globals.hpp"
#include "halo/hs/hs2_commands.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/rasterizer/vars.hpp"


extern "C" {
extern void chimera__console_out(void *color, const char *format, ...);
extern void (*hs_type_inspectors[])(int16_t type, int32_t value, char *buffer);
extern void message_delta_metrics_dump(char *suffix);
}
static auto &network_bandwidth_graph_globals = halo::link::ref<uint8_t []>(halo::main::vars().network_bandwidth_graph_globals);
static auto &actor_mode_default_look_weights = halo::link::ref<void *>(halo::networking::vars().actor_mode_default_look_weights);
static auto &lens_flare_object_visibility_table = halo::link::ref<uint32_t [0x8c0]>(halo::rasterizer::vars().lens_flare_object_visibility_table);
static auto &lens_flare_marker_visibility = halo::link::ref<uint32_t [0x4002]>(halo::rasterizer::vars().lens_flare_marker_visibility);
static auto &lens_flare_instance_count = halo::link::ref<int32_t>(halo::rasterizer::vars().lens_flare_instance_count);
static auto &rasterizer_model_ambient_reflection_tint = halo::link::ref<uint32_t *>(halo::cutscene::vars().rasterizer_model_ambient_reflection_tint);
static auto &lights_enabled = halo::link::ref<uint8_t *>(halo::objects::vars().lights_enabled);
static auto &cinematic_screen_effect_state = halo::link::ref<uint8_t *>(halo::cutscene::vars().cinematic_screen_effect_state);

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)halo::hs::globals().syntax_data->data + (node & halo::k_slot_mask) * 0x14);
}

namespace halo::hs {

/**
 * Evaluate handler of hs function "help"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4827e0
 */
void DebugCommands::evaluate_help(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_help_print_function((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "inspect"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x489b80
 */
void DebugCommands::evaluate_inspect(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread));
    hs_stack_frame *frame = thread->stack;
    int32_t *result = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    datum_index argument = syntax_get(syntax_get(frame->syntax_node)->data.first_child)->next_node;
    char buffer[0x400];

    frame->size = frame->size + 4;
    if (first != 0) {
        halo::hs::hs_thread_push(argument, thread_index, result);
        return;
    }
    {
        int16_t type = syntax_get(argument)->type;

        if (hs_type_inspectors[type] != 0) {
            hs_type_inspectors[type](type, *result, buffer);
            if (halo::hs::globals().preserve_token_case != 0 || halo::cseries::globals().debug_log_level >= 4) {
                halo::interface::chimera__console_out(0, buffer);
            }
        }
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "list_count"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a950
 */
void DebugCommands::evaluate_list_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index list = (datum_index)arguments[0];
    uint16_t count = 0;

    if (list != k_datum_index_none) {
        count = *(uint16_t *)((uint8_t *)halo::objects::globals().object_list_header_data->data + (list & halo::k_slot_mask) * 0xc + 6);
    }
    halo::hs::hs_thread_return((int32_t)count, thread_index);
    }
}

/**
 * Evaluate handler of hs function "list_get"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a900
 */
void DebugCommands::evaluate_list_get(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return(halo::hs::object_list_nth_reference((datum_index)arguments[0], *(int16_t *)&arguments[1]), thread_index);
    }
}

/**
 * Evaluate handler of hs function "message_metrics_dump"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480780
 */
void DebugCommands::evaluate_message_metrics_dump(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::networking::message_delta_metrics_dump((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "net_graph_clear"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4806b0
 */
void DebugCommands::evaluate_net_graph_clear(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::network_bandwidth_graph_instance_history_reset((network_bandwidth_graph *)network_bandwidth_graph_globals);
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "net_graph_show"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4806d0
 */
void DebugCommands::evaluate_net_graph_show(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return((int32_t)(uint8_t)(halo::networking::network_bandwidth_graph_set_units_command((const char *)arguments[0], (const char *)arguments[1])), thread_index);
    }
}

/**
 * Evaluate handler of hs function "print"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a3c0
 */
void DebugCommands::evaluate_print(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::interface::console_printf_verbose((ColorARGB *)actor_mode_default_look_weights, (char *)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "print_binds"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482480
 */
void DebugCommands::evaluate_print_binds(int16_t function_index, uint32_t thread_index, char first)
{
    halo::input::BindingNames::print_bound_controls();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "rasterizer_fixed_function_ambient"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480ff0
 */
void DebugCommands::evaluate_rasterizer_fixed_function_ambient(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint32_t level = (uint32_t)arguments[0] & 0xff;

        halo::rasterizer::fields::fixed_function_ambient_color = 0xff000000 | (level << 16) | (level << 8) | level;
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "rasterizer_lights_reset_for_new_map"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4810c0
 */
void DebugCommands::evaluate_rasterizer_lights_reset_for_new_map(int16_t function_index, uint32_t thread_index, char first)
{
    int32_t i;

    for (i = 0; i < 0x8c0; i++) {
        lens_flare_object_visibility_table[i] = 0;
    }
    for (i = 0; i < 0x4002; i++) {
        lens_flare_marker_visibility[i] = 0;
    }
    lens_flare_instance_count = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "rasterizer_model_ambient_reflection_tint"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481050
 */
void DebugCommands::evaluate_rasterizer_model_ambient_reflection_tint(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (rasterizer_model_ambient_reflection_tint != 0) {
            rasterizer_model_ambient_reflection_tint[0] = (uint32_t)arguments[0];
            rasterizer_model_ambient_reflection_tint[1] = (uint32_t)arguments[1];
            rasterizer_model_ambient_reflection_tint[2] = (uint32_t)arguments[2];
            rasterizer_model_ambient_reflection_tint[3] = (uint32_t)arguments[3];
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "render_lights"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b6a0
 */
void DebugCommands::evaluate_render_lights(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t value = *(uint8_t *)&arguments[0];

    *lights_enabled = value;
    halo::hs::hs_thread_return((int32_t)value, thread_index);
    }
}

/**
 * Evaluate handler of hs function "script_doc"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47acf0
 */
void DebugCommands::evaluate_script_doc(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_doc();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "script_recompile"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47acd0
 */
void DebugCommands::evaluate_script_recompile(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::globals().reload_pending = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "script_screen_effect_set_value"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4810f0
 */
void DebugCommands::evaluate_script_screen_effect_set_value(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = *(int16_t *)&arguments[0];

        if (cinematic_screen_effect_state != 0 && slot >= 0 && slot < 4) {
            *(uint32_t *)(cinematic_screen_effect_state + 0x64 + slot * 4) = (uint32_t)arguments[1];
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_gamma"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480fa0
 */
void DebugCommands::evaluate_set_gamma(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::rasterizer::globals().gamma_exponent = arguments[0];
        halo::rasterizer::chimera__gamma();
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Table of the hs functions handled by DebugCommands, in source order.
 */
EvaluateCommandTable DebugCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &DebugCommands::evaluate_help,
        &DebugCommands::evaluate_inspect,
        &DebugCommands::evaluate_list_count,
        &DebugCommands::evaluate_list_get,
        &DebugCommands::evaluate_message_metrics_dump,
        &DebugCommands::evaluate_net_graph_clear,
        &DebugCommands::evaluate_net_graph_show,
        &DebugCommands::evaluate_print,
        &DebugCommands::evaluate_print_binds,
        &DebugCommands::evaluate_rasterizer_fixed_function_ambient,
        &DebugCommands::evaluate_rasterizer_lights_reset_for_new_map,
        &DebugCommands::evaluate_rasterizer_model_ambient_reflection_tint,
        &DebugCommands::evaluate_render_lights,
        &DebugCommands::evaluate_script_doc,
        &DebugCommands::evaluate_script_recompile,
        &DebugCommands::evaluate_script_screen_effect_set_value,
        &DebugCommands::evaluate_set_gamma,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
