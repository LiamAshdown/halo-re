// hs_evaluate_random_range  (not a Ghidra function; the evaluate handler of hs function 59 "random_range")
// address 0x47ad00, size 132 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[59] -> record 0x657d34, name "random_range", return type 7
//   (short), parameters (7 short, 7 short); evaluate (+0xc) 0x47ad00, only reachable through that pointer.
//   Campaign track: a level script reached it from hs_runtime_update.
// objdump 0x47ad00..0x47ad83: the standard builtin evaluator; with arguments it steps the global random seed
//   (0x00719cd0, seed * 0x19660d + 0x3c6ef35f) and returns low + ((high - low) * (seed >> 16)) >> 16 as a
//   word in a dword that starts zeroed (hs_thread_return, ECX = thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern uint32_t random_seed_global; // 0x00719cd0

extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

void hs_evaluate_random_range(int16_t function_index, uint32_t thread_index, char first)
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
