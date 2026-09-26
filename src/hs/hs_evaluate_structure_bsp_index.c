// hs_evaluate_structure_bsp_index  (not a Ghidra function; the evaluate handler of hs function 268 "structure_bsp_index" (no parameters -> short))
// address 0x47f670, size 33 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f670, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f670..0x47f691: returns global_structure_bsp_index (0x0069e8d8) as a word in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern int16_t global_structure_bsp_index; // 0x0069e8d8

void hs_evaluate_structure_bsp_index(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint16_t)global_structure_bsp_index, thread_index);
}
