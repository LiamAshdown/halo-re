// hs_evaluate_core_save  (not a Ghidra function; the evaluate handler of hs function 318 "core_save" (no parameters -> void))
// address 0x482910, size 18 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x482910, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x482910..0x482922: main globals byte 0x00719751 = 1, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t main_globals_byte_00719751; // 0x00719751

void hs_evaluate_core_save(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_byte_00719751 = 1;
    hs_thread_return(0, thread_index);
}
