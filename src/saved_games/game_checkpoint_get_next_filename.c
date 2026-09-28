// game_checkpoint_get_next_filename  (Ghidra: game_checkpoint_get_next_filename, already named)
// address 0x538ae0, size 141 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; strings "checkpoints\\checkpoint%d" and
// "%s%s.sav" match probing checkpoint0..checkpoint99 in the given directory; on exhaustion it
// falls back to game_checkpoint_enumerate_files with the reclaim callback
// (game_checkpoint_reclaim_slot_callback, 0x538ac0) to steal the oldest/lowest-priority slot's
// name instead. ESI (never assigned in-function) is the out-buffer register, matching the
// module's file-helper convention of an ESI/EDI output-buffer register plus a directory stack
// parameter.
// register convention: out_name in ESI; directory is the recognized stack parameter.
// Note: the enumerate_files call below shows only 3 of its 4 arguments in Ghidra's own
// decompilation (game_checkpoint_enumerate_files(0,0,&LAB_00538ac0)); the 4th (user_data) is
// supplied here as out_name, which is the only reading consistent with
// out/phase4/saved_games_types_notes.md's description of the callback ("copies the first name
// into user_data") and with this function's own use of *out_name as its success flag
// afterward. Confirmed in objdump: 0x538b47 pushes ESI (out_name) as the 4th argument.

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
extern int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first,
    checkpoint_enumerate_proc callback, void *user_data); // 0x538e70
extern uint8_t game_checkpoint_reclaim_slot_callback(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data); // 0x538ac0

// blam-cc: out_name in ESI, then the recognized stack parameter (directory)
// Finds the first unused "checkpoints\checkpointN" (N = 0..99) slot under directory and writes
// it to out_name. If all 100 slots are taken, reclaims one instead via
// game_checkpoint_enumerate_files (oldest/lowest-priority entry first, autosaves excluded).
uint8_t game_checkpoint_get_next_filename(char *out_name, char *directory)
{
    int32_t index;
    char path[256];
    win32_find_dataa find_data;
    void *find_handle;

    for (index = 0; index <= 99; index = index + 1) {
        _sprintf(out_name, "checkpoints\\checkpoint%d", index);
        _sprintf(path, "%s%s.sav", directory, out_name);
        find_handle = FindFirstFileA(path, &find_data);
        if (find_handle == (void *)0xffffffff) {
            return 1;
        }
        FindClose(find_handle);
    }

    *out_name = 0;
    game_checkpoint_enumerate_files(0, 0, game_checkpoint_reclaim_slot_callback, out_name);
    return *out_name != 0;
}

#if 0
Original Ghidra decompilation (0x538ae0):

bool game_checkpoint_get_next_filename(undefined4 param_1)

{
  HANDLE hFindFile;
  char *unaff_ESI;
  int iVar1;
  char local_240 [256];
  _WIN32_FIND_DATAA local_140;

  iVar1 = 0;
  while( true ) {
    if (99 < iVar1) {
      *unaff_ESI = '\0';
      game_checkpoint_enumerate_files(0,0,&LAB_00538ac0);
      return *unaff_ESI != '\0';
    }
    _sprintf(unaff_ESI,"checkpoints\\checkpoint%d",iVar1);
    _sprintf(local_240,"%s%s.sav",param_1);
    hFindFile = FindFirstFileA(local_240,&local_140);
    if (hFindFile == (HANDLE)0xffffffff) break;
    FindClose(hFindFile);
    iVar1 = iVar1 + 1;
  }
  return true;
}
#endif
