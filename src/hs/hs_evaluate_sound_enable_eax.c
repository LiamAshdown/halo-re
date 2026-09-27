// hs_evaluate_sound_enable_eax  (not a Ghidra function; the evaluate handler of hs "sound_enable_eax" (boolean -> void))
// address 0x481560, size 68 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x481560, only reachable through that pointer.
//   Campaign track: 3 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x481560: stack the boolean (zero-extended); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void sound_effects_object_reinitialize(int enable); // 0x5514d0, stack

void hs_evaluate_sound_enable_eax(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        sound_effects_object_reinitialize((int)*(uint8_t *)&arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
