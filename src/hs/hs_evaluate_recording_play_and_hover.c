// hs_evaluate_recording_play_and_hover  (not a Ghidra function; the evaluate handler of hs "recording_play_and_hover" (unit, cutscene_recording -> boolean))
// address 0x47b010, size 93 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47b010, only reachable through that pointer.
//   Campaign track: 49 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47b010: EAX = unit (+0x0), CX = recording (+0x4 word), stack 0x10 (hover); the byte result is returned zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_cutscene.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_recording_play_and_hover(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t result = recorded_animation_start((datum_index)arguments[0], (int16_t)*(uint16_t *)&arguments[1], 0x10);
        hs_thread_return((int32_t)result, thread_index);
    }
}
