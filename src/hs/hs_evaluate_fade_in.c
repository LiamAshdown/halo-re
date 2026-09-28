// hs_evaluate_fade_in  (not a Ghidra function; the evaluate handler of hs function 293 "fade_in" (real, real, real, short -> void))
// address 0x47f6f0, size 120 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f6f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f6f0..0x47f768: the screen fade globals (*0x006f1884): +0xec/+0xf0/+0xf4 = the colour, +0xfc = the tick word, +0xfe = 0 (in),
//   +0xf8 = the game time (0x006f1d6c +0x0c); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *cinematic_fade_globals; // 0x006f1884
extern game_time_globals *game_time; // 0x006f1d6c

void hs_evaluate_fade_in(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    *(int32_t *)(cinematic_fade_globals + 0xf0) = arguments[1];
    *(int16_t *)(cinematic_fade_globals + 0xfc) = *(int16_t *)&arguments[3];
    *(int32_t *)(cinematic_fade_globals + 0xf4) = arguments[2];
    *(int32_t *)(cinematic_fade_globals + 0xec) = arguments[0];
    cinematic_fade_globals[0xfe] = 0;
    *(int32_t *)(cinematic_fade_globals + 0xf8) = game_time->game_time;
    hs_thread_return(0, thread_index);
    }
}
