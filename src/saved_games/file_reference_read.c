// file_reference_read  (Ghidra: file_reference_read, already named)
// address 0x555a20, size 112 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA. Register convention from out/phase4/
// saved_games_types_notes.md: "file_reference_read / _write EDX reference, ECX buffer, ESI size".
// register convention: reference record (ref) in EDX, buffer in ECX, size in ESI.
// FIXED (register inputs, objdump): EDX carries ref (read at 0x555a2f, mov eax,[edx+0x108]);
// the note named it "reference record" instead of "ref", so it did not parse.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void saved_games_report_last_error(void); // 0x556170, this module

// blam-cc: ref in EDX, buffer in ECX, size in ESI
// Reads exactly size bytes from ref's open handle into buffer. Fails (ERROR_HANDLE_EOF, 0x26)
// if fewer bytes were read than requested. Returns 1 on success, 0 on failure (after reporting
// the Win32 error).
uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size)
{
    int32_t ok;
    uint32_t bytes_read;

    ok = ReadFile(ref->handle, buffer, size, &bytes_read, 0);
    if (ok != 0) {
        if (bytes_read == size) {
            return 1;
        }
        SetLastError(0x26);
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x555a20):

undefined4 file_reference_read(void)

{
  BOOL BVar1;
  DWORD dwMessageId;
  LPVOID in_ECX;
  int in_EDX;
  DWORD unaff_ESI;
  DWORD local_804;
  CHAR local_800 [2048];

  BVar1 = ReadFile(*(HANDLE *)(in_EDX + 0x108),in_ECX,unaff_ESI,&local_804,(LPOVERLAPPED)0x0);
  if (BVar1 != 0) {
    if (local_804 == unaff_ESI) {
      return 1;
    }
    SetLastError(0x26);
  }
  dwMessageId = GetLastError();
  FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_800,0x800,(va_list *)0x0);
  SetLastError(0);
  return 0;
}
#endif
