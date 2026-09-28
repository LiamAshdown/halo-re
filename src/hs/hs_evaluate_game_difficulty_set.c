// hs_evaluate_game_difficulty_set  (not a Ghidra function; the evaluate handler of hs function 265 "game_difficulty_set" (game_difficulty -> void))
// address 0x47f580, size 76 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f580 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f580..0x47f5cb: a difficulty 0..3 becomes the pending difficulty
//   (0x00696564); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern int16_t pending_difficulty; // 0x00696564

void hs_evaluate_game_difficulty_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t difficulty = (int16_t)arguments[0];

        if (difficulty >= 0 && difficulty < 4) {
            pending_difficulty = difficulty;
        }
        hs_thread_return(0, thread_index);
    }
}
