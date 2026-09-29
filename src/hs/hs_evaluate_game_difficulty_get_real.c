// hs_evaluate_game_difficulty_get_real  (not a Ghidra function; the evaluate handler of hs function 259 "game_difficulty_get_real" (no parameters -> game_difficulty))
// address 0x47f0d0, size 36 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f0d0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f0d0..0x47f0f4: the difficulty word (+0x0e of the game options at *0x006b0b80) in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "fn_hs.h"


extern game_main_globals *main_game_globals; // 0x006b0b80

void hs_evaluate_game_difficulty_get_real(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint16_t)main_game_globals->difficulty, thread_index);
}
