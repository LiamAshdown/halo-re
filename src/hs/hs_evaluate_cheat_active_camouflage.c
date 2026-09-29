// hs_evaluate_cheat_active_camouflage  (not a Ghidra function; the evaluate procedure of hs function "cheat_active_camouflage"; no C existed, so running it
//   from the console trapped as unlisted_47ced0)
// address 0x47ced0, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x47ced0: calls 0x45a6c0 (named cheat_make_selected_object_invincible in its C file). Returns 0 through hs_thread_return (EAX 0, ECX thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern void cheat_make_selected_object_invincible(void); // 0x45a6c0

void hs_evaluate_cheat_active_camouflage(int16_t function_index, uint32_t thread_index, char first)
{
    cheat_make_selected_object_invincible();
    hs_thread_return(0, thread_index);
}
