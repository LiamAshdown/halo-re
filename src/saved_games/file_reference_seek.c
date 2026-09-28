// file_reference_seek  (Ghidra: file_reference_seek, already named)
// address 0x5558f0, size 87 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA; confirmed by objdump 0x5558f0..0x555948 (`push
// ecx+0x108` handle deref, in_EAX used directly as the SetFilePointer distance).
// register convention: offset in EAX, reference record in ECX.
// FIXED (register inputs, objdump): notes wording ("reference record in ECX") didn't match the
// checker's alias for file_reference_record*, so ECX (read at 0x5558f0) looked unclaimed. Body
// already used ref->handle correctly; reworded the blam-cc line to the standard "REG -> name" form.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void saved_games_report_last_error(void); // 0x556170, this module
extern uint32_t __stdcall SetFilePointer(void *file, int32_t distance, int32_t *distance_high, uint32_t method); // Win32

// blam-cc: EAX -> offset, ECX -> ref
// Seeks ref's open handle to an absolute byte offset. Returns 1 on success, 0 on failure (after
// reporting the Win32 error).
uint8_t file_reference_seek(int32_t offset, file_reference_record *ref)
{
    uint32_t result;

    result = SetFilePointer(ref->handle, offset, 0, 0);
    if (result == 0xffffffff) {
        saved_games_report_last_error();
    }
    return result != 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x5558f0):

bool file_reference_seek(void)

{
  LONG in_EAX;
  DWORD DVar1;
  DWORD dwMessageId;
  int in_ECX;
  CHAR local_800 [2048];

  DVar1 = SetFilePointer(*(HANDLE *)(in_ECX + 0x108),in_EAX,(PLONG)0x0,0);
  if (DVar1 == 0xffffffff) {
    dwMessageId = GetLastError();
    FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_800,0x800,(va_list *)0x0);
    SetLastError(0);
  }
  return DVar1 != 0xffffffff;
}
#endif
