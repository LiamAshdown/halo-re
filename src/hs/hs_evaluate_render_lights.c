// hs_evaluate_render_lights  (not a Ghidra function; the evaluate handler of hs function 88 "render_lights" (boolean -> boolean))
// address 0x47b6a0, size 85 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b6a0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b6a0..0x47b6f5: stores the boolean through lights_enabled (0x0071cfb8, a pointer) and returns it in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t *lights_enabled; // 0x0071cfb8

void hs_evaluate_render_lights(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t value = *(uint8_t *)&arguments[0];

    *lights_enabled = value;
    hs_thread_return((int32_t)value, thread_index);
    }
}
