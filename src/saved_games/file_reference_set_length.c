// file_reference_set_length  (Ghidra: FUN_005559b0, renamed)
// address 0x5559b0, size 98 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md summary "Seeks to a position and then
// truncates or extends the open file to that length." Confirmed against objdump
// 0x5559b0..0x5559da: `mov ecx,esi` feeds file_reference_seek's ECX=reference argument, while
// EAX (the offset) is never touched before that call, so it passes straight through from this
// function's own caller.
// register convention: offset in EAX, reference record in ESI (EAX forwarded unchanged into
// file_reference_seek's own EAX=offset argument; ESI copied to ECX for its reference argument).
// FIXED (register inputs, objdump): notes wording ("reference record in ESI") didn't match the
// checker's alias for file_reference_record*, so ESI (read at 0x5559b6) looked unclaimed. Body
// already used ref correctly; reworded the blam-cc line to the standard "REG -> name" form.

#include "win32.h"
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
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // 0x5558f0, this module
extern void saved_games_report_last_error(void); // 0x556170, this module

// blam-cc: EAX -> offset, ESI -> ref
// Seeks ref's open handle to offset, then truncates or extends the file to that length.
// Returns 1 on success, 0 on failure (after reporting the Win32 error).
uint8_t file_reference_set_length(int32_t offset, file_reference_record *ref)
{
    uint8_t seeked;
    int32_t ok;

    seeked = file_reference_seek(offset, ref);
    if (seeked != 0) {
        ok = SetEndOfFile(ref->handle);
        if (ok != 0) {
            return 1;
        }
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x5559b0):

undefined4 FUN_005559b0(void)

{
  char cVar1;
  BOOL BVar2;
  DWORD dwMessageId;
  int unaff_ESI;
  CHAR local_800 [2048];

  cVar1 = file_reference_seek();
  if (cVar1 != '\0') {
    BVar2 = SetEndOfFile(*(HANDLE *)(unaff_ESI + 0x108));
    if (BVar2 != 0) {
      return 1;
    }
  }
  dwMessageId = GetLastError();
  FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_800,0x800,(va_list *)0x0);
  SetLastError(0);
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
