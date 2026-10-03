#include "halo/hs/hs2_commands.hpp"


#ifdef __cplusplus
extern "C" {
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern uint32_t random_seed_global;
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
#ifdef __cplusplus
}
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int16_t *arguments = (int16_t *)hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t low = arguments[0];
        int16_t high = arguments[2];
        uint16_t result;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        result = (uint16_t)(((uint32_t)((int32_t)high - (int32_t)low) * (random_seed_global >> 0x10)) >> 0x10);
        result = (uint16_t)(result + (uint16_t)low);
        hs_thread_return((int32_t)result, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float low = *(float *)&arguments[0];
    float high = *(float *)&arguments[1];
    float result;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    result = (high - low) * ((float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f) + low;
    hs_thread_return(*(int32_t *)&result, thread_index);
    }
}

}
