// hs_evaluate_ai_conversation  (not a Ghidra function; the evaluate handler of hs function 235 "ai_conversation" (conversation -> boolean))
// address 0x47ec30, size 88 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ec30, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ec30..0x47ec88: ai_conversation_activate(AX conversation, stack 1) into a zeroed dword.
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
extern uint8_t ai_conversation_activate(int16_t conversation_definition_index, uint8_t allow_eviction); // 0x4307c0, AX, stack

void hs_evaluate_ai_conversation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)ai_conversation_activate(*(int16_t *)&arguments[0], 1), thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
