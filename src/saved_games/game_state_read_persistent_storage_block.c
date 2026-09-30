// game_state_read_persistent_storage_block  (Ghidra: game_state_read_persistent_storage_block, already named)
// address 0x539850, size 141 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md
// register-convention note: "game_state_read_persistent_storage_block EAX size"; buffer is
// Ghidra's own recognized stack parameter. Same open/seek/read/fatal-error-and-delete pattern
// as game_state_write_persistent_storage and game_state_read_persistent_storage.
// register convention: size in EAX, buffer is the recognized stack parameter.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4

extern void *game_state_open_persistent_storage(char *name); // 0x5398e0
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

// blam-cc: size in EAX, then the recognized stack parameter (buffer)
// Opens the current profile's savegame.bin and reads size bytes from its start into buffer.
// On any failure, raises a fatal error dialog and attempts to delete the current profile's
// save file.
void game_state_read_persistent_storage_block(int32_t size, void *buffer)
{
    void *file;
    uint32_t bytes_read;
    char directory[264];

    file = game_state_open_persistent_storage(0);
    if (file == (void *)0xffffffff) {
        return;
    }

    if (SetFilePointer(file, 0, 0, 0) == 0xffffffff ||
        ReadFile(file, buffer, size, &bytes_read, 0) == 0 ||
        bytes_read != (uint32_t)size) {
        shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
        if (saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory) != 0) {
            DeleteFileA(directory);
        }
    }
    CloseHandle(file);
}

#if 0
Original Ghidra decompilation (0x539850):

void game_state_read_persistent_storage_block(LPVOID param_1)

{
  char cVar1;
  DWORD in_EAX;
  HANDLE hFile;
  DWORD DVar2;
  BOOL BVar3;
  DWORD local_104;
  CHAR local_100 [256];

  hFile = game_state_open_persistent_storage((char *)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    DVar2 = SetFilePointer(hFile,0,(PLONG)0x0,0);
    if (((DVar2 == 0xffffffff) ||
        (BVar3 = ReadFile(hFile,param_1,in_EAX,&local_104,(LPOVERLAPPED)0x0), BVar3 == 0)) ||
       (local_104 != in_EAX)) {
      shell_display_fatal_error_dialog(0x8b,0x8c,1);
      cVar1 = FUN_0053d080();
      if (cVar1 != '\0') {
        DeleteFileA(local_100);
      }
    }
    CloseHandle(hFile);
    return;
  }
  return;
}
#endif
