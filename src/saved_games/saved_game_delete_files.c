// saved_game_delete_files  (Ghidra: saved_game_delete_files, already named)
// address 0x5388c0, size 192 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x5388c0 confirms EDI (never
// assigned in-function) is pushed as the sprintf "name" argument at both call sites (0x538910,
// 0x538933), unlike the stack-parameter convention saved_game_file_exists (0x538770) uses for
// the structurally similar name argument -- confirmed per-function per the module's register-
// convention note. FUN_0053d080's (EAX handle, ESI out buffer) convention as in the sibling
// file helpers; handle is the fixed current-profile handle (0x00714dd4).
// register convention: name in EDI; no recognized stack parameters.

#include "crt.h"
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

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL

// blam-cc: name in EDI
// Deletes "<current profile directory><name>.bin" and "<name>.sav", but only if the .sav
// exists. Returns whether either file was deleted.
uint8_t saved_game_delete_files(char *name)
{
    char directory[264];
    char check_path[256];
    win32_find_dataa find_data;
    void *find_handle;
    char path[256];
    int32_t deleted_bin;
    int32_t deleted_sav;

    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(check_path, "%s%s.sav", directory, name);
    find_handle = FindFirstFileA(check_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle == (void *)0xffffffff) {
        return 0;
    }
    FindClose(find_handle);

    sprintf(path, "%s%s.bin", directory, name);
    deleted_bin = DeleteFileA(path);
    sprintf(path, "%s%s.sav", directory, name);
    deleted_sav = DeleteFileA(path);
    if (deleted_sav == 0 && deleted_bin == 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5388c0):

undefined4 saved_game_delete_files(void)

{
  HANDLE hFindFile;
  BOOL BVar1;
  BOOL BVar2;
  char local_340 [256];
  undefined1 local_240 [256];
  _WIN32_FIND_DATAA local_140;

  FUN_0053d080();
  _sprintf(local_340,"%s%s.sav",local_240);
  hFindFile = FindFirstFileA(local_340,&local_140);
  if (hFindFile == (HANDLE)0xffffffff) {
    return 0;
  }
  FindClose(hFindFile);
  _sprintf(local_340,"%s%s.bin",local_240);
  BVar1 = DeleteFileA(local_340);
  _sprintf(local_340,"%s%s.sav",local_240);
  BVar2 = DeleteFileA(local_340);
  if ((BVar2 == 0) && ((char)BVar1 == '\0')) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
