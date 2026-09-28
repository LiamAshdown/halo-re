// hs_evaluate_sound_set_gain  (not a Ghidra function; the evaluate handler of hs function 54 "sound_set_gain" (string, real -> void))
// address 0x47ac10, size 82 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47ac10 trapped.
// WRITTEN 2026-09-28 from objdump 0x47ac10..0x47ac61: looks up the named gain (0x488b10) and, when found, stores
//   the real in it; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern float *hs_sound_get_gain_reference(char *name); // 0x488b10, blam-cc: EAX name

void hs_evaluate_sound_set_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float *gain = hs_sound_get_gain_reference((char *)arguments[0]);

        if (gain != 0) {
            *gain = *(float *)&arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}
