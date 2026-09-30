// file_reference_get_size  (Ghidra: file_reference_get_size, already named)
// address 0x555950, size 81 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA; Ghidra's own `in_EAX` reads `*(HANDLE*)(in_EAX+0x108)`
// directly, matching file_reference_record::handle.
// register convention: EAX -> ref.
// FIXED (register inputs, objdump): notes wording ("reference record in EAX") didn't match the
// checker's alias for file_reference_record*, so EAX (read at 0x555950) looked unclaimed. Body
// already used ref correctly; reworded the blam-cc line to the standard "REG -> name" form.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


// blam-cc: EAX -> ref
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
