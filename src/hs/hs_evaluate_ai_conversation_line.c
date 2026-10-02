// hs_evaluate_ai_conversation_line  (not a Ghidra function; the evaluate handler of hs "ai_conversation_line" (conversation -> short))
// address 0x47ec90, size 88 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47ec90, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47ec90: BX = the conversation (+0x0 word); the short result is returned zero-extended.
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
extern int16_t ai_conversation_get_unknown_48(int16_t conversation_definition_index); // 0x430960, BX

void hs_evaluate_ai_conversation_line(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t line = ai_conversation_get_unknown_48(*(int16_t *)&arguments[0]);
        hs_thread_return((int32_t)(uint16_t)line, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
