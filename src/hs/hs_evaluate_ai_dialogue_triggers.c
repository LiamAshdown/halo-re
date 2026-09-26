// hs_evaluate_ai_dialogue_triggers  (not a Ghidra function; the evaluate handler of hs function 285 "ai_dialogue_triggers" (boolean -> void))
// address 0x47cf30, size 67 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47cf30, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47cf30..0x47cf73: ai globals (*0x00880354) +0x10 = the boolean, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *ai_globals_ptr; // 0x00880354

void hs_evaluate_ai_dialogue_triggers(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_globals_ptr[0x10] = *(uint8_t *)&arguments[0];
    hs_thread_return(0, thread_index);
    }
}
