// hs_evaluate_ai_follow_target_ai  (not a Ghidra function; the evaluate handler of hs "ai_follow_target_ai" (ai, ai -> void))
// address 0x47e510, size 113 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47e510, only reachable through that pointer.
//   Campaign track: 4 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47e510: the encounter record (0x6c) +0x62 = 3 and +0x64 = the target ai, or +0x62 = 0 for none; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *encounter_data; // 0x008802c8, 0x6c-byte encounter records
extern uint8_t *ai_globals_ptr; // 0x00880354, byte +0x1 = actors valid

void hs_evaluate_ai_follow_target_ai(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            uint8_t *encounter = (uint8_t *)encounter_data->data + (arguments[0] & 0xffff) * 0x6c;

            if ((uint32_t)arguments[1] == 0xffffffff) {
                *(int16_t *)(encounter + 0x62) = 0;
            } else {
                *(int16_t *)(encounter + 0x62) = 3;
                *(uint32_t *)(encounter + 0x64) = (uint32_t)arguments[1];
            }
        }
        hs_thread_return(0, thread_index);
    }
}
