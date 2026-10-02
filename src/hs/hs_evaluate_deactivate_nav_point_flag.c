// hs_evaluate_deactivate_nav_point_flag  (not a Ghidra function; the evaluate handler of hs function "deactivate_nav_point_flag" (unit, cutscene_flag -> void);
//   no C existed, so a script call would hit the unlisted trap)
// address 0x480550, size 90 bytes
// name confidence: 0.9  rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x480550..0x4805a9: the unit's player, when there is one, gets hud_waypoint_deactivate_for_player(EAX player,
//   EDI the flag (movsx), SI 0); returns 0.
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
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack
extern void hud_waypoint_deactivate_for_player(datum_index player_index, datum_index target, int16_t kind);
    // 0x4af230, blam-cc: EAX player, EDI target, SI kind

void hs_evaluate_deactivate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        datum_index player = player_index_from_unit_index((datum_index)arguments[0]);

        if (player != k_datum_index_none) {
            hud_waypoint_deactivate_for_player(player, (datum_index)(int32_t)*(int16_t *)&arguments[1], 0);
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
