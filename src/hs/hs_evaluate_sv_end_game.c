// hs_evaluate_sv_end_game  (not a Ghidra function; the evaluate handler of hs function 484 "sv_end_game" ( -> void))
// address 0x482bc0, size 58 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482bc0 trapped.
// WRITTEN 2026-09-28 from objdump 0x482bc0..0x482bf9: as the server (network_game_mode 2): sets 0x006f1d25, begins
//   the end game sequence and prints "Server is stopping the game..."; otherwise prints "sv_end_game is a server-only
//   function!"; both in the console colour at 0x006851fc. Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void chimera__console_out(void *color, char *format, ...); // 0x496b50, blam-cc: EAX color
extern int16_t network_game_mode; // 0x00719720
extern uint8_t g_006f1d25; // 0x006f1d25
extern void game_engine_begin_end_game_sequence(void); // 0x45fd90
extern void *console_color_006851fc; // 0x006851fc

void hs_evaluate_sv_end_game(int16_t function_index, uint32_t thread_index, char first)
{
    if (network_game_mode == 2) {
        g_006f1d25 = 1;
        game_engine_begin_end_game_sequence();
        chimera__console_out(console_color_006851fc, "Server is stopping the game..."); // 0x0066dbd4
    } else {
        chimera__console_out(console_color_006851fc, "sv_end_game is a server-only function!"); // 0x0066dbac
    }
    hs_thread_return(0, thread_index);
}
