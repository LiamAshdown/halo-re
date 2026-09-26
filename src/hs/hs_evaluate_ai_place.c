// hs_evaluate_ai_place  (not a Ghidra function; the evaluate handler of hs function 162 "ai_place" (ai -> void))
// address 0x47d1f0, size 67 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47d1f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47d1f0..0x47d233: ai_reference_activate_squads(stack ai), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_reference_activate_squads(uint32_t packed_reference); // 0x432b80, stack

void hs_evaluate_ai_place(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_reference_activate_squads((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
