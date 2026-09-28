// game_state_open_persistent_storage  (Ghidra: game_state_open_persistent_storage, already named)
// address 0x5398e0, size 350 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x5398e0 re-derived by hand
// because Ghidra's decompilation of the two path-building loops is unreadable (raw pointer-
// offset tricks for a strcpy and an end-of-string scan): when name is NULL the current
// profile's directory (FUN_0053d080, called twice back to back on the same buffer -- both
// calls reproduced verbatim even though the second is redundant, since that is what the
// disassembly does) is used; when name is given it is copied in directly and the two
// FUN_0053d080 calls are skipped entirely (the jump at 0x539a30 lands past them); either way
// "savegame.bin" is appended at the end of the resulting string before CreateFileA. objdump
// also confirms the 0x1000-dword zero block, the 0x4000-byte initial WriteFile and the
// SetFilePointer/SetEndOfFile pre-sizing to k_game_state_file_size, matching
// out/phase4/saved_games_types_notes.md's file globals. FUN_0053d080's (EAX handle, ESI out
// buffer) convention confirmed in objdump (0x5398fe, 0x539914, 0x5399ee).
// register convention: __cdecl (Ghidra-recognized); name is the one recognized stack parameter.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4 (saved_player_profile_slots[0].handle)

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

extern char *strcpy(char *dest, const char *source);
extern uint32_t strlen(const char *str);
extern void *memset(void *dest, int32_t value, uint32_t count);

// Opens (creating and pre-sizing to k_game_state_file_size if necessary) the savegame.bin file
// under either the caller-supplied directory (name) or, when name is NULL, the current player
// profile's own directory. Returns the file handle, or (void *)-1 on failure.
void *game_state_open_persistent_storage(char *name)
{
    char path[0x120];
    char *end;
    void *file;
    uint32_t file_size;
    uint32_t bytes_written;
    uint32_t zero_block[k_game_state_file_initial_block / 4];
    char delete_path[0x120];

    if (name == 0) {
        if (saved_game_get_directory_by_handle(saved_player_profile_slots_handle, path) == 0) {
            return (void *)0xffffffff;
        }
        saved_game_get_directory_by_handle(saved_player_profile_slots_handle, path);
    } else {
        strcpy(path, name);
    }

    end = path + strlen(path);
    strcpy(end, "savegame.bin");

    file = CreateFileA(path, 0xc0000000, 0, 0, 4 /* OPEN_ALWAYS */, 0, 0);
    if (file == (void *)0xffffffff) {
        return (void *)0xffffffff;
    }

    file_size = GetFileSize(file, 0);
    if (file_size != k_game_state_file_size) {
        memset(zero_block, 0, sizeof(zero_block));
        if (WriteFile(file, zero_block, k_game_state_file_initial_block, &bytes_written, 0) == 0 ||
            bytes_written != k_game_state_file_initial_block ||
            SetFilePointer(file, k_game_state_file_size, 0, 0) == 0xffffffff ||
            SetEndOfFile(file) == 0) {
            shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
            if (saved_game_get_directory_by_handle(saved_player_profile_slots_handle, delete_path) != 0) {
                DeleteFileA(delete_path);
            }
            CloseHandle(file);
            return (void *)0xffffffff;
        }
    }
    return file;
}

#if 0
Original Ghidra decompilation (0x5398e0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

HANDLE __cdecl game_state_open_persistent_storage(char *name)

{
  char *pcVar1;
  char cVar2;
  HANDLE hFile;
  undefined1 *puVar3;
  BOOL BVar4;
  DWORD DVar5;
  int iVar6;
  char *pcVar7;
  undefined4 *puVar8;
  undefined4 local_420c;
  CHAR local_4208 [12];
  char local_41fc [244];
  CHAR local_4108 [256];
  undefined4 local_4008 [4095];
  undefined4 uStack_c;

  uStack_c = 0x5398f0;
  if (name == (char *)0x0) {
    cVar2 = FUN_0053d080();
    if (cVar2 == '\0') {
      return (HANDLE)0xffffffff;
    }
    FUN_0053d080();
  }
  else {
    iVar6 = -(int)name;
    do {
      cVar2 = *name;
      name[(int)(local_4208 + iVar6)] = cVar2;
      name = name + 1;
    } while (cVar2 != '\0');
  }
  pcVar1 = (char *)((int)&local_420c + 3);
  do {
    pcVar7 = pcVar1;
    pcVar1 = pcVar7 + 1;
  } while (pcVar7[1] != '\0');
  builtin_strncpy(pcVar7 + 1,"savegame.bin",0xd);
  hFile = CreateFileA(local_4208,0xc0000000,0,(LPSECURITY_ATTRIBUTES)0x0,4,0,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    puVar3 = (undefined1 *)GetFileSize(hFile,(LPDWORD)0x0);
    if (puVar3 != &LAB_00480000) {
      puVar8 = local_4008;
      for (iVar6 = 0x1000; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      BVar4 = WriteFile(hFile,local_4008,0x4000,&local_420c,(LPOVERLAPPED)0x0);
      if ((((BVar4 == 0) || (local_420c != 0x4000)) ||
          (DVar5 = SetFilePointer(hFile,0x480000,(PLONG)0x0,0), DVar5 == 0xffffffff)) ||
         (BVar4 = SetEndOfFile(hFile), BVar4 == 0)) {
        shell_display_fatal_error_dialog(0x8b,0x8c,1);
        cVar2 = FUN_0053d080();
        if (cVar2 != '\0') {
          DeleteFileA(local_4108);
        }
        CloseHandle(hFile);
        return (HANDLE)0xffffffff;
      }
    }
    return hFile;
  }
  return (HANDLE)0xffffffff;
}
#endif
