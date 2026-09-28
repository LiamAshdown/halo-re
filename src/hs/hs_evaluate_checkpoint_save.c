// hs_evaluate_checkpoint_save  (not a Ghidra function; the evaluate handler of hs function 510 "checkpoint_save" ( -> void))
// address 0x482690, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482690 trapped.
// WRITTEN 2026-09-28 from objdump 0x482690..0x48269f: calls game_checkpoint_save_new; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t game_checkpoint_save_new(void); // 0x538db0

void hs_evaluate_checkpoint_save(int16_t function_index, uint32_t thread_index, char first)
{
    game_checkpoint_save_new();
    hs_thread_return(0, thread_index);
}
