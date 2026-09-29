// hs_evaluate_cinematic_stop  (not a Ghidra function; the evaluate handler of hs function 296 "cinematic_stop" (no parameters -> void))
// address 0x47f800, size 16 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f800, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f800..0x47f810: cutscene_stop(), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void cutscene_stop(void); // 0x449eb0

void hs_evaluate_cinematic_stop(int16_t function_index, uint32_t thread_index, char first)
{
    cutscene_stop();
    hs_thread_return(0, thread_index);
}
