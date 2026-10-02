// hs_evaluate_ai_allegiance_remove  (not a Ghidra function; the evaluate handler of hs function 187 "ai_allegiance_remove" (team, team -> void))
// address 0x47d970, size 88 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47d970, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47d970..0x47d9c8: with both teams set: team_pair_override_remove(EAX = the second team, stack: the first (movsx)); returns 0.
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
extern uint32_t team_pair_override_remove(int16_t index_a, int16_t index_b); // 0x45bf10, blam-cc: EAX, stack

void hs_evaluate_ai_allegiance_remove(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t first_team = *(int16_t *)&arguments[0];
    int16_t second_team = *(int16_t *)&arguments[1];

    if (first_team != -1 && second_team != -1) {
        team_pair_override_remove(second_team, first_team);
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
