// hs_evaluate_ai_playfight  (not a Ghidra function; the evaluate handler of hs function 218 "ai_playfight" (ai, boolean -> void))
// address 0x47e070, size 88 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47e070, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47e070..0x47e0c8: the reference's encounter (low word; encounter_data 0x008802c8, 0x6c each) +0x60 = the boolean; returns 0.
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
extern data_array *encounter_data; // 0x008802c8

void hs_evaluate_ai_playfight(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint32_t reference = (uint32_t)arguments[0];

    if (reference != 0xffffffff) {
        ((uint8_t *)encounter_data->data)[(reference & 0xffff) * 0x6c + 0x60] = *(uint8_t *)&arguments[1];
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
