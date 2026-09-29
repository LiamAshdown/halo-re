// hs_evaluate_crash  (not a Ghidra function; the evaluate handler of hs function 266 "crash" (string -> void))
// address 0x47f5d0, size 66 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f5d0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f5d0..0x47f611: deliberately crashes: stores the address of "chucky was here!
//   NULL belongs to me!!!!!" at address 0 (the return after it is never reached).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_crash(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        *(volatile const char **)0 = "chucky was here! NULL belongs to me!!!!!"; // 0x0066b288
        hs_thread_return(0, thread_index);
    }
}
