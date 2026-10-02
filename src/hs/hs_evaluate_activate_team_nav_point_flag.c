// hs_evaluate_activate_team_nav_point_flag  (not a Ghidra function; the evaluate handler of hs function 370 "activate_team_nav_point_flag" (navpoint, team, cutscene_flag, real -> void))
// address 0x480490, size 87 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480490, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480490..0x4804e7: hud_waypoint_activate_for_team(EAX = the flag (movsx), stack: the navpoint (movsx), the team word zero-extended,
//   0, the offset), returns 0.
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
extern void hud_waypoint_activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind,
    float vertical_offset); // 0x4af1b0, blam-cc: EAX, stack

void hs_evaluate_activate_team_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_waypoint_activate_for_team((datum_index)(int32_t)*(int16_t *)&arguments[2], *(int16_t *)&arguments[0],
        *(int16_t *)&arguments[1], 0, *(float *)&arguments[3]);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
