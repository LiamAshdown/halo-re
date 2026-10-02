// hs_evaluate_units_set_current_vitality  (not a Ghidra function; the evaluate handler of hs function 115 "units_set_current_vitality" (object_list, real, real -> void))
// address 0x47c100, size 74 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c100, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c100..0x47c14a: ai_object_list_update_vitality_fractions(EAX list, stack: body, shield), returns 0.
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
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_object_list_update_vitality_fractions(datum_index object_list_header_handle, float body_delta,
    float shield_delta); // 0x561cb0, blam-cc: EAX, stack

void hs_evaluate_units_set_current_vitality(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_object_list_update_vitality_fractions((datum_index)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
