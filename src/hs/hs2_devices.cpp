#include "halo/ai/records.hpp"
#include "halo/hs/hs2_commands.hpp"
#include "halo/devices/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"


#ifdef __cplusplus
#endif
#ifdef __cplusplus
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "device_set_position"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ca40
 */
void DeviceCommands::evaluate_device_set_position(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];
    uint8_t result = 0;

    if (device != k_datum_index_none) {
        uint8_t *object = reinterpret_cast<uint8_t *>(halo::ai::object_at(device));
        uint16_t group = *(uint16_t *)(object + 0x204);

        if (group != halo::k_word_none) {
            result = halo::devices::device_group_set_value(group, *(float *)&arguments[1]);
        }
    }
    halo::hs::hs_thread_return((int32_t)result, thread_index);
    }
}

/**
 * Evaluate handler of hs function "device_set_position_immediate"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47cb60
 */
void DeviceCommands::evaluate_device_set_position_immediate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];

    if (device != k_datum_index_none) {
        uint16_t group = *(uint16_t *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(device)) + 0x204);

        if (group != halo::k_word_none) {
            halo::devices::device_group_set_value_immediate(group, *(float *)&arguments[1]);
        }
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "device_set_power"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47c930
 */
void DeviceCommands::evaluate_device_set_power(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index device = (datum_index)arguments[0];
    float power = *(float *)&arguments[1];

    if (device != k_datum_index_none) {
        uint8_t *object = reinterpret_cast<uint8_t *>(halo::ai::object_at(device));

        *(uint32_t *)(object + 0x1f4) |= 4;
        *(float *)(object + 0x1fc) = power;
        halo::devices::device_group_set_value(*(uint16_t *)(object + 0x1f8), power);
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Table of the hs functions handled by DeviceCommands, in source order.
 */
EvaluateCommandTable DeviceCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &DeviceCommands::evaluate_device_set_position,
        &DeviceCommands::evaluate_device_set_position_immediate,
        &DeviceCommands::evaluate_device_set_power,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
