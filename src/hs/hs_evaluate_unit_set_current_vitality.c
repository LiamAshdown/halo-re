// hs_evaluate_unit_set_current_vitality  (not a Ghidra function; the evaluate handler of hs function 114 "unit_set_current_vitality" (unit, real, real -> void))
// address 0x47c0b0, size 74 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c0b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c0b0..0x47c0fa: unit_update_vitality_fractions(EAX unit, stack: body, shield), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta); // 0x561b80, EAX, stack

void hs_evaluate_unit_set_current_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    unit_update_vitality_fractions((uint32_t)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
