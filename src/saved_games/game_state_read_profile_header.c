// game_state_read_profile_header  (Ghidra: game_state_read_profile_header, already named)
// address 0x539460, size 123 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x539460 confirms the sprintf
// argument order Ghidra dropped: sprintf(dest, "%s\\%s", game_state_core_directory, name), i.e.
// EAX (name) is the second %s; ReadFile's size is EDI (register-passed, matching the caller
// game_state_load_core's EDI=k_game_state_header_size setup) and its destination buffer is the
// function's own recognized stack parameter.
// register convention: name in EAX, size in EDI, buffer is the recognized stack parameter.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern char game_state_core_directory[0x100]; // 0x006e2efc


// blam-cc: name in EAX, size in EDI, then the recognized stack parameter (buffer)
// Opens "<game_state_core_directory>\<name>" and reads exactly size bytes into buffer,
// returning success without raising a fatal error.
uint8_t game_state_read_profile_header(char *name, int32_t size, void *buffer)
{
    char path[1024];
    void *file;
    uint32_t bytes_read;
    uint8_t result;

    result = 0;
    sprintf(path, "%s\\%s", game_state_core_directory, name);
    file = CreateFileA(path, 0x80000000, 0, 0, 3 /* OPEN_EXISTING */, 0x80 /* FILE_FLAG_RANDOM_ACCESS */, 0);
    if (file != (void *)0xffffffff) {
        if (ReadFile(file, buffer, size, &bytes_read, 0) != 0 && bytes_read == (uint32_t)size) {
            result = 1;
        }
    }
    CloseHandle(file);
    return result;
}

#if 0
Original Ghidra decompilation (0x539460):

undefined1 game_state_read_profile_header(LPVOID param_1)

{
  HANDLE hFile;
  BOOL BVar1;
  undefined1 uVar2;
  DWORD unaff_EDI;
  DWORD local_404;
  char local_400 [1024];

  uVar2 = 0;
  _sprintf(local_400,"%s\\%s",&DAT_006e2efc);
  hFile = CreateFileA(local_400,0x80000000,0,(LPSECURITY_ATTRIBUTES)0x0,3,0x80,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    BVar1 = ReadFile(hFile,param_1,unaff_EDI,&local_404,(LPOVERLAPPED)0x0);
    if ((BVar1 != 0) && (local_404 == unaff_EDI)) {
      uVar2 = 1;
    }
  }
  CloseHandle(hFile);
  return uVar2;
}
#endif
