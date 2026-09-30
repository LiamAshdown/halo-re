// hs_evaluate_activate_nav_point_flag  (not a Ghidra function; the evaluate handler of hs function "activate_nav_point_flag" (navpoint, unit, cutscene_flag, real -> void);
//   no C existed, so a script call would hit the unlisted trap)
// address 0x4803b0, size 112 bytes
// name confidence: 0.9  rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4803b0..0x48041f: the unit's player, when there is one, gets hud_waypoint_activate_for_player(EAX player,
//   EBX the flag (movsx), DX 0, stack: the navpoint word, the offset); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_interface.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack


void hs_evaluate_activate_nav_point_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        datum_index player = player_index_from_unit_index((datum_index)arguments[1]);

        if (player != k_datum_index_none) {
            hud_waypoint_activate_for_player(player, (datum_index)(int32_t)*(int16_t *)&arguments[2], 0,
                *(int16_t *)&arguments[0], *(float *)&arguments[3]);
        }
        hs_thread_return(0, thread_index);
    }
}
