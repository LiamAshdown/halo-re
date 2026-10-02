// hs_evaluate_ai_vehicle_enterable_actors  (not a Ghidra function; the evaluate handler of hs function 225 "ai_vehicle_enterable_actors" (unit, ai -> void))
// address 0x47e240, size 107 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e240 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e240..0x47e2aa: for a unit and an ai: appends the ai to the (at most 6)
//   enterable actors of the unit's ai attention record (+0x0c count, +0x10 list); returns 0.
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

void hs_evaluate_ai_vehicle_enterable_actors(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1 && arguments[1] != -1) {
            uint8_t *record = ai_object_attention_find_or_create((datum_index)arguments[0]);

            if (record != 0 && *(int16_t *)(record + 0xc) < 6) {
                ((int32_t *)(record + 0x10))[*(int16_t *)(record + 0xc)] = arguments[1];
                *(int16_t *)(record + 0xc) += 1;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
