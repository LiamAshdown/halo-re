// hs_evaluate_numeric_countdown_timer_set  (not a Ghidra function; the evaluate handler of hs "numeric_countdown_timer_set" (long, boolean -> void))
// address 0x47ae10, size 73 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47ae10, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47ae10: the milliseconds (+0x0) go to 0x721e50 and the running flag (+0x4 byte) to 0x721e54; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern int32_t numeric_countdown_timer_remaining_ms; // 0x00721e50
extern uint8_t numeric_countdown_timer_running; // 0x00721e54

void hs_evaluate_numeric_countdown_timer_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        numeric_countdown_timer_remaining_ms = arguments[0];
        numeric_countdown_timer_running = *(uint8_t *)&arguments[1];
        hs_thread_return(0, thread_index);
    }
}
