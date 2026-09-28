// player_update_history_log_write  (Ghidra: player_update_history_log_write, already named)
// address 0x4e5ea0, size 126 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; literal string "ClientPlayerUpdateHistory.log".
// register convention: disassembly (objdump -d -M intel) shows the real parameter list --
// Ghidra only recovered the stack-based format string as `char *param_1`, missing the two
// register-passed gate values and misreporting the varargs as `&stack0x00000008`:
//   4e5ea6: test ecx,ecx / 4e5eaa,4e5eb6: mov [DAT_00710314 or DAT_00710318]  ; ECX selects which
//           category mask to gate against
//   entry EAX is ANDed against that mask (the "in_EAX" of Ghidra's decompile)
//   4e5ec2: mov ecx,[esp+0x404]      ; the format string pointer -- this is stack argument 1,
//           landing 4 bytes further out than Ghidra's own `param_1` offset once the callee's
//           `sub esp,0x400` and the return address are both accounted for
//   4e5ec9: lea eax,[esp+0x408]      ; &second stack argument = the va_list Ghidra printed as
//           `&stack0x00000008`
// register convention: EAX -> category_flags, ECX -> use_filtered_mask, stack -> format, ...
//   // blam-cc: EAX -> category_flags, ECX -> use_filtered_mask, stack -> format, ...
// UNSURE: DAT_00710318 vs DAT_00710314's exact meaning; the selector is 0 from every call site
// discovered in this batch except player_update_history_log_printf_filtered.c's, which passes
// non-zero, so this rewrite names them "default" and "filtered" respectively, matching that one
// caller. UNSURE: the fopen mode 0x0066b87c is a single ASCII 'a' (confirmed by reading
// bin/halo.exe's .rdata), i.e. C's "a" (append, no explicit text/binary suffix).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>

extern uint32_t player_update_log_categories_default;  // 0x00710314
extern uint32_t player_update_log_categories_filtered;  // 0x00710318
extern uint8_t player_update_log_flags;                 // 0x00710310, bit1 = write to the log file
extern char *player_update_history_log_path;            // 0x006997f8, "ClientPlayerUpdateHistory.log"
extern char player_update_log_file_mode_string[];        // 0x0066b87c, "a"

// fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x624186, fopen-shaped CRT wrapper

// Formats `format` (with the trailing varargs) into a scratch buffer if category_flags is fully
// covered by the selected category mask, then appends it to ClientPlayerUpdateHistory.log when
// file logging is enabled (player_update_log_flags bit1).
void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask, const char *format, ...)
    // blam-cc: EAX -> category_flags, ECX -> use_filtered_mask, stack -> format, ...
{
    uint32_t mask;
    char buffer[0x400];
    va_list args;
    FILE *file;

    mask = use_filtered_mask != 0 ? player_update_log_categories_filtered : player_update_log_categories_default;
    if ((mask & category_flags) != category_flags) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    if ((player_update_log_flags & 2) == 0) {
        return;
    }
    file = (FILE *)fopen(player_update_history_log_path, player_update_log_file_mode_string);
    if (file == 0) {
        return;
    }
    fprintf(file, buffer);
    fclose(file);
}

#if 0
Original Ghidra decompilation (0x4e5ea0), from tools/pack.py 0x4e5ea0:

void player_update_history_log_write(char *param_1)

{
  uint uVar1;
  uint in_EAX;
  FILE *_File;
  int in_ECX;
  char local_400 [1024];

  uVar1 = DAT_00710318;
  if (in_ECX == 0) {
    uVar1 = DAT_00710314;
  }
  if ((((uVar1 & in_EAX) == in_EAX) &&
      (_vsprintf(local_400,param_1,&stack0x00000008), (DAT_00710310 & 2) != 0)) &&
     (_File = (FILE *)FUN_00624186(PTR_s_ClientPlayerUpdateHistory_log_006997f8,&DAT_0066b87c),
     _File != (FILE *)0x0)) {
    _fprintf(_File,local_400);
    _fclose(_File);
  }
  return;
}
#endif
