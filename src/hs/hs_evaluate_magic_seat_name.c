// hs_evaluate_magic_seat_name  (not a Ghidra function; the evaluate handler of hs function 118 "magic_seat_name" (string -> void))
// address 0x47c210, size 71 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c210 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c210..0x47c256: stores the base animation state named by the string
//   (0x56eb90) in 0x0069fde0; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern int16_t unit_base_animation_state_from_name(const char *name); // 0x56eb90, blam-cc: EDI name
extern int16_t magic_seat_animation_state_0069fde0; // 0x0069fde0, UNSURE name

void hs_evaluate_magic_seat_name(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        magic_seat_animation_state_0069fde0 = unit_base_animation_state_from_name((const char *)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
