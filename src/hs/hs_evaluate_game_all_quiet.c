// hs_evaluate_game_all_quiet  (not a Ghidra function; the evaluate handler of hs function 308 "game_all_quiet" (no parameters -> boolean))
// address 0x47fa60, size 31 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fa60, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fa60..0x47fa7f: game_safe_to_pause() into a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint32_t game_safe_to_pause(void); // 0x45b9e0

void hs_evaluate_game_all_quiet(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint8_t)game_safe_to_pause(), thread_index);
}
