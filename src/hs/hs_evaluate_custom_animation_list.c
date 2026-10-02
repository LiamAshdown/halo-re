// hs_evaluate_custom_animation_list  (not a Ghidra function; the evaluate handler of hs function 102 "custom_animation_list" (object_list, animation_graph, string, boolean -> boolean))
// address 0x47bb40, size 99 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47bb40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47bb40..0x47bba3: ai_object_list_start_user_animation_until_failure(ECX list, stack: graph, name, interpolate byte) into a zeroed
//   dword.
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
extern uint8_t ai_object_list_start_user_animation_until_failure(datum_index object_list_header_handle,
    datum_index graph_tag_id, const char *animation_name, uint8_t interpolate); // 0x561e60, blam-cc: ECX, stack

void hs_evaluate_custom_animation_list(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)ai_object_list_start_user_animation_until_failure((datum_index)arguments[0],
        (datum_index)arguments[1], (const char *)arguments[2], *(uint8_t *)&arguments[3]), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
