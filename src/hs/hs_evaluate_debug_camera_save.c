// hs_evaluate_debug_camera_save  (not a Ghidra function; the evaluate handler of hs function 254 "debug_camera_save" ( -> void))
// address 0x47f020, size 16 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f020 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f020..0x47f02f: calls camera_debug_save_to_file; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void camera_debug_save_to_file(void); // 0x445880

void hs_evaluate_debug_camera_save(int16_t function_index, uint32_t thread_index, char first)
{
    camera_debug_save_to_file();
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
