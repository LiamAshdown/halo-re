// hs_evaluate_object_teleport  (not a Ghidra function; the evaluate handler of hs function 34 "object_teleport" (object, cutscene_flag -> void))
// address 0x47a7c0, size 75 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a7c0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a7c0..0x47a80b: hs_object_detach_and_place_at_location(AX flag, stack: object, 1, 1), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_object_teleport(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_object_detach_and_place_at_location(*(int16_t *)&arguments[1], (datum_index)arguments[0], 1, 1);
    hs_thread_return(0, thread_index);
    }
}
