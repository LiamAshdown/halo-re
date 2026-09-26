// hs_evaluate_cinematic_abort  (not a Ghidra function; the evaluate handler of hs function 297 "cinematic_abort" (no parameters -> void))
// address 0x47f810, size 34 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f810, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f810..0x47f832: main globals: word 0x00719754 = -1, byte 0x0071973c = 0, byte 0x0071973b = 1; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint16_t main_globals_word_00719754; // 0x00719754
extern uint8_t main_globals_byte_0071973c; // 0x0071973c
extern uint8_t main_globals_byte_0071973b; // 0x0071973b

void hs_evaluate_cinematic_abort(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_word_00719754 = 0xffff;
    main_globals_byte_0071973c = 0;
    main_globals_byte_0071973b = 1;
    hs_thread_return(0, thread_index);
}
