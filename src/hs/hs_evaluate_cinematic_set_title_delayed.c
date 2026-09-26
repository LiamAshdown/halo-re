// hs_evaluate_cinematic_set_title_delayed  (not a Ghidra function; the evaluate handler of hs function 302 "cinematic_set_title_delayed" (cutscene_title, real -> void))
// address 0x47f960, size 74 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f960, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f960..0x47f9aa: cutscene_title_queue(stack: the title word zero-extended, the delay), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void cutscene_title_queue(int16_t title_index, float delay_seconds); // 0x449960

void hs_evaluate_cinematic_set_title_delayed(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    cutscene_title_queue(*(int16_t *)&arguments[0], *(float *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
