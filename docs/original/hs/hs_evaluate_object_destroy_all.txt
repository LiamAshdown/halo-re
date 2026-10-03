// hs_evaluate_object_destroy_all  (not a Ghidra function; the evaluate handler of hs function 44 "object_destroy_all" (no parameters -> void))
// address 0x47a7b0, size 16 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47a7b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47a7b0..0x47a7c0: hs_object_runtime_cleanup(), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void hs_object_runtime_cleanup(void); // 0x487dd0

void hs_evaluate_object_destroy_all(int16_t function_index, uint32_t thread_index, char first)
{
    hs_object_runtime_cleanup();
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
