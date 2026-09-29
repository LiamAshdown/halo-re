// hs_evaluate_cinematic_screen_effect_start  (not a Ghidra function; the evaluate handler of hs function 422 "cinematic_screen_effect_start" (boolean -> void))
// address 0x481150, size 99 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481150, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481150..0x4811b3: with the screen effect state (*0x0071cfc4): true, or false while +0x39 is clear, zeroes its first 0x38 bytes and
//   sets +0x39; +0x38 = 1 either way; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t *cinematic_screen_effect_state; // 0x0071cfc4

void hs_evaluate_cinematic_screen_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        if (*(uint8_t *)&arguments[0] || !cinematic_screen_effect_state[0x39]) {
            int32_t i;

            for (i = 0; i < 0xe; i++) {
                ((uint32_t *)cinematic_screen_effect_state)[i] = 0;
            }
            cinematic_screen_effect_state[0x39] = 1;
        }
        cinematic_screen_effect_state[0x38] = 1;
    }
    hs_thread_return(0, thread_index);
    }
}
