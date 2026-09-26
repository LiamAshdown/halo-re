// hs_evaluate_ai_allegiance  (not a Ghidra function; the evaluate handler of hs function 186 "ai_allegiance" (team, team -> void))
// address 0x47d920, size 74 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47d920, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47d920..0x47d96a: ai_category_matches_wildcard(AX = the second team, stack: the first team zero-extended), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_category_matches_wildcard(int16_t category, int16_t other_category); // 0x433ba0, blam-cc: stack, AX

void hs_evaluate_ai_allegiance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_category_matches_wildcard(*(int16_t *)&arguments[0], *(int16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
