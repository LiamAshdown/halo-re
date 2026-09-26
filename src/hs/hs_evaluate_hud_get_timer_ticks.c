// hs_evaluate_hud_get_timer_ticks  (not a Ghidra function; the evaluate handler of hs function 411 "hud_get_timer_ticks" (no parameters -> short))
// address 0x480f20, size 126 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480f20, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480f20..0x480f9e: with the timer shown (HUD messaging *0x006b3a40 +0x487): -1 when unset (+0x47c == -1), the stored ticks while
//   paused (+0x486), else stored + start (+0x478) - game time (0x006f1d6c +0x0c), all word arithmetic; 0 when
//   hidden; a word in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t *hud_messaging; // 0x006b3a40
extern uint8_t *game_time; // 0x006f1d6c

void hs_evaluate_hud_get_timer_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    uint16_t ticks = 0;

    if (hud_messaging[0x487]) {
        uint16_t stored = *(uint16_t *)(hud_messaging + 0x47c);

        if (stored == 0xffff) {
            ticks = 0xffff;
        } else if (hud_messaging[0x486]) {
            ticks = stored;
        } else {
            ticks = (uint16_t)(*(uint16_t *)(hud_messaging + 0x478) - *(uint16_t *)(game_time + 0xc) + stored);
        }
    }
    hs_thread_return((int32_t)ticks, thread_index);
}
