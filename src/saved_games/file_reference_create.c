// file_reference_create  (Ghidra: file_reference_create, already named)
// address 0x5555b0, size 189 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: already named by Ghidra/CEA. Confirmed against objdump 0x5555b0..0x55563f: EAX is
// the reference record (`mov esi,eax` at entry copies it, then `mov cx,[esi+6]` / `lea
// eax,[esi+8]` feed path_build_full). CreateDirectoryA is called with EDI = ref->path directly
// (the RAW, un-prefixed path, loaded at `lea edi,[esi+8]` before path_build_full's own EDX
// destination is touched) rather than the path_build_full output; only the CreateFileA branch
// uses the built absolute path. This looks like a quirk of the original binary rather than a
// decompiler artifact -- objdump confirms EDI (not the local scratch buffer) is pushed as
// CreateDirectoryA's path argument -- so it is kept exactly as found.
// register convention: reference record in EAX (confirmed by objdump: `mov esi,eax` at entry,
// then `mov cx,[esi+6]` / `lea eax,[esi+8]` feed path_build_full).

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
extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, this module
extern void saved_games_report_last_error(void); // 0x556170, this module


// blam-cc: reference record in EAX
// Creates the directory or file described by ref (directory when the is-file bit of flags is
// clear, file otherwise). Tolerates ERROR_ALREADY_EXISTS (0xb7) for a directory. Returns 1 on
// success, 0 on failure (after reporting the Win32 error).
uint8_t file_reference_create(file_reference_record *ref)
{
    char full_path[0x800];
    void *handle;
    int32_t created;
    uint32_t error;

    path_build_full(ref->path, full_path, ref->location);
    if ((ref->flags & _file_reference_is_file_bit) == 0) {
        // UNSURE: the binary passes ref->path here, not the full_path just built above (see the
        // header comment); preserved as found rather than "corrected".
        created = CreateDirectoryA(ref->path, 0);
        if (created == 0) {
            error = GetLastError();
            if (error != 0xb7) {
                saved_games_report_last_error();
                return 0;
            }
        }
    } else {
        handle = CreateFileA(full_path, 0x40000000, 0, 0, 2, 0x80, 0);
        if (handle == (void *)-1) {
            saved_games_report_last_error();
            return 0;
        }
        CloseHandle(handle);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5555b0):

undefined4 file_reference_create(void)

{
  int in_EAX;
  HANDLE hObject;
  BOOL BVar1;
  DWORD DVar2;
  int iVar3;
  undefined4 *puVar4;
  CHAR local_908;
  undefined4 local_907;
  CHAR local_808 [2052];

  local_908 = '\0';
  puVar4 = &local_907;
  for (iVar3 = 0x3f; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = 0;
  *(undefined1 *)((int)puVar4 + 2) = 0;
  FUN_005560d0();
  if ((*(byte *)(in_EAX + 4) & 1) == 0) {
    BVar1 = CreateDirectoryA((LPCSTR)(in_EAX + 8),(LPSECURITY_ATTRIBUTES)0x0);
    if (BVar1 == 0) {
      DVar2 = GetLastError();
      if (DVar2 != 0xb7) goto LAB_0055563b;
    }
  }
  else {
    hObject = CreateFileA(&local_908,0x40000000,0,(LPSECURITY_ATTRIBUTES)0x0,2,0x80,(HANDLE)0x0);
    if (hObject == (HANDLE)0xffffffff) {
LAB_0055563b:
      DVar2 = GetLastError();
      FormatMessageA(0x12ff,(LPCVOID)0x0,DVar2,0,local_808,0x800,(va_list *)0x0);
      SetLastError(0);
      return 0;
    }
    CloseHandle(hObject);
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
