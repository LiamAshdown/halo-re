// directory_create_recursive  (Ghidra: directory_create_recursive, already named)
// address 0x449250, size 278 bytes
// name confidence: 0.8   rewrite confidence: 0.85  (reviewed line by line against objdump)
// evidence: out/phase2/cseries/00.md; calls GetFileAttributesA to check if the path already
// exists; if not, walks the path splitting on '\\' (skipping the first two components for UNC
// "\\server\share" paths via strpbrk) and calls CreateDirectoryA on each successive prefix,
// wrapped in SetErrorMode(0x8000) to suppress the critical-error dialog for missing drives.
// register convention: __cdecl(char *path); confirmed at the callee prologue -- the frame is
// `sub esp,0x108; push ebx,esi,edi`, and `path` is read from [esp+0x118] (0x108 locals + 0xc
// saved registers + 0x4 return address), the standard cdecl stack slot for the first argument.
// Returns the byte in AL.
//   // blam-cc: stack -> path

#include "tags.h"
#include "cseries.h"
#include <string.h>

extern uint32_t SetErrorMode(uint32_t mode);                                  // Win32
extern uint32_t GetFileAttributesA(const char *path);                         // Win32
extern uint32_t CreateDirectoryA(const char *path, void *security_attributes); // Win32

// Creates `path` and every missing parent directory along it. If `path` already exists (per
// GetFileAttributesA), returns 1 immediately. Otherwise walks the path one '\\'-delimited
// component at a time, creating each prefix directory that GetFileAttributesA reports missing;
// a UNC path's "\\server\share" prefix is skipped (the walk starts after it), since that portion
// cannot be created. Returns 1 if the full path exists or was successfully created, 0 otherwise.
// SetErrorMode(SEM_NOOPENFILEERRORBOX) is held for the whole walk so a missing drive does not
// pop the Windows "insert disk" dialog. It is restored on every path EXCEPT the early "already
// exists" return (0x449286..0x449291 returns 1 without the second SetErrorMode call), so the
// process keeps SEM_NOOPENFILEERRORBOX after any call on an existing path. That is the retail
// binary's behaviour and is reproduced here on purpose.
// The UNC walk: for "\\server\share\..." the first two separators after the leading pair are
// skipped; if either is missing the function fails (returns 0) without creating anything.
// The final CreateDirectoryA uses the caller's `path`, not the scratch copy (0x449340 push ebx,
// EBX reloaded from the argument slot at 0x4492ee on every iteration).
char directory_create_recursive(char *path)
    // blam-cc: stack -> path
{
    char buffer[k_directory_create_buffer_size];
    uint8_t all_created;
    uint32_t previous_error_mode;
    uint32_t attributes;
    char *cursor;
    char saved_char;

    all_created = 1;
    cursor = buffer;
    previous_error_mode = SetErrorMode(k_sem_noopenfileerrorbox);
    attributes = GetFileAttributesA(path);
    if (attributes != 0xffffffff) {
        return 1;
    }

    strcpy(buffer, path);

    if (buffer[0] != '\\' || buffer[1] != '\\' ||
        ((cursor = strpbrk(buffer + 2, "\\")) != 0 &&
         (cursor = strpbrk(cursor + 1, "\\")) != 0)) {
        do {
            cursor = strpbrk(cursor + 1, "\\");
            if (cursor == 0) {
                if (all_created == 1 && CreateDirectoryA(path, 0) != 0) {
                    goto done;
                }
                break;
            }
            saved_char = *cursor;
            *cursor = '\0';
            attributes = GetFileAttributesA(buffer);
            if (attributes == 0xffffffff && CreateDirectoryA(buffer, 0) == 0) {
                all_created = 0;
            }
            *cursor = saved_char;
        } while (all_created == 1);
    }
    all_created = 0;
done:
    SetErrorMode(previous_error_mode);
    return (char)all_created;
}

#if 0
Original Ghidra decompilation (0x449250), from tools/pack.py 0x449250:

char __cdecl directory_create_recursive(char *path)

{
  char cVar1;
  UINT uMode;
  DWORD DVar2;
  char *pcVar3;
  char *pcVar4;
  BOOL BVar5;
  char local_105;
  char local_100 [256];

  local_105 = '\x01';
  pcVar4 = local_100;
  uMode = SetErrorMode(0x8000);
  DVar2 = GetFileAttributesA(path);
  if (DVar2 != 0xffffffff) {
    return '\x01';
  }
  pcVar3 = path;
  do {
    cVar1 = *pcVar3;
    pcVar3[(int)(local_100 + -(int)path)] = cVar1;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  if (((local_100[0] != '\\') || (local_100[1] != '\\')) ||
     ((pcVar4 = _strpbrk(local_100 + 2,"\\"), pcVar4 != (char *)0x0 &&
      (pcVar4 = _strpbrk(pcVar4 + 1,"\\"), pcVar4 != (char *)0x0)))) {
    do {
      pcVar4 = _strpbrk(pcVar4 + 1,"\\");
      if (pcVar4 == (char *)0x0) {
        if ((local_105 == '\x01') &&
           (BVar5 = CreateDirectoryA(path,(LPSECURITY_ATTRIBUTES)0x0), BVar5 != 0))
        goto LAB_0044934c;
        break;
      }
      cVar1 = *pcVar4;
      *pcVar4 = '\0';
      DVar2 = GetFileAttributesA(local_100);
      if ((DVar2 == 0xffffffff) &&
         (BVar5 = CreateDirectoryA(local_100,(LPSECURITY_ATTRIBUTES)0x0), BVar5 == 0)) {
        local_105 = '\0';
      }
      *pcVar4 = cVar1;
    } while (local_105 == '\x01');
  }
  local_105 = '\0';
LAB_0044934c:
  SetErrorMode(uMode);
  return local_105;
}
#endif
