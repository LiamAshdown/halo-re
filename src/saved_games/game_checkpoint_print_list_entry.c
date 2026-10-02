// game_checkpoint_print_list_entry  (Ghidra: not exported as a standalone function, named)
// address 0x539110, size 143 bytes (0x539110..0x53919e)
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_types_notes.md "In-range functions missing from the
// 116-function list": "0x539110 (checkpoint list print callback, ticks to h:m:s, level name
// from 0x00696574)"; matches checkpoint_enumerate_proc. Not present in out/functions.json, so
// rewritten directly from objdump: the three "multiply by a magic constant, shift, add the sign
// bit" sequences are MSVC's standard division-by-constant idiom for /108000 (ticks per hour at
// 30 ticks/s), /1800 (ticks per minute) and /30 (ticks per second) on a non-negative dividend,
// so they are written here as plain integer division -- identical results, no behaviour change.
// objdump's rdata dump confirms the format "%-15s %-20s %02d:%02d:%02d" at 0x6709cc and the
// level-name table read (0x00696574 + level_index*4, guarded to level_index in 0..9, matching
// k_campaign_level_count) that types_notes.md's evidence table also cites for 0x538c60's
// reader. chimera__console_out's (EAX color, stack format+args) convention from
// src/interface/chimera__console_out.c; 0x00686af8 is passed as the color, UNSURE of its exact
// identity (likely a fixed "info" ColorARGB constant).
// Phase 4 review: matched objdump 0x539110..0x53919e; 0x00686af8 holds a ColorARGB pointer (mov
// eax,ds:0x686af8).
// register convention: __cdecl, all seven parameters on the stack (Ghidra's own
// checkpoint_enumerate_proc order); only name, level_index and game_time_ticks are used.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char *campaign_level_paths[k_campaign_level_count]; // 0x00696574, UNSURE: element type/exact table length
extern ColorARGB *actor_mode_default_look_weights; // 0x00686af8, UNSURE: console color passed to chimera__console_out

extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50

uint8_t game_checkpoint_print_list_entry(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data)
{
    int32_t hours;
    int32_t minutes;
    int32_t seconds;
    int32_t remainder;
    char *level_name;

    hours = game_time_ticks / 108000;
    remainder = game_time_ticks - hours * 108000;
    minutes = remainder / 1800;
    remainder = remainder - minutes * 1800;
    seconds = remainder / 30;

    level_name = 0;
    if (0 <= level_index && level_index < k_campaign_level_count) {
        level_name = campaign_level_paths[level_index];
    }

    chimera__console_out(actor_mode_default_look_weights, (char *)"%-15s %-20s %02d:%02d:%02d", name, level_name, hours, minutes, seconds);
    return 1;
}

#if 0
Original disassembly (0x539110, no Ghidra export -- not in out/functions.json):

00539110:
  8b 4c 24 14          mov    ecx,[esp+0x14]      ; game_time_ticks
  b8 39 37 58 9b       mov    eax,0x9b583739
  f7 e9                imul   ecx
  03 d1                add    edx,ecx
  c1 fa 10             sar    edx,0x10
  56                   push   esi
  8b f2                mov    esi,edx
  c1 ee 1f             shr    esi,0x1f
  03 f2                add    esi,edx                ; esi = game_time_ticks / 108000 (hours)
  8b c6                mov    eax,esi
  69 c0 20 5a fe ff    imul   eax,eax,0xfffe5a20
  03 c8                add    ecx,eax                ; ecx = remainder after removing whole hours
  b8 c5 b3 a2 91       mov    eax,0x91a2b3c5
  f7 e9                imul   ecx
  03 d1                add    edx,ecx
  c1 fa 0a             sar    edx,0xa
  57                   push   edi
  8b fa                mov    edi,edx
  c1 ef 1f             shr    edi,0x1f
  03 fa                add    edi,edx                ; edi = remainder / 1800 (minutes)
  8b d7                mov    edx,edi
  69 d2 08 07 00 00    imul   edx,edx,0x708
  2b ca                sub    ecx,edx                ; ecx = remainder after removing whole minutes
  b8 89 88 88 88       mov    eax,0x88888889
  f7 e9                imul   ecx
  03 d1                add    edx,ecx
  c1 fa 04             sar    edx,0x4
  8b c2                mov    eax,edx
  c1 e8 1f             shr    eax,0x1f
  03 c2                add    eax,edx                ; eax = remainder / 30 (seconds)
  66 8b 54 24 14       mov    dx,[esp+0x14]           ; level_index (word)
  33 c9                xor    ecx,ecx
  66 85 d2             test   dx,dx
  7c 10                jl     0x53917f
  66 83 fa 0a          cmp    dx,0xa
  73 0a                jae    0x53917f
  0f bf ca             movsx  ecx,dx
  8b 0c 8d 74 65 69 00 mov    ecx,[0x696574+ecx*4]
0053917f:
  8b 54 24 10          mov    edx,[esp+0x10]           ; name
  50                   push   eax
  a1 f8 6a 68 00       mov    eax,ds:0x686af8
  57                   push   edi
  56                   push   esi
  51                   push   ecx
  52                   push   edx
  68 cc 09 67 00       push   0x6709cc                  ; "%-15s %-20s %02d:%02d:%02d"
  e8 b9 d9 f5 ff       call   0x496b50                   ; chimera__console_out
  83 c4 18             add    esp,0x18
  5f                   pop    edi
  b0 01                mov    al,0x1
  5e                   pop    esi
  c3                   ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
