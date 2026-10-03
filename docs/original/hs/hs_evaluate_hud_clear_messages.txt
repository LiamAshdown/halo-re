// hs_evaluate_hud_clear_messages  (not a Ghidra function; the evaluate handler of hs function 403 "hud_clear_messages" (no parameters -> void))
// address 0x480c80, size 38 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480c80, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480c80..0x480ca6: clears byte +0x82 of the four 0x8c-byte message slots of HUD messaging (*0x006b3a40); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *hud_messaging; // 0x006b3a40

void hs_evaluate_hud_clear_messages(int16_t function_index, uint32_t thread_index, char first)
{
    int32_t slot;

    for (slot = 0; slot < 4; slot++) {
        hud_messaging[0x82 + slot * 0x8c] = 0;
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
