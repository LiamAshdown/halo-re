// hs_evaluate_recording_kill  (not a Ghidra function; the evaluate handler of hs function 69 "recording_kill" (unit -> void))
// address 0x47b070, size 80 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b070, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b070..0x47b0c0: the unit's recording (recorded_animation_find_by_object, EBX unit, stack 0) gets flags byte +0x0a |= 3;
//   returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "cutscene.h"
#include "fn_hs.h"
#include "fn_cutscene.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


    // 0x44ad20, blam-cc: EBX, stack

void hs_evaluate_recording_kill(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *recording = (uint8_t *)recorded_animation_find_by_object((datum_index)arguments[0], 0);

    if (recording != 0) {
        recording[0xa] |= 3;
    }
    hs_thread_return(0, thread_index);
    }
}
