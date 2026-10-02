// game_checkpoint_save_new  (Ghidra: game_checkpoint_save_new, already named)
// address 0x538db0, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/saved_games_functions.md; waits for any in-progress write, resolves the
// current profile's directory, finds a free checkpoint slot name, then copies the "savegame"
// .bin/.sav pair onto it via saved_game_copy_files_to_target (source_directory in ESI,
// source_name in EDI, target_name on the stack -- matching that function's own established
// convention).
// register convention: no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint8_t game_state_write_in_progress; // 0x006e3000
extern int32_t saved_player_profile_slots_handle; // 0x00714dd4

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern uint8_t game_checkpoint_get_next_filename(char *out_name, char *directory); // 0x538ae0
extern uint8_t saved_game_copy_files_to_target(char *source_directory, char *source_name, char *target_name); // 0x5387e0

uint8_t game_checkpoint_save_new(void)
{
    char directory[264];
    char target_name[256];

    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    if (game_checkpoint_get_next_filename(target_name, directory) == 0) {
        return 0;
    }
    return saved_game_copy_files_to_target(directory, (char *)"savegame", target_name);
}

#if 0
Original Ghidra decompilation (0x538db0):

uint game_checkpoint_save_new(void)

{
  uint uVar1;
  undefined1 local_120 [32];
  undefined1 local_100 [256];

  if (DAT_006e3000 != '\0') {
    while (DAT_006e3000 != '\0') {
      Sleep(0);
    }
  }
  FUN_0053d080();
  uVar1 = game_checkpoint_get_next_filename(local_100);
  if ((char)uVar1 == '\0') {
    return uVar1 & 0xffffff00;
  }
  uVar1 = saved_game_copy_files_to_target(local_120);
  return uVar1;
}
#endif
