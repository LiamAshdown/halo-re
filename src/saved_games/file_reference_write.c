// file_reference_write  (Ghidra: file_reference_write, already named)
// address 0x555a90, size 103 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA. Register convention from out/phase4/
// saved_games_types_notes.md: "file_reference_read / _write EDX reference, ECX buffer, ESI size".
// register convention: reference record in EDX, buffer in ECX, size in ESI.
//   // blam-cc: EDX -> ref, ECX -> buffer, ESI -> size
// FIXED (register inputs, objdump): EDX (the reference record, already a C parameter) was
//   read at 0x555a9e but missing from the machine-checked "blam-cc" annotation; reworded so
//   the checker recognizes it.

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

// blam-cc: EDX -> ref, ECX -> buffer, ESI -> size
// Writes exactly size bytes from buffer to ref's open handle. Returns 1 on success, 0 on
// failure (after reporting the Win32 error).
uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size)
{
    int32_t ok;
    uint32_t bytes_written;

    ok = WriteFile(ref->handle, buffer, size, (LPDWORD)&bytes_written, 0);
    if (ok != 0 && bytes_written == size) {
        return 1;
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x555a90):

undefined4 file_reference_write(void)

{
  BOOL BVar1;
  DWORD dwMessageId;
  LPCVOID in_ECX;
  int in_EDX;
  DWORD unaff_ESI;
  DWORD local_804;
  CHAR local_800 [2048];

  BVar1 = WriteFile(*(HANDLE *)(in_EDX + 0x108),in_ECX,unaff_ESI,&local_804,(LPOVERLAPPED)0x0);
  if ((BVar1 != 0) && (local_804 == unaff_ESI)) {
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
