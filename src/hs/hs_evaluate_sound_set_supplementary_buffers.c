// hs_evaluate_sound_set_supplementary_buffers  (not a Ghidra function; the evaluate handler of hs function 441 "sound_set_supplementary_buffers" (short, boolean -> void))
// address 0x4816a0, size 124 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4816a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4816a0..0x48171b: the short (out of 0..2 means 2) becomes the supplementary
//   buffer count (0x00746122); when it changed and the boolean is set, re-applies EAX (0x548200 with 0x00746121,
//   forced). Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern int16_t sound_supplementary_buffers_00746122; // 0x00746122, UNSURE name
extern uint8_t directsound_eax_enabled; // 0x00746121
extern void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force); // 0x548200, blam-cc: stack, CL

void hs_evaluate_sound_set_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t count = (int16_t)arguments[0];
        uint8_t changed = 0;

        if (count < 0 || count > 2) {
            count = 2;
        }
        if (sound_supplementary_buffers_00746122 != count) {
            sound_supplementary_buffers_00746122 = count;
            changed = 1;
        }
        if ((uint8_t)arguments[1] && changed) {
            sound_driver_set_eax_enabled(directsound_eax_enabled, 1);
        }
        hs_thread_return(0, thread_index);
    }
}
