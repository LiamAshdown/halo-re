// game_state_save_thread_proc  (Ghidra: game_state_save_thread_proc, already named)
// address 0x538980, size 308 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x538980 fills in every
// register/stack argument Ghidra dropped: FUN_0053d080's (EAX handle, ESI out buffer)
// convention (handle = 0x00714dd4); game_state_write_persistent_storage(crc_slot /*EAX,
// &write_buffer[0x148]*/, buffer, header_size, total_size); game_checkpoint_write_stats_file
// (scenario_name, difficulty) matches Ghidra's own recovered call. Started by
// game_state_allocate_buffer via __beginthread; loops forever, waiting on
// game_state_write_event, flushing game_state_write_buffer to savegame.bin in 0x4000-byte
// chunks, and -- only for a checkpoint write (game_state_write_is_checkpoint) -- also writing
// the checkpoint stats file and rotating the autosave pair via saved_game_copy_files_to_target.
// register convention: no parameters, no return value (thread entry point).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4
extern void *game_state_write_event; // 0x006e2ffc
extern void *game_state_persistent_storage; // 0x006e2df8
extern uint8_t game_state_write_is_checkpoint; // 0x006e3001
extern uint32_t game_state_size; // 0x006e2df0
extern uint8_t game_state_write_in_progress; // 0x006e3000
extern uint8_t *game_state_write_buffer; // 0x006e2de4
extern uint8_t game_state_write_completed; // 0x006e2df5

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70
extern uint32_t Sleep(uint32_t milliseconds);
extern uint32_t SetFilePointer(void *file, int32_t distance, void *distance_high, uint32_t method); // Win32
extern uint32_t WriteFile(void *file, const void *buffer, uint32_t bytes_to_write, uint32_t *bytes_written, void *overlapped); // Win32
extern uint32_t WaitForSingleObject(void *handle, uint32_t timeout_ms); // Win32
extern void game_state_write_persistent_storage(uint32_t *crc_slot, uint8_t *buffer, int32_t header_size, int32_t total_size); // 0x539710
extern void game_checkpoint_write_stats_file(char *scenario_name, int32_t difficulty); // 0x538b70
extern uint8_t saved_game_copy_files_to_target(char *source_directory, char *source_name, char *target_name); // 0x5387e0

void game_state_save_thread_proc(void)
{
    uint8_t is_checkpoint;
    int32_t remaining;
    uint32_t chunk;
    uint32_t bytes_written;
    char directory[264];

    while (1) {
        WaitForSingleObject(game_state_write_event, 0xffffffff);
        is_checkpoint = game_state_write_is_checkpoint;
        remaining = game_state_size;
        game_state_write_in_progress = 1;
        game_state_write_is_checkpoint = 0;

        if (SetFilePointer(game_state_persistent_storage, 0, 0, 0) != 0xffffffff) {
            while (0 < remaining) {
                chunk = remaining;
                if (0x3fff < remaining) {
                    chunk = 0x4000;
                }
                WriteFile(game_state_persistent_storage, game_state_write_buffer + (game_state_size - remaining),
                    chunk, &bytes_written, 0);
                remaining = remaining - bytes_written;
                Sleep(0);
            }
        }

        if (remaining != 0) {
            shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
            game_state_write_in_progress = 0;
            continue;
        }

        if (is_checkpoint != 0) {
            saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
            game_state_write_persistent_storage((uint32_t *)(game_state_write_buffer + 0x148),
                game_state_write_buffer, k_game_state_header_size, k_game_state_size);
            game_checkpoint_write_stats_file((char *)(game_state_write_buffer + 4),
                *(int16_t *)(game_state_write_buffer + 0x126));
            saved_game_copy_files_to_target(directory, "checkpoints\\autosave", "checkpoints\\autosave1");
            saved_game_copy_files_to_target(directory, "savegame", "checkpoints\\autosave");
        }

        game_state_write_completed = 1;
        game_state_write_in_progress = 0;
    }
}

#if 0
Original Ghidra decompilation (0x538980):

void game_state_save_thread_proc(void)

{
  int iVar1;
  char cVar2;
  DWORD DVar3;
  DWORD DVar4;
  DWORD local_104 [65];

  do {
    while( true ) {
      WaitForSingleObject(DAT_006e2ffc,0xffffffff);
      cVar2 = DAT_006e3001;
      DVar4 = DAT_006e2df0;
      DAT_006e3000 = 1;
      DAT_006e3001 = '\0';
      DVar3 = SetFilePointer(DAT_006e2df8,0,(PLONG)0x0,0);
      if (DVar3 != 0xffffffff) {
        while (0 < (int)DVar4) {
          DVar3 = DVar4;
          if (0x3fff < (int)DVar4) {
            DVar3 = 0x4000;
          }
          WriteFile(DAT_006e2df8,(LPCVOID)((DAT_006e2de4 - DVar4) + DAT_006e2df0),DVar3,local_104,
                    (LPOVERLAPPED)0x0);
          DVar4 = DVar4 - local_104[0];
          Sleep(0);
        }
      }
      if (DVar4 != 0) break;
      if (cVar2 != '\0') {
        FUN_0053d080();
        iVar1 = DAT_006e2de4;
        game_state_write_persistent_storage(DAT_006e2de4,0x14c,0x440000);
        game_checkpoint_write_stats_file(iVar1 + 4,(int)*(short *)(iVar1 + 0x126));
        saved_game_copy_files_to_target("checkpoints\\autosave1");
        saved_game_copy_files_to_target("checkpoints\\autosave");
      }
      DAT_006e2df5 = 1;
      DAT_006e3000 = 0;
    }
    shell_display_fatal_error_dialog(0x8b,0x8c,1);
    DAT_006e3000 = 0;
  } while( true );
}
#endif
