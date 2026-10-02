// hs_evaluate_sound_eax_enabled  (not a Ghidra function; the evaluate handler of hs function 438 "sound_eax_enabled" (no parameters -> boolean))
// address 0x4815b0, size 77 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x4815b0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x4815b0..0x4815fd: 1 when the sound effect object (*0x00721f24) exists and its mode (+0x04) is 0, 1 or 2, in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *global_sound_effect_object; // 0x00721f24

void hs_evaluate_sound_eax_enabled(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t enabled = 0;

    if (global_sound_effect_object != 0) {
        int32_t mode = *(int32_t *)(global_sound_effect_object + 4);

        enabled = (uint8_t)(mode == 0 || mode == 1 || mode == 2);
    }
    hs_thread_return((int32_t)enabled, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
