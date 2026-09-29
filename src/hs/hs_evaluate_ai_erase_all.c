// hs_evaluate_ai_erase_all  (not a Ghidra function; the evaluate handler of hs "ai_erase_all" (-> void))
// address 0x47d330, size 33 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d330, only reachable through that pointer.
//   Campaign track: 15 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d330: ai_release_actors_filtered(EAX -1, EDI -1, stack -1, BL 0) -- every actor; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"


void hs_evaluate_ai_erase_all(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    ai_release_actors_filtered(0xffffffff, -1, -1, 0);
    hs_thread_return(0, thread_index);
}
