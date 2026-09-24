// file_reference_get_size  (Ghidra: file_reference_get_size, already named)
// address 0x555950, size 81 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA; Ghidra's own `in_EAX` reads `*(HANDLE*)(in_EAX+0x108)`
// directly, matching file_reference_record::handle.
// register convention: reference record in EAX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void saved_games_report_last_error(void); // 0x556170, this module
extern uint32_t GetFileSize(void *file, uint32_t *file_size_high); // Win32

// blam-cc: reference record in EAX
// Returns the size in bytes of ref's open handle, or 0xffffffff on failure (after reporting the
// Win32 error).
uint32_t file_reference_get_size(file_reference_record *ref)
{
    uint32_t size;

    size = GetFileSize(ref->handle, 0);
    if (size == 0xffffffff) {
        saved_games_report_last_error();
    }
    return size;
}

#if 0
Original Ghidra decompilation (0x555950):

DWORD file_reference_get_size(void)

{
  int in_EAX;
  DWORD DVar1;
  DWORD dwMessageId;
  CHAR local_800 [2048];

  DVar1 = GetFileSize(*(HANDLE *)(in_EAX + 0x108),(LPDWORD)0x0);
  if (DVar1 == 0xffffffff) {
    dwMessageId = GetLastError();
    FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_800,0x800,(va_list *)0x0);
    SetLastError(0);
  }
  return DVar1;
}
#endif
