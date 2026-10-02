// hs_evaluate_player_effect_stop  (not a Ghidra function; the evaluate handler of hs function 395 "player_effect_stop" (real -> void))
// address 0x480970, size 111 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480970, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480970..0x4809df: player effect globals (*0x006f1884): the value times 30.0 (0x672ac8), stored as a float and rounded by fistp,
//   goes to the words +0x11c and +0x11e; +0x120 |= 2; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern player_effect_globals *player_effect_globals_pointer;
extern long lrint(double x);

void hs_evaluate_player_effect_stop(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float scaled = *(float *)&arguments[0] * 30.0f;
    int16_t ticks = (int16_t)lrint((double)scaled);

    player_effect_globals_pointer->scripted_shake_ticks = ticks;
    player_effect_globals_pointer->scripted_shake_duration = ticks;
    player_effect_globals_pointer->scripted_shake_flags |= 2;
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
