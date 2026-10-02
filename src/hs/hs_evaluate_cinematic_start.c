// hs_evaluate_cinematic_start  (not a Ghidra function; the evaluate handler of hs function 295 "cinematic_start" (no parameters -> void))
// address 0x47f7f0, size 16 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f7f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f7f0..0x47f800: cutscene_start(), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void cutscene_start(void); // 0x449720

void hs_evaluate_cinematic_start(int16_t function_index, uint32_t thread_index, char first)
{
    cutscene_start();
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
