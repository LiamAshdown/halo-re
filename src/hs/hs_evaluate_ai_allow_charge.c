// hs_evaluate_ai_allow_charge  (not a Ghidra function; the evaluate handler of hs function 243 "ai_allow_charge" (ai, boolean -> void))
// address 0x47e790, size 72 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e790 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e790..0x47e7d7: ai_reference_set_unknown_1cb (0x434d40) with the ai and the
//   boolean; returns 0.
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
extern void ai_reference_set_unknown_1cb(uint32_t packed_reference, char flag); // 0x434d40, blam-cc: EAX, stack

void hs_evaluate_ai_allow_charge(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_set_unknown_1cb((uint32_t)arguments[0], (char)(uint8_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
