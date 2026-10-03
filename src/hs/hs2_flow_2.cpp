#include "halo/hs/records.hpp"
#include "halo/hs/hs2_commands.hpp"
#include "halo/math/api.hpp"
#include "halo/core/lcg.hpp"
#include "halo/hs/api.hpp"


#ifdef __cplusplus
#endif
#ifdef __cplusplus
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "random_range"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ad00
 */
void FlowCommands::evaluate_random_range(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int16_t *arguments = (int16_t *)halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t low = arguments[0];
        int16_t high = arguments[2];
        uint16_t result;

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        result = (uint16_t)(((uint32_t)((int32_t)high - (int32_t)low) * (halo::math::globals().random_seed_global >> 0x10)) >> 0x10);
        result = (uint16_t)(result + (uint16_t)low);
        halo::hs::hs_thread_return((int32_t)result, thread_index);
    }
}

/**
 * Evaluate handler of hs function "real_random_range"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ad90
 */
void FlowCommands::evaluate_real_random_range(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    float low = halo::hs::argument_real(arguments[0]);
    float high = halo::hs::argument_real(arguments[1]);
    float result;

    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    result = (high - low) * ((float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f) + low;
    halo::hs::hs_thread_return(*(int32_t *)&result, thread_index);
    }
}

}
