// hs_evaluate_ai_set_respawn  (not a Ghidra function; the evaluate handler of hs function 170 "ai_set_respawn" (ai, boolean -> void))
// address 0x47d3b0, size 133 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47d3b0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47d3b0..0x47d434: with a valid encounter reference while encounters are live
//   (ai_globals +1): the encounter's respawn byte (+0x3c) takes the boolean, its +0x0e word becomes 0x96 and it is
//   activated. Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t *ai_global_data; // 0x00880354 (ai_globals *; +0 enabled, +1 encounters live)
extern data_array *encounter_data; // 0x008802c8
extern uint8_t encounter_activate(datum_index encounter_index); // 0x437710, blam-cc: ECX

void hs_evaluate_ai_set_respawn(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1 && ai_global_data[1] != 0) {
            uint32_t index = (uint32_t)arguments[0] & 0xffff;
            uint8_t *encounter = (uint8_t *)encounter_data->data + index * 0x6c;

            encounter[0x3c] = (uint8_t)arguments[1];
            *(int16_t *)((uint8_t *)encounter_data->data + index * 0x6c + 0xe) = 0x96;
            encounter_activate((datum_index)index);
        }
        hs_thread_return(0, thread_index);
    }
}
