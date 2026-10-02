// game_checkpoint_write_stats_file  (Ghidra: game_checkpoint_write_stats_file, already named)
// address 0x538b70, size 232 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md
// "checkpoint_file_entry" evidence: "the disassembly of the writer (0x538bdc..0x538c07)
// pushes tick, difficulty, name, call 0x4c8b90, add esp,4, push result -> fprintf sees level,
// difficulty, tick" -- confirmed here by hand: campaign_level_find_index_for_path's call only pops 4 of its 12
// pushed bytes, leaving difficulty and tick sitting on the stack as the fprintf call's 2nd and
// 3rd varargs (the same stack-reuse trick seen in saved_game_copy_files_to_target), so the
// three-field first line is level (campaign_level_find_index_for_path's return), difficulty, game_time. objdump
// confirms the "wt" fopen mode (0x0065fd30) and the "%d,%d,%d\n" / "%hu,%hu,%hu\n" formats.
// register convention: __cdecl; scenario_name and difficulty are the recognized stack
// parameters (Ghidra's param_1/param_2), matching the caller's (name, difficulty) order.

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
extern game_time_globals *game_time; // 0x006f1d6c
extern char network_summary_log_mode_string[]; // 0x0065fd30, fopen mode "wt"

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern int16_t campaign_level_find_index_for_path(char *scenario_name); // 0x4c8b90, not in this module; one stack argument (add esp,4 at 0x538bdb)

// blam-cc: scenario_name and difficulty are the recognized stack parameters
// Writes <profile directory>savegame.sav, the companion stats file game_checkpoint_read_stats_file
// reads back: a first line of "<level>,<difficulty>,<tick>" (level from campaign_level_find_index_for_path) followed
// by the current local date and time as two more comma-separated lines.
void game_checkpoint_write_stats_file(char *scenario_name, int32_t difficulty)
{
    char directory[264];
    char path[264];
    void *file;
    win32_systemtime now;
    int16_t level;

    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    strcpy(path, directory);
    strcpy(path + strlen(path), "savegame.sav");

    file = fopen(path, network_summary_log_mode_string);
    if (file != 0) {
        GetLocalTime((LPSYSTEMTIME)&now);
        level = campaign_level_find_index_for_path(scenario_name);
        fprintf((FILE *)file, "%d,%d,%d\n", (int32_t)level, difficulty, game_time->game_time);
        fprintf((FILE *)file, "%hu,%hu,%hu\n", now.month, now.day, now.year);
        fprintf((FILE *)file, "%hu,%hu,%hu\n", now.hour, now.minute, now.second);
        fclose((FILE *)file);
    }
}

#if 0
Original Ghidra decompilation (0x538b70):

void game_checkpoint_write_stats_file(undefined4 param_1,undefined4 param_2)

{
  char *pcVar1;
  short sVar2;
  FILE *_File;
  char *pcVar3;
  _SYSTEMTIME local_110;
  char local_100 [12];
  char local_f4 [244];

  FUN_0053d080();
  pcVar1 = (char *)((int)&local_110.wMilliseconds + 1);
  do {
    pcVar3 = pcVar1;
    pcVar1 = pcVar3 + 1;
  } while (pcVar3[1] != '\0');
  builtin_strncpy(pcVar3 + 1,"savegame.sav",0xd);
  _File = (FILE *)FUN_00624186(local_100,&DAT_0065fd30);
  if (_File != (FILE *)0x0) {
    GetLocalTime(&local_110);
    sVar2 = FUN_004c8b90(param_1,param_2,*(undefined4 *)(DAT_006f1d6c + 0xc));
    _fprintf(_File,"%d,%d,%d\n",(int)sVar2);
    _fprintf(_File,"%hu,%hu,%hu\n",(uint)local_110.wMonth,(uint)local_110.wDay,(uint)local_110.wYear
            );
    _fprintf(_File,"%hu,%hu,%hu\n",(uint)local_110.wHour,(uint)local_110.wMinute,
             (uint)local_110.wSecond);
    _fclose(_File);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
