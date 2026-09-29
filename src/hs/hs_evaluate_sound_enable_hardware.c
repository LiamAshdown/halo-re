// hs_evaluate_sound_enable_hardware  (not a Ghidra function; the evaluate handler of hs function 440 "sound_enable_hardware" (boolean, boolean -> void))
// address 0x481650, size 72 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481650 trapped.
// WRITTEN 2026-09-28 from objdump 0x481650..0x481697: sound_driver_set_eax_enabled (0x548200) with the first
//   boolean and the second as force; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force); // 0x548200, blam-cc: stack, CL

void hs_evaluate_sound_enable_hardware(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        sound_driver_set_eax_enabled((uint8_t)arguments[0], (uint8_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
