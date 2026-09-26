// hs_evaluate_player_effect_start  (not a Ghidra function; the evaluate handler of hs function 394 "player_effect_start" (real, real -> void))
// address 0x4808e0, size 131 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4808e0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4808e0..0x480963: player effect globals (*0x006f1884): +0x118 = the first value; the second times 30.0 (0x672ac8), stored as a
//   float and rounded by fistp (round-half-to-even), goes to the words +0x11c and +0x11e; +0x120 = (& ~2) | 1;
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
extern uint8_t *player_effect_globals_pointer; // 0x006f1884
extern long lrint(double x);

void hs_evaluate_player_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float scaled = *(float *)&arguments[1] * 30.0f;
    int16_t ticks = (int16_t)lrint((double)scaled);

    *(int32_t *)(player_effect_globals_pointer + 0x118) = arguments[0];
    *(int16_t *)(player_effect_globals_pointer + 0x11c) = ticks;
    *(int16_t *)(player_effect_globals_pointer + 0x11e) = ticks;
    *(uint32_t *)(player_effect_globals_pointer + 0x120) = (*(uint32_t *)(player_effect_globals_pointer + 0x120) & 0xfffffffd) | 1;
    hs_thread_return(0, thread_index);
    }
}
