// hs_evaluate_players_unzoom_all  (not a Ghidra function; the evaluate handler of hs function 346 "players_unzoom_all" ( -> void))
// address 0x47f100, size 22 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47f100 trapped.
// WRITTEN 2026-09-28 from objdump 0x47f100..0x47f115: player control globals (0x006b145c) word +0x34 = -1 (zoom
//   level); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t *player_control_globals_ptr; // 0x006b145c

void hs_evaluate_players_unzoom_all(int16_t function_index, uint32_t thread_index, char first)
{
    *(int16_t *)(player_control_globals_ptr + 0x34) = -1;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
