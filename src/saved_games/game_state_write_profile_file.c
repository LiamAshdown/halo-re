// game_state_write_profile_file  (Ghidra: game_state_write_profile_file, already named)
// address 0x5393d0, size 143 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; same "%s\\%s" + game_state_core_directory
// pattern as game_state_read_profile_header/_file, with the size argument in the same EDI slot
// (confirmed by that sibling pair's disassembly); the two recognized stack parameters (name,
// buffer) match the call order sprintf/WriteFile use them in.
// register convention: size in EDI; name and buffer are the recognized stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern char game_state_core_directory[0x100]; // 0x006e2efc

extern uint32_t CreateDirectoryA(const char *path, void *security_attributes); // Win32
extern int32_t _sprintf(char *dest, const char *format, ...); // 0x623693
extern void *CreateFileA(const char *path, uint32_t access, uint32_t share_mode,
    void *security_attributes, uint32_t creation_disposition, uint32_t flags, void *template_file); // Win32
extern uint32_t WriteFile(void *file, const void *buffer, uint32_t bytes_to_write, uint32_t *bytes_written, void *overlapped); // Win32
extern uint32_t CloseHandle(void *handle); // Win32

// blam-cc: size in EDI, then the recognized stack parameters (name, buffer)
// Ensures game_state_core_directory exists, then writes size bytes of buffer to
// "<game_state_core_directory>\<name>", creating or truncating the file. Returns success.
uint8_t game_state_write_profile_file(int32_t size, char *name, const void *buffer)
{
    char path[1024];
    void *file;
    uint32_t bytes_written;
    uint8_t result;

    result = 0;
    CreateDirectoryA(game_state_core_directory, 0);
    _sprintf(path, "%s\\%s", game_state_core_directory, name);
    file = CreateFileA(path, 0x40000000, 0, 0, 2 /* CREATE_ALWAYS */, 0x80 /* FILE_FLAG_RANDOM_ACCESS */, 0);
    if (file != (void *)0xffffffff) {
        if (WriteFile(file, buffer, size, &bytes_written, 0) != 0 && bytes_written == (uint32_t)size) {
            result = 1;
        }
    }
    CloseHandle(file);
    return result;
}

#if 0
Original Ghidra decompilation (0x5393d0):

undefined1 game_state_write_profile_file(undefined4 param_1,LPCVOID param_2)

{
  HANDLE hFile;
  BOOL BVar1;
  undefined1 uVar2;
  DWORD unaff_EDI;
  DWORD local_404;
  char local_400 [1024];

  uVar2 = 0;
  CreateDirectoryA(&DAT_006e2efc,(LPSECURITY_ATTRIBUTES)0x0);
  _sprintf(local_400,"%s\\%s",&DAT_006e2efc,param_1);
  hFile = CreateFileA(local_400,0x40000000,0,(LPSECURITY_ATTRIBUTES)0x0,2,0x80,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    BVar1 = WriteFile(hFile,param_2,unaff_EDI,&local_404,(LPOVERLAPPED)0x0);
    if ((BVar1 != 0) && (local_404 == unaff_EDI)) {
      uVar2 = 1;
    }
  }
  CloseHandle(hFile);
  return uVar2;
}
#endif
