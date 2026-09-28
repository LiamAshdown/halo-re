// saved_game_files_dispose  (Ghidra: saved_game_files_dispose, already named)
// address 0x53c480, size 89 bytes
// name confidence: 0.9   rewrite confidence: 0.7
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_functions.md summary "Shuts down
// the saved-games module: closes its synchronization handles, waits for any background worker
// thread, and clears the initialized flag." types/networking.h network_mutex_record fields
// (handle 0x00, name 0x04, in_use 0x24) match the three separate writes Ghidra shows per mutex
// (offset 4 = name[0], offset 0 = handle, offset 0x24 = in_use); name[0] is cleared but the rest
// of the mutex's name string is left as-is, matching the binary exactly.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern network_mutex_record *saved_game_files_mutex; // 0x0072143c
extern network_mutex_record *savegame_index_mutex; // 0x00721440
extern uint8_t saved_game_files_initialized; // 0x00721446

extern int32_t __stdcall CloseHandle(void *object); // Win32
extern void player_profile_verify_thread_wait_and_clear(void); // 0x539a40, outside this batch
extern void control_profile_variant_write_wait_and_clear(void); // 0x53bae0, this module

// blam-cc: __cdecl, no parameters
// Closes and clears each of the two module mutexes (if created), waits for and clears the
// background profile-verification and game-variant-write threads, and clears
// saved_game_files_initialized.
void saved_game_files_dispose(void)
{
    network_mutex_record *mutex;

    mutex = saved_game_files_mutex;
    if (saved_game_files_mutex != 0) {
        CloseHandle(mutex->handle);
        mutex->name[0] = 0;
        mutex->handle = 0;
        mutex->in_use = 0;
        saved_game_files_mutex = 0;
    }
    mutex = savegame_index_mutex;
    if (savegame_index_mutex != 0) {
        CloseHandle(mutex->handle);
        mutex->name[0] = 0;
        mutex->handle = 0;
        mutex->in_use = 0;
        savegame_index_mutex = 0;
    }
    player_profile_verify_thread_wait_and_clear();
    control_profile_variant_write_wait_and_clear();
    saved_game_files_initialized = 0;
}

#if 0
Original Ghidra decompilation (0x53c480):

void __cdecl saved_game_files_dispose(void)

{
  undefined4 *puVar1;

  puVar1 = DAT_0072143c;
  if (DAT_0072143c != (undefined4 *)0x0) {
    CloseHandle((HANDLE)*DAT_0072143c);
    *(undefined1 *)(puVar1 + 1) = 0;
    *puVar1 = 0;
    *(undefined1 *)(puVar1 + 9) = 0;
    DAT_0072143c = (undefined4 *)0x0;
  }
  puVar1 = DAT_00721440;
  if (DAT_00721440 != (undefined4 *)0x0) {
    CloseHandle((HANDLE)*DAT_00721440);
    *(undefined1 *)(puVar1 + 1) = 0;
    *puVar1 = 0;
    *(undefined1 *)(puVar1 + 9) = 0;
    DAT_00721440 = (undefined4 *)0x0;
  }
  FUN_00539a40();
  FUN_0053bae0();
  DAT_00721446 = 0;
  return;
}
#endif
