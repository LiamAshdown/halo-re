// hs_evaluate_ai_follow_target_unit  (not a Ghidra function; the evaluate handler of hs function 232 "ai_follow_target_unit" (ai, unit -> void))
// address 0x47e490, size 113 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e490 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e490..0x47e500: for a valid encounter: without a unit its follow mode word
//   (+0x62) becomes 0; with one, 2 and the unit goes to +0x64. Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern data_array *encounter_data; // 0x008802c8

void hs_evaluate_ai_follow_target_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *encounter = (uint8_t *)encounter_data->data + ((uint32_t)arguments[0] & 0xffff) * 0x6c;

            if (arguments[1] == -1) {
                ((struct encounter *)encounter)->follow_target_type = 0;
            } else {
                ((struct encounter *)encounter)->follow_target_type = 2;
                ((struct encounter *)encounter)->follow_target = arguments[1];
            }
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
