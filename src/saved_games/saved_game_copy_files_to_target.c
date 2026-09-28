// saved_game_copy_files_to_target  (Ghidra: saved_game_copy_files_to_target, already named)
// address 0x5387e0, size 214 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x5387e0 re-derived by hand
// because Ghidra dropped 3 of this function's 4 sprintf arguments (they live in the very
// callee-saved push slots the disassembly reuses as the stack the sprintf calls read from,
// [esp+8]/[esp+0xc] aliasing the pushed ESI/EDI). Cross-checked against the caller
// game_state_save_thread_proc (0x538980, disassembly 0x538a60..0x538a7e): call 1 passes
// ESI=profile directory, EDI="checkpoints\\autosave" (the STRING itself, source), stack
// target="checkpoints\\autosave1"; call 2 reuses the same ESI, passes EDI="savegame" (source),
// stack target=EDI's previous value "checkpoints\\autosave" -- i.e. it first backs up the
// current newest autosave to autosave1, then copies the just-written savegame over the newest
// autosave slot. objdump of 0x670aa4/0x670ab0 confirms the "%s%s.bin" / "%s%s.sav" formats.
// register convention: source_directory in ESI, source_name in EDI, target_name is the
// recognized stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t _sprintf(char *dest, const char *format, ...); // 0x623693
extern void *__stdcall FindFirstFileA(const char *path, win32_find_dataa *find_data); // Win32
extern int32_t __stdcall FindClose(void *find_handle); // Win32
extern int32_t __stdcall CopyFileA(const char *existing_path, const char *new_path, int32_t fail_if_exists); // Win32

// blam-cc: source_directory in ESI, source_name in EDI, then the recognized stack parameter
// (target_name)
// If "<source_directory><source_name>.sav" exists, copies both the .bin and .sav files of
// source_name onto target_name (both within source_directory). Used for autosave rotation
// (copy the previous newest autosave to autosave1, then copy the just-written save onto the
// newest autosave slot) and for checkpoint loading.
uint8_t saved_game_copy_files_to_target(char *source_directory, char *source_name, char *target_name)
{
    char check_path[256];
    win32_find_dataa find_data;
    void *find_handle;
    char source_path[256];
    char target_path[256];

    _sprintf(check_path, "%s%s.sav", source_directory, source_name);
    find_handle = FindFirstFileA(check_path, &find_data);
    if (find_handle == (void *)0xffffffff) {
        return 0;
    }
    FindClose(find_handle);

    _sprintf(target_path, "%s%s.bin", source_directory, target_name);
    _sprintf(source_path, "%s%s.bin", source_directory, source_name);
    CopyFileA(source_path, target_path, 0);

    _sprintf(source_path, "%s%s.sav", source_directory, source_name);
    _sprintf(target_path, "%s%s.sav", source_directory, target_name);
    CopyFileA(source_path, target_path, 0);
    return 1;
}

#if 0
Original Ghidra decompilation (0x5387e0):

undefined4 saved_game_copy_files_to_target(void)

{
  HANDLE hFindFile;
  char local_240 [256];
  _WIN32_FIND_DATAA local_140;

  _sprintf(local_240,"%s%s.sav");
  hFindFile = FindFirstFileA(local_240,&local_140);
  if (hFindFile != (HANDLE)0xffffffff) {
    FindClose(hFindFile);
    _sprintf((char *)&local_140,"%s%s.bin");
    _sprintf(local_240,"%s%s.bin");
    CopyFileA(local_240,(LPCSTR)&local_140,0);
    _sprintf(local_240,"%s%s.sav");
    _sprintf((char *)&local_140,"%s%s.sav");
    CopyFileA(local_240,(LPCSTR)&local_140,0);
    return 1;
  }
  return 0;
}
#endif
