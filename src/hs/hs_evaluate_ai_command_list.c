// hs_evaluate_ai_command_list  (not a Ghidra function; the evaluate handler of hs function 208 "ai_command_list" (ai, ai_command_list -> void))
// address 0x47dda0, size 69 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47dda0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47dda0..0x47dde5: ai_reference_flee_if_ready(EAX ai, BX = the command list word), returns 0.
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
extern void ai_reference_flee_if_ready(uint32_t packed_reference, uint32_t readiness_param); // 0x434d90, blam-cc: EAX, EBX

void hs_evaluate_ai_command_list(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_reference_flee_if_ready((uint32_t)arguments[0], *(uint16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
