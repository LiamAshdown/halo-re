// hs_evaluate_ai_allegiance_broken  (not a Ghidra function; the evaluate handler of hs "ai_allegiance_broken" (team, team -> boolean))
// address 0x47ed50, size 155 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47ed50, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47ed50: both teams valid: team_pair_flag_test(ECX a, EDX b) and teams_are_enemies(CX b, DX a); byte result zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b); // 0x45bdb0, ECX, EDX
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX

void hs_evaluate_ai_allegiance_broken(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t team_a = *(int16_t *)&arguments[0];
        int16_t team_b = *(int16_t *)&arguments[1];
        uint8_t broken = 0;

        if (team_a != -1 && team_b != -1 && team_pair_flag_test(team_a, team_b) &&
            teams_are_enemies(team_b, team_a)) {
            broken = 1;
        }
        hs_thread_return((int32_t)broken, thread_index);
    }
}
