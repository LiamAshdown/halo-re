// hs_evaluate_deactivate_team_nav_point_object  (not a Ghidra function; the evaluate handler of hs function 375 "deactivate_team_nav_point_object" (team, object -> void))
// address 0x480660, size 79 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480660, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480660..0x4806af: hud_waypoint_deactivate_for_team(EAX 1, stack: the team word zero-extended, the object), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void hud_waypoint_deactivate_for_team(int16_t kind, int16_t team, datum_index target); // 0x4af2b0, EAX, stack

void hs_evaluate_deactivate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_waypoint_deactivate_for_team(1, *(int16_t *)&arguments[0], (datum_index)arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
