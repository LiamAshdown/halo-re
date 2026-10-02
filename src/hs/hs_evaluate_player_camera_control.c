// hs_evaluate_player_camera_control  (not a Ghidra function; the evaluate handler of hs function 348 "player_camera_control" (boolean -> boolean))
// address 0x47f170, size 121 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f170, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f170..0x47f1e9: clears (true) or sets (false) bit 0 of player control globals (0x006b145c) +0x0c; returns the boolean in a
//   zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern player_control_globals *player_control_globals_ptr;

void hs_evaluate_player_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t enable = *(uint8_t *)&arguments[0];

    if (enable) {
        player_control_globals_ptr->flags &= 0xfffffffe;
    } else {
        player_control_globals_ptr->flags |= 1;
    }
    hs_thread_return((int32_t)enable, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
