// hs_evaluate_numeric_countdown_timer_get  (not a Ghidra function; the evaluate handler of hs function 62 "numeric_countdown_timer_get" (short -> short))
// address 0x47ae60, size 84 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47ae60 trapped.
// WRITTEN 2026-09-28 from objdump 0x47ae60..0x47aeb3: returns countdown timer digit (0x5400c0) of the short
//   argument as a short.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern int16_t numeric_countdown_timer_get_digit(int16_t digit_index); // 0x5400c0, blam-cc: AX

void hs_evaluate_numeric_countdown_timer_get(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint16_t)(numeric_countdown_timer_get_digit((int16_t)arguments[0])), thread_index);
    }
}
