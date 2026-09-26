// hs_evaluate_activate_team_nav_point_object  (not a Ghidra function; the evaluate handler of hs function 371 "activate_team_nav_point_object" (navpoint, team, object, real -> void))
// address 0x4804f0, size 86 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4804f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4804f0..0x480546: hud_waypoint_activate_for_team(EAX object, stack: the navpoint and team words zero-extended, 1, the offset),
//   returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void hud_waypoint_activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind,
    float vertical_offset); // 0x4af1b0, blam-cc: EAX, stack

void hs_evaluate_activate_team_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_waypoint_activate_for_team((datum_index)arguments[2], *(int16_t *)&arguments[0], *(int16_t *)&arguments[1], 1,
        *(float *)&arguments[3]);
    hs_thread_return(0, thread_index);
    }
}
