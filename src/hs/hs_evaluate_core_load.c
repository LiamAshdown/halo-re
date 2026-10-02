// hs_evaluate_core_load  (not a Ghidra function; the evaluate handler of hs function 320 "core_load" (no parameters -> void))
// address 0x4828f0, size 18 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4828f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4828f0..0x482902: main globals byte 0x00719752 = 1, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t main_globals_byte_00719752; // 0x00719752

void hs_evaluate_core_load(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_byte_00719752 = 1;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
