// hs_evaluate_custom_animation  (not a Ghidra function; the evaluate handler of hs function 101 "custom_animation" (unit, animation_graph, string, boolean -> boolean))
// address 0x47bad0, size 102 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47bad0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47bad0..0x47bb36: unit_start_user_animation(stack: unit, the interpolate byte zero-extended; EDI graph, EAX name) into a zeroed
//   dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name,
    uint8_t interpolate); // 0x5702a0, blam-cc: stack, EDI, EAX, stack

void hs_evaluate_custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)unit_start_user_animation((uint32_t)arguments[0], (datum_index)arguments[1],
        (const char *)arguments[2], *(uint8_t *)&arguments[3]), thread_index);
    }
}
