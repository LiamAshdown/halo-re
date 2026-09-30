// hs_evaluate_recording_play  (not a Ghidra function; the evaluate handler of hs function 66 "recording_play" (unit, cutscene_recording -> boolean))
// address 0x47af50, size 93 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47af50, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47af50..0x47afad: recorded_animation_start(EAX unit, CX recording zero-extended, stack 0) into a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_cutscene.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


    // 0x44a930, blam-cc: EAX, CX, stack

void hs_evaluate_recording_play(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)recorded_animation_start((datum_index)arguments[0], *(int16_t *)&arguments[1], 0), thread_index);
    }
}
