// hs_evaluate_ai_strength  (not a Ghidra function; the evaluate handler of hs function 190 "ai_strength" (ai -> real))
// address 0x47ea40, size 92 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ea40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ea40..0x47ea9c: ai_reference_get_stat_pair(EAX ai, EDI 0, stack: 0, &strength) with strength starting at 0; returns strength.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint32_t ai_reference_get_stat_pair(uint32_t packed_reference, int16_t stat_kind, int32_t *out_member_count,
    uint32_t *out_extra); // 0x432f90, blam-cc: EAX, EDI, stack

void hs_evaluate_ai_strength(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t strength = 0;

    ai_reference_get_stat_pair((uint32_t)arguments[0], 0, 0, &strength);
    hs_thread_return((int32_t)strength, thread_index);
    }
}
