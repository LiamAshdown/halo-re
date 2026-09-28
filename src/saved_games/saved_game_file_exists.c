// saved_game_file_exists  (Ghidra: saved_game_file_exists, already named)
// address 0x538770, size 100 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x538770 confirms
// FUN_0053d080's (EAX handle, ESI out buffer) convention with EAX = the current profile handle
// (0x00714dd4) and objdump's 0x670ab0 string dump ("%s%s.sav") sets the sprintf argument order:
// dest, format, directory (ESI), name (the function's own recognized stack parameter).
// register convention: __cdecl; name is the recognized stack parameter.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern int32_t _sprintf(char *dest, const char *format, ...); // 0x623693

// Checks whether "<current profile directory><name>.sav" exists on disk.
uint8_t saved_game_file_exists(char *name)
{
    char directory[264];
    char path[832];
    win32_find_dataa find_data;
    void *find_handle;
    uint8_t found;

    found = 0;
    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    _sprintf(path, "%s%s.sav", directory, name);
    find_handle = FindFirstFileA(path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)0xffffffff) {
        found = 1;
        FindClose(find_handle);
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x538770):

bool saved_game_file_exists(undefined4 param_1)

{
  HANDLE hFindFile;
  char local_340 [256];
  undefined1 local_240 [256];
  _WIN32_FIND_DATAA local_140;

  FUN_0053d080();
  _sprintf(local_340,"%s%s.sav",local_240,param_1);
  hFindFile = FindFirstFileA(local_340,&local_140);
  if (hFindFile != (HANDLE)0xffffffff) {
    FindClose(hFindFile);
  }
  return hFindFile != (HANDLE)0xffffffff;
}
#endif
