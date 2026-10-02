// game_state_read_profile_file  (Ghidra: game_state_read_profile_file, already named)
// address 0x5394e0, size 135 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; identical path-building and read pattern to
// game_state_read_profile_header (0x539460), confirmed by the shared "%s\\%s" +
// game_state_core_directory strings/globals; the caller game_state_load_core (0x538390) sets
// up EAX=name, EDI=k_game_state_size, stack=game_state_base for this call (disassembly at
// 0x5383d1..0x5383d9), matching the sibling reader's register convention exactly. On failure
// this one raises a fatal error dialog instead of returning a status.
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

extern char game_state_core_directory[0x100]; // 0x006e2efc

extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

// blam-cc: name in EAX, size in EDI, then the recognized stack parameter (buffer)
// Opens "<game_state_core_directory>\<name>" and reads exactly size bytes into buffer,
// raising a fatal error dialog (string 0x8b, title 0x8c) if the open or the read fails.
void game_state_read_profile_file(char *name, int32_t size, void *buffer)
{
    char path[1024];
    void *file;
    uint32_t bytes_read;

    sprintf(path, "%s\\%s", game_state_core_directory, name);
    file = CreateFileA(path, 0x80000000, 0, 0, 3 /* OPEN_EXISTING */, 0x80 /* FILE_FLAG_RANDOM_ACCESS */, 0);
    if (file == (void *)0xffffffff ||
        ReadFile(file, buffer, size, (LPDWORD)&bytes_read, 0) == 0 ||
        bytes_read != (uint32_t)size) {
        shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
    }
    CloseHandle(file);
}

#if 0
Original Ghidra decompilation (0x5394e0):

void game_state_read_profile_file(LPVOID param_1)

{
  HANDLE hFile;
  BOOL BVar1;
  DWORD unaff_EDI;
  DWORD local_404;
  char local_400 [1024];

  _sprintf(local_400,"%s\\%s",&DAT_006e2efc);
  hFile = CreateFileA(local_400,0x80000000,0,(LPSECURITY_ATTRIBUTES)0x0,3,0x80,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    BVar1 = ReadFile(hFile,param_1,unaff_EDI,&local_404,(LPOVERLAPPED)0x0);
    if ((BVar1 != 0) && (local_404 == unaff_EDI)) goto LAB_00539558;
  }
  shell_display_fatal_error_dialog(0x8b,0x8c,1);
LAB_00539558:
  CloseHandle(hFile);
  return;
}
#endif
