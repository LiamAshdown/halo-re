// hs_evaluate_cheat_all_weapons  (not a Ghidra function; the evaluate procedure of hs function "cheat_all_weapons"; no C existed, so running it
//   from the console trapped as unlisted_4828a0)
// address 0x4828a0, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4828a0: calls cheat_all_weapons (0x45a530). Returns 0 through hs_thread_return (EAX 0, ECX thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, EAX value, ECX thread

extern void cheat_all_weapons(void); // 0x45a530

void hs_evaluate_cheat_all_weapons(int16_t function_index, uint32_t thread_index, char first)
{
    cheat_all_weapons();
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
