// hs_evaluate_ai_vehicle_enterable_team  (not a Ghidra function; the evaluate handler of hs function 223 "ai_vehicle_enterable_team" (unit, team -> void))
// address 0x47e180, size 95 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e180 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e180..0x47e1de: for a unit: sets bit (team) of the word +0x08 of its ai
//   attention record (0x435900, created on demand); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t *ai_object_attention_find_or_create(datum_index object_index); // 0x435900, blam-cc: EDI

void hs_evaluate_ai_vehicle_enterable_team(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *record = ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0) {
                *(uint16_t *)(record + 8) |= (uint16_t)(1u << ((int16_t)arguments[1] & 0x1f));
            }
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
