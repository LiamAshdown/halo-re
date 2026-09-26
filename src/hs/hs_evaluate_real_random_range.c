// hs_evaluate_real_random_range  (not a Ghidra function; the evaluate handler of hs function 60 "real_random_range" (real, real -> real))
// address 0x47ad90, size 122 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ad90, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ad90..0x47ae0a: steps the global random seed (0x00719cd0, seed * 0x19660d + 0x3c6ef35f) and returns
//   low + (high - low) * ((seed >> 16) * 1/65535 (0x672b84)) as a float.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint32_t random_seed_global; // 0x00719cd0

void hs_evaluate_real_random_range(int16_t function_index, uint32_t thread_index, char first)
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
