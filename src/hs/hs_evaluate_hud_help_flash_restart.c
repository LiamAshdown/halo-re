// hs_evaluate_hud_help_flash_restart  (not a Ghidra function; the evaluate handler of hs function 367 "hud_help_flash_restart" (no parameters -> void))
// address 0x480380, size 41 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480380, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480380..0x4803a9: with HUD messaging (*0x006b3a40) +0x464 set, +0x460 = the game time (0x006f1d6c +0x0c); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "fn_hs.h"


extern uint8_t *hud_messaging; // 0x006b3a40
extern game_time_globals *game_time; // 0x006f1d6c

void hs_evaluate_hud_help_flash_restart(int16_t function_index, uint32_t thread_index, char first)
{
    if (hud_messaging[0x464]) {
        *(int32_t *)(hud_messaging + 0x460) = game_time->game_time;
    }
    hs_thread_return(0, thread_index);
}
