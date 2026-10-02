// hs_evaluate_script_screen_effect_set_value  (not a Ghidra function; the evaluate handler of hs "script_screen_effect_set_value" (short, real -> void))
// address 0x4810f0, size 90 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x4810f0, only reachable through that pointer.
//   Campaign track: 2 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x4810f0: with the screen effect block present and the slot (+0x0 word) in 0..3, block +0x64 + slot * 4 = the value (+0x4); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *cinematic_screen_effect_state; // 0x0071cfc4, four script values at +0x64

void hs_evaluate_script_screen_effect_set_value(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = *(int16_t *)&arguments[0];

        if (cinematic_screen_effect_state != 0 && slot >= 0 && slot < 4) {
            *(uint32_t *)(cinematic_screen_effect_state + 0x64 + slot * 4) = (uint32_t)arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
