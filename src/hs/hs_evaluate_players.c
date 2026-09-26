// hs_evaluate_players  (not a Ghidra function; the evaluate handler of hs function 29 "players" (no parameters -> object_list))
// address 0x47a410, size 14 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a410, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a410..0x47a41e: returns hs_object_list_collect_player_units().
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern datum_index hs_object_list_collect_player_units(void); // 0x487630

void hs_evaluate_players(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)hs_object_list_collect_player_units(), thread_index);
}
