// saved_game_load_checkpoint_by_name  (Ghidra: saved_game_load_checkpoint_by_name, already named)
// address 0x5391a0, size 238 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/saved_games_functions.md; disassembly of the caller saved_game_load_
// checkpoint (0x539290, 0x5392ee..0x53931a) confirms this function takes a second, hidden
// register parameter (EDI, the bare/unprefixed checkpoint name, e.g. "autosave") that survives
// unmodified from the caller across every intervening call and is later used as the source
// name for the promote-to-savegame copy -- name (the stack parameter Ghidra recognized) is
// always the "checkpoints\\..." -prefixed path used for the actual file I/O.
// game_checkpoint_read_stats_file's (EBX out_difficulty, ...) convention confirmed by this
// call: only the difficulty out-pointer is supplied (game_time/time are NULL), matching
// out/phase4/saved_games_types_notes.md's "0x53827c" reader note. The hand-rolled 9-iteration
// compare loop against "savegame" is rewritten as strcmp, per the precedent in
// src/interface/console_update_display.c.
// register convention: source_name in EDI; name is the recognized stack parameter.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include <string.h>
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4
extern int16_t pending_difficulty; // 0x00696564

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern int16_t game_checkpoint_read_stats_file(int32_t *out_difficulty, char *name,
    int32_t *out_game_time, win32_systemtime *out_time); // 0x538c60
extern void main_queue_map_change(char *map_name); // 0x4c8740, blam-cc: EAX -> map_name
extern uint8_t saved_game_copy_files_to_target(char *source_directory, char *source_name, char *target_name); // 0x5387e0

extern char *campaign_level_paths[10]; // 0x00696574

// blam-cc: stack -> name (cdecl)
// FIXED (verified against 0x5391a0..0x53928d): the only argument is the stack name; EDI is saved and restored,
//   not an input. The stats file reader takes &difficulty in EBX. A difficulty 0..3 becomes pending_difficulty
//   (0x696564, a word). main_queue_map_change gets EAX = campaign_level_paths[level] for a level 0..9,
//   otherwise NULL (the draft passed nothing). When the name is not "savegame" (9-byte compare) the files are
//   copied onto "savegame" (ESI = directory, EDI = name). Returns whether the stats file gave a level.
// Loads the checkpoint "<current profile directory><name>.sav" as the active checkpoint.
uint8_t saved_game_load_checkpoint_by_name(char *name)
{
    char directory[264];
    char path[264];
    win32_find_dataa find_data;
    void *find_handle;
    int32_t difficulty;
    int16_t level;
    char *map_path = 0;

    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(path, "%s%s.sav", directory, name);
    find_handle = FindFirstFileA(path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle == (void *)0xffffffff) {
        return 0;
    }
    FindClose(find_handle);

    level = game_checkpoint_read_stats_file(&difficulty, name, 0, 0);
    if (level == -1) {
        return 0;
    }
    if ((int16_t)difficulty >= 0 && (int16_t)difficulty < 4) {
        pending_difficulty = (int16_t)difficulty;
    }
    if (level >= 0 && level < 10) {
        map_path = campaign_level_paths[level];
    }
    main_queue_map_change(map_path);
    if (memcmp(name, "savegame", 9) != 0) {
        saved_game_copy_files_to_target(directory, name, (char *)"savegame");
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5391a0):

undefined1 saved_game_load_checkpoint_by_name(char *param_1)

{
  short sVar1;
  HANDLE hFindFile;
  int iVar2;
  char *pcVar3;
  bool bVar4;
  short local_344;
  undefined1 local_340 [256];
  char local_240 [256];
  _WIN32_FIND_DATAA local_140;

  FUN_0053d080();
  _sprintf(local_240,"%s%s.sav",local_340,param_1);
  hFindFile = FindFirstFileA(local_240,&local_140);
  if (hFindFile != (HANDLE)0xffffffff) {
    FindClose(hFindFile);
    sVar1 = game_checkpoint_read_stats_file(param_1,0,0);
    if (sVar1 != -1) {
      if ((-1 < local_344) && (local_344 < 4)) {
        DAT_00696564 = local_344;
      }
      main_queue_map_change();
      iVar2 = 9;
      bVar4 = true;
      pcVar3 = "savegame";
      do {
        if (iVar2 == 0) break;
        iVar2 = iVar2 + -1;
        bVar4 = *param_1 == *pcVar3;
        param_1 = param_1 + 1;
        pcVar3 = pcVar3 + 1;
      } while (bVar4);
      if (!bVar4) {
        saved_game_copy_files_to_target("savegame");
      }
      return 1;
    }
  }
  return 0;
}
#endif
