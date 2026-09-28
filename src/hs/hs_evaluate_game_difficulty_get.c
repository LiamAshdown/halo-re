// hs_evaluate_game_difficulty_get  (not a Ghidra function; the evaluate handler of hs function 258 "game_difficulty_get" (no parameters -> game_difficulty))
// address 0x47f0a0, size 48 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f0a0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f0a0..0x47f0d0: the difficulty word (+0x0e of the game options at *0x006b0b80), raised to 1 when below, in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern game_main_globals *main_game_globals; // 0x006b0b80

void hs_evaluate_game_difficulty_get(int16_t function_index, uint32_t thread_index, char first)
{
    int16_t difficulty = main_game_globals->difficulty;

    if (difficulty <= 1) {
        difficulty = 1;
    }
    hs_thread_return((int32_t)(uint16_t)difficulty, thread_index);
}
