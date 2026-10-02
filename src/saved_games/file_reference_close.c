// file_reference_close  (Ghidra: file_reference_close, already named)
// address 0x555890, size 90 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA. Register convention from
// out/phase4/saved_games_types_notes.md: "_open / _close / _delete ESI reference".
// register convention: reference record (ref) in ESI.
// FIXED (register inputs, objdump): ESI carries ref (read at 0x555890, mov eax,[esi+0x108]);
// the note named the param "reference record" instead of "ref", so it did not parse.

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

extern void saved_games_report_last_error(void); // 0x556170, this module

// blam-cc: ref in ESI
// Closes ref's open handle and clears it. Returns 1 on success, 0 on failure (after reporting
// the Win32 error, leaving the stale handle in place).
uint8_t file_reference_close(file_reference_record *ref)
{
    int32_t ok;

    ok = CloseHandle(ref->handle);
    if (ok != 0) {
        ref->handle = 0;
        return 1;
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x555890):

undefined4 file_reference_close(void)

{
  BOOL BVar1;
  DWORD dwMessageId;
  int unaff_ESI;
  CHAR local_800 [2048];

  BVar1 = CloseHandle(*(HANDLE *)(unaff_ESI + 0x108));
  if (BVar1 != 0) {
    *(undefined4 *)(unaff_ESI + 0x108) = 0;
    return 1;
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
