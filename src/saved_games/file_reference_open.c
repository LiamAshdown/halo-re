// file_reference_open  (Ghidra: file_reference_open, already named)
// address 0x5557a0, size 230 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: already named by Ghidra/CEA; types/saved_games.h file_reference_open_mode: bit 0
// read (maps to GENERIC_READ), bit 1 write (GENERIC_WRITE), bit 2 append (seek to end after
// opening). Confirmed against objdump 0x5557a0..0x5557f4: the mode byte is the one stack
// argument ([ebp+8]); ESI (no assignment in the prologue) is the reference record.
// register convention: reference record in ESI; open-mode flags as the one stack argument.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


// blam-cc: reference record in ESI; open-mode flags as the one stack argument
// Opens ref's full path with CreateFileA (share mode FILE_SHARE_READ, OPEN_ALWAYS,
// FILE_ATTRIBUTE_NORMAL), storing the handle in ref->handle. If _file_open_append is set, seeks
// to the end of the file; on failure to do so, closes and clears the handle. Returns 1 on
// success, 0 on failure (after reporting the Win32 error).
uint8_t file_reference_open(file_reference_record *ref, uint8_t mode)
{
    char full_path[0x800];
    void *handle;
    uint32_t desired_access;
    uint32_t seek_result;

    path_build_full(ref->path, full_path, ref->location);
    desired_access = 0;
    if ((mode & _file_open_read) != 0) {
        desired_access = 0x80000000;
    }
    if ((mode & _file_open_write) != 0) {
        desired_access = desired_access | 0x40000000;
    }
    handle = CreateFileA(full_path, desired_access, 1, 0, 3, 0x80, 0);
    if (handle != (void *)-1) {
        ref->handle = handle;
        if ((mode & _file_open_append) == 0) {
            return 1;
        }
        seek_result = SetFilePointer(handle, 0, 0, 2);
        if (seek_result != 0xffffffff) {
            return 1;
        }
        CloseHandle(ref->handle);
        ref->handle = 0;
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x5557a0):

undefined4 file_reference_open(byte param_1)

{
  HANDLE hFile;
  int iVar1;
  int unaff_ESI;
  undefined4 *puVar2;
  DWORD DVar3;
  CHAR local_908;
  undefined4 local_907;
  CHAR local_808 [2052];

  local_908 = '\0';
  puVar2 = &local_907;
  for (iVar1 = 0x3f; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  *(undefined1 *)((int)puVar2 + 2) = 0;
  DVar3 = 0;
  FUN_005560d0();
  if ((param_1 & 1) != 0) {
    DVar3 = 0x80000000;
  }
  if ((param_1 & 2) != 0) {
    DVar3 = DVar3 | 0x40000000;
  }
  hFile = CreateFileA(&local_908,DVar3,1,(LPSECURITY_ATTRIBUTES)0x0,3,0x80,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    *(HANDLE *)(unaff_ESI + 0x108) = hFile;
    if ((param_1 & 4) == 0) {
      return 1;
    }
    DVar3 = SetFilePointer(hFile,0,(PLONG)0x0,2);
    if (DVar3 != 0xffffffff) {
      return 1;
    }
    CloseHandle(*(HANDLE *)(unaff_ESI + 0x108));
    *(undefined4 *)(unaff_ESI + 0x108) = 0;
  }
  DVar3 = GetLastError();
  FormatMessageA(0x12ff,(LPCVOID)0x0,DVar3,0,local_808,0x800,(va_list *)0x0);
  SetLastError(0);
  return 0;
}
#endif
