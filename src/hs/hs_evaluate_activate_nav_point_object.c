// hs_evaluate_activate_nav_point_object  (not a Ghidra function; the evaluate handler of hs function 369 "activate_nav_point_object" (navpoint, unit, object, real -> void))
// address 0x480420, size 109 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480420, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480420..0x48048d: the unit's player (player_index_from_unit_index, stack unit), when there is one, gets
//   hud_waypoint_activate_for_player(EAX player, EBX object, DX 1, stack: the navpoint word, the offset); returns 0.
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
extern void hud_waypoint_activate_for_player(datum_index player_index, datum_index target, int16_t kind,
    int16_t arrow_index, float vertical_offset); // 0x4af0d0, blam-cc: EAX, EBX, DX, stack

void hs_evaluate_activate_nav_point_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index player = player_index_from_unit_index((datum_index)arguments[1]);

    if (player != k_datum_index_none) {
        hud_waypoint_activate_for_player(player, (datum_index)arguments[2], 1, *(int16_t *)&arguments[0],
            *(float *)&arguments[3]);
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
