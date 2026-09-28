// file_reference_delete  (Ghidra: file_reference_delete, already named)
// address 0x555670, size 173 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: already named by Ghidra/CEA. Confirmed against objdump 0x555670..0x5556d5: ESI
// (register, unaff_ESI in Ghidra's output -- no `mov esi,X` in the prologue, so it is a genuine
// incoming argument) feeds `mov cx,[esi+6]` / `lea eax,[esi+8]` into path_build_full, and
// SetFileAttributesA / DeleteFileA / RemoveDirectoryA all take the built full path.
// register convention: ESI -> ref.
// FIXED (register inputs, objdump): notes wording ("reference record in ESI") didn't match the
// checker's alias for file_reference_record*, so ESI (read at 0x555691) looked unclaimed. Body
// already used ref correctly; reworded the blam-cc line to the standard "REG -> name" form.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, this module
extern void saved_games_report_last_error(void); // 0x556170, this module

extern int32_t __stdcall RemoveDirectoryA(const char *path); // Win32
extern int32_t __stdcall SetFileAttributesA(const char *path, uint32_t attributes); // Win32
extern int32_t __stdcall DeleteFileA(const char *path); // Win32

// blam-cc: ESI -> ref
// Deletes the directory or file described by ref (built to its full path). Returns 1 on
// success, 0 on failure (after reporting the Win32 error).
uint8_t file_reference_delete(file_reference_record *ref)
{
    char full_path[0x800];
    int32_t ok;

    path_build_full(ref->path, full_path, ref->location);
    if ((ref->flags & _file_reference_is_file_bit) == 0) {
        ok = RemoveDirectoryA(full_path);
        if (ok != 0) {
            return 1;
        }
    } else {
        ok = SetFileAttributesA(full_path, 0x80);
        if (ok != 0) {
            ok = DeleteFileA(full_path);
            if (ok != 0) {
                return 1;
            }
        }
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x555670):

undefined4 file_reference_delete(void)

{
  BOOL BVar1;
  DWORD dwMessageId;
  int iVar2;
  int unaff_ESI;
  undefined4 *puVar3;
  CHAR local_908;
  undefined4 local_907;
  CHAR local_808 [2052];

  local_908 = '\0';
  puVar3 = &local_907;
  for (iVar2 = 0x3f; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = 0;
  *(undefined1 *)((int)puVar3 + 2) = 0;
  FUN_005560d0();
  if ((*(byte *)(unaff_ESI + 4) & 1) == 0) {
    BVar1 = RemoveDirectoryA(&local_908);
    if (BVar1 != 0) {
      return 1;
    }
  }
  else {
    BVar1 = SetFileAttributesA(&local_908,0x80);
    if (BVar1 != 0) {
      BVar1 = DeleteFileA(&local_908);
      if (BVar1 != 0) {
        return 1;
      }
    }
  }
  dwMessageId = GetLastError();
  FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_808,0x800,(va_list *)0x0);
  SetLastError(0);
  return 0;
}
#endif
