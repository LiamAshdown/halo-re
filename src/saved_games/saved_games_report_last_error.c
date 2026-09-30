// saved_games_report_last_error  (Ghidra: FUN_00556170, renamed)
// address 0x556170, size 55 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md summary "Formats and discards the current
// Win32 last-error code into a scratch buffer, matching the error-reporting pattern used
// throughout this module's file helpers."; out/phase4/saved_games_types_notes.md confirms
// "0x556170 is Blam code (FormatMessageA error reporter), not CRT." Every file_reference_*
// helper in this module ends its failure path with exactly this GetLastError / FormatMessageA
// / SetLastError(0) sequence; this is the shared tail some of them call directly instead of
// repeating inline.
// register convention: no arguments; reads/clears the calling thread's Win32 last-error code.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


// blam-cc: no arguments
// Formats the current Win32 last-error code into a discarded 0x800-byte scratch buffer (the
// message text is never read back -- only the FormatMessageA call itself matters, presumably
// for its side effect of validating/consuming the error), then clears the last-error code.
void saved_games_report_last_error(void)
{
    uint32_t message_id;
    char scratch[0x800];

    message_id = GetLastError();
    FormatMessageA(0x12ff, 0, message_id, 0, (LPSTR)scratch, 0x800, 0);
    SetLastError(0);
    return;
}

#if 0
Original Ghidra decompilation (0x556170):

void FUN_00556170(void)

{
  DWORD dwMessageId;
  CHAR local_800 [2048];

  dwMessageId = GetLastError();
  FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_800,0x800,(va_list *)0x0);
  SetLastError(0);
  return;
}
#endif
