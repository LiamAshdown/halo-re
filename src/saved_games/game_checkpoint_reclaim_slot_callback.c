// game_checkpoint_reclaim_slot_callback  (Ghidra: not exported as a standalone function, named)
// address 0x538ac0, size 30 bytes (0x538ac0..0x538adf)
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_types_notes.md "In-range functions missing from the
// 116-function list": "0x538ac0 (checkpoint reclaim callback, copies the first name into
// user_data)"; matches the checkpoint_enumerate_proc typedef (types/saved_games.h). Not present
// in out/functions.json, so rewritten directly from objdump: all seven parameters are plain
// stack arguments at [esp+4]..[esp+0x1c] (index, name, level_index, difficulty, game_time,
// time, user_data), and objdump's 0x66a4dc string dump confirms the format "checkpoints\%s".
// Passed as the callback to game_checkpoint_enumerate_files when reclaiming a checkpoint slot
// (game_checkpoint_get_next_filename): the first entry visited stores its relative path into
// *user_data; every entry after that is left alone.
// register convention: __cdecl, all seven parameters on the stack (Ghidra's own
// checkpoint_enumerate_proc order).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t _sprintf(char *dest, const char *format, ...); // 0x623693

uint8_t game_checkpoint_reclaim_slot_callback(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data)
{
    char *out_name = (char *)user_data;

    if (out_name[0] == 0) {
        _sprintf(out_name, "checkpoints\\%s", name);
    }
    return 1;
}

#if 0
Original disassembly (0x538ac0, no Ghidra export -- not in out/functions.json):

00538ac0:
  8b 44 24 1c          mov    eax,[esp+0x1c]     ; user_data
  80 38 00             cmp    byte ptr [eax],0x0
  75 13                jne    0x538adc
  8b 4c 24 08          mov    ecx,[esp+0x8]       ; name
  51                   push   ecx
  68 dc a4 66 00       push   0x66a4dc            ; "checkpoints\%s"
  50                   push   eax
  e8 ba ab 0e 00       call   0x623693            ; _sprintf
  83 c4 0c             add    esp,0xc
00538adc:
  b0 01                mov    al,0x1
  c3                   ret
#endif
