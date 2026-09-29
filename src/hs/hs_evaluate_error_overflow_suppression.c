// hs_evaluate_error_overflow_suppression  (not a Ghidra function; the evaluate handler of hs function 389 "error_overflow_suppression" (boolean -> void))
// address 0x4807d0, size 64 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4807d0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4807d0..0x48080f: stores the boolean in console_debug_flag_4 (0x0087ac04);
//   returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t console_debug_flag_4; // 0x0087ac04

void hs_evaluate_error_overflow_suppression(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        console_debug_flag_4 = (uint8_t)arguments[0];
        hs_thread_return(0, thread_index);
    }
}
