// hs_evaluate_ai_set_team  (not a Ghidra function; the evaluate handler of hs function 242 "ai_set_team" (ai, team -> void))
// address 0x47e740, size 67 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e740 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e740..0x47e782: encounter_set_team (0x435b30) with the ai and the team;
//   returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void encounter_set_team(datum_index encounter_index, int16_t team); // 0x435b30, blam-cc: EAX, CX

void hs_evaluate_ai_set_team(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        encounter_set_team((datum_index)arguments[0], (int16_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
