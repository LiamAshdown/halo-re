// hs_evaluate_unit_custom_animation_at_frame  (not a Ghidra function; the evaluate handler of hs function 100 "unit_custom_animation_at_frame" (unit, animation_graph, string, boolean, short -> boolean))
// address 0x47bbb0, size 106 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47bbb0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47bbb0..0x47bc1a: unit_set_custom_animation_frame(ECX unit, AL interpolate, EDI graph, stack: name, the frame word zero-extended)
//   into a zeroed dword.
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
extern uint8_t unit_set_custom_animation_frame(uint32_t unit_index, uint8_t warn_if_missing,
    datum_index graph_tag_id, const char *animation_name, int16_t frame); // 0x570220, blam-cc: ECX, EAX, EDI, stack

void hs_evaluate_unit_custom_animation_at_frame(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)unit_set_custom_animation_frame((uint32_t)arguments[0], *(uint8_t *)&arguments[3],
        (datum_index)arguments[1], (const char *)arguments[2], *(int16_t *)&arguments[4]), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
