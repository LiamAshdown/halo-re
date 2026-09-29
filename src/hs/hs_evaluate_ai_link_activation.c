// hs_evaluate_ai_link_activation  (not a Ghidra function; the evaluate handler of hs function 240 "ai_link_activation" (ai, ai -> void))
// address 0x47e690, size 83 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47e690, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47e690..0x47e6e3: with both references set: ai_encounter_record_recent_zone(EAX = the first's encounter (low word), SI = the
//   second); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_link_activation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t first_reference = (uint32_t)arguments[0];
    uint32_t second_reference = (uint32_t)arguments[1];

    if (first_reference != 0xffffffff && second_reference != 0xffffffff) {
        ai_encounter_record_recent_zone(first_reference & 0xffff, (int16_t)second_reference);
    }
    hs_thread_return(0, thread_index);
    }
}
