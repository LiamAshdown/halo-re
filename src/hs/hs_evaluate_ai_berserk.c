// hs_evaluate_ai_berserk  (not a Ghidra function; the evaluate handler of hs function 241 "ai_berserk" (ai, boolean -> void))
// address 0x47e6f0, size 66 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47e6f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47e6f0..0x47e732: ai_reference_set_combat_alert_flag(EAX ai, CL boolean), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_berserk(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_reference_set_combat_alert_flag((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
