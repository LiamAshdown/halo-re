// hs_evaluate_ai_follow_target_players  (not a Ghidra function; the evaluate handler of hs function 231 "ai_follow_target_players" (ai -> void))
// address 0x47e430, size 87 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47e430, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47e430..0x47e487: the reference's encounter (low word; encounter_data 0x008802c8, 0x6c each) +0x62 word = 1; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *encounter_data; // 0x008802c8

void hs_evaluate_ai_follow_target_players(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t reference = (uint32_t)arguments[0];

    if (reference != 0xffffffff) {
        *(int16_t *)((uint8_t *)encounter_data->data + (reference & 0xffff) * 0x6c + 0x62) = 1;
    }
    hs_thread_return(0, thread_index);
    }
}
