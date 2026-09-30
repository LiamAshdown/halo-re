// hs_evaluate_game_safe_to_speak  (not a Ghidra function; the evaluate handler of hs function 309 "game_safe_to_speak" (no parameters -> boolean))
// address 0x47fa80, size 31 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fa80, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fa80..0x47fa9f: game_no_player_is_dead() into a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_game.h"


void hs_evaluate_game_safe_to_speak(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint8_t)game_no_player_is_dead(), thread_index);
}
