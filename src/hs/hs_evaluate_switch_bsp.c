// hs_evaluate_switch_bsp  (not a Ghidra function; the evaluate handler of hs function 267 "switch_bsp" (short -> void))
// address 0x47f620, size 66 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f620, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f620..0x47f662: scenario_structure_bsp_switch(SI index), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index); // 0x53eeb0, blam-cc: SI

void hs_evaluate_switch_bsp(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    scenario_structure_bsp_switch(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
