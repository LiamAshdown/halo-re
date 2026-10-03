// hs_evaluate_objects_can_see_flag  (not a Ghidra function; the evaluate handler of hs function 52 "objects_can_see_flag" (object_list, cutscene_flag, real -> boolean))
// address 0x47ab60, size 95 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ab60, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ab60..0x47abbf: hs_object_list_any_angle_match_gated(EAX list, BX flag, stack degrees) into a zeroed dword.
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
extern uint32_t hs_object_list_any_angle_match_gated(datum_index header_index, int16_t gate,
    float angle_degrees); // 0x487ad0, blam-cc: EAX, BX, stack

void hs_evaluate_objects_can_see_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint8_t)hs_object_list_any_angle_match_gated((datum_index)arguments[0],
        *(int16_t *)&arguments[1], *(float *)&arguments[2]), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
