// hs_evaluate_core_load_at_startup  (not a Ghidra function; the evaluate handler of hs function 321 "core_load_at_startup" (no parameters -> void))
// address 0x47fc00, size 18 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fc00, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fc00..0x47fc12: main globals byte 0x00719753 = 1, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern uint8_t main_globals_byte_00719753; // 0x00719753

void hs_evaluate_core_load_at_startup(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_byte_00719753 = 1;
    hs_thread_return(0, thread_index);
}
