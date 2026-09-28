// hs_evaluate_ai_set_deaf  (not a Ghidra function; the evaluate handler of hs "ai_set_deaf" (ai, boolean -> void))
// address 0x47d440, size 103 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d440, only reachable through that pointer.
//   Campaign track: 5 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d440: with actors valid (ai globals +1), the encounter record (0x6c, index = ai & 0xffff) +0x41 = the boolean; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern data_array *encounter_data; // 0x008802c8, 0x6c-byte encounter records
extern ai_globals *ai_globals_ptr;

void hs_evaluate_ai_set_deaf(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff && ai_globals_ptr->actors_valid != 0) {
            ((uint8_t *)encounter_data->data)[(arguments[0] & 0xffff) * 0x6c + 0x41] = *(uint8_t *)&arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}
