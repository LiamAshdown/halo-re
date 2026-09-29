// hs_evaluate_camera_control  (not a Ghidra function; the evaluate handler of hs function 246 "camera_control" (boolean -> void))
// address 0x47edf0, size 69 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47edf0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47edf0..0x47ee35: camera_control(stack: the boolean zero-extended), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void camera_control(uint8_t enable); // 0x445cc0

void hs_evaluate_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    camera_control(*(uint8_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
