// hs_evaluate_rasterizer_fixed_function_ambient  (not a Ghidra function; the evaluate handler of hs "rasterizer_fixed_function_ambient" (long -> void))
// address 0x480ff0, size 86 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x480ff0, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x480ff0: the low byte becomes an opaque grey 0xFFvvvvvv stored at 0x69c684; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint32_t renderer_unknown_69c684; // 0x0069c684, the fixed function ambient colour

void hs_evaluate_rasterizer_fixed_function_ambient(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint32_t level = (uint32_t)arguments[0] & 0xff;

        renderer_unknown_69c684 = 0xff000000 | (level << 16) | (level << 8) | level;
        hs_thread_return(0, thread_index);
    }
}
