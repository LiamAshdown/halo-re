// game_checkpoint_read_stats_file  (Ghidra: game_checkpoint_read_stats_file, already named)
// address 0x538c60, size 335 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// "checkpoint_file_entry" evidence table (reader call at 0x538f6c..0x538f7f: "EBX = difficulty
// out, second arg = tick out, third = SYSTEMTIME"). Field order (level,difficulty,tick) mirrors
// game_checkpoint_write_stats_file exactly. objdump confirms the "rt" fopen mode (0x0066d81c).
// The hand-rolled path-append loop is rewritten as strcpy+strlen, matching this file's sibling
// writer.
// register convention: out_difficulty in EBX (per the module's register-convention note,
// checked against the caller game_checkpoint_enumerate_files); name, out_game_time and out_time
// are the recognized stack parameters, in that order.

#include "crt.h"
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
extern char network_ban_file_read_mode_string[]; // 0x0066d81c, fopen mode "rt"

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL

// blam-cc: out_difficulty in EBX, then the recognized stack parameters (name, out_game_time, out_time)
// Reads back "checkpoints\<name>.sav" as written by game_checkpoint_write_stats_file: a first
// line "<level>,<difficulty>,<tick>" and two more lines of local date and time. Any out pointer
// may be NULL. Returns the level index (as an unsigned 16-bit value; -1/0xffff if the file
// could not be opened or scanned).
int16_t game_checkpoint_read_stats_file(int32_t *out_difficulty, char *name, int32_t *out_game_time,
    win32_systemtime *out_time)
{
    char directory[264];
    char path[264];
    void *file;
    int32_t level;
    int32_t difficulty;
    int32_t game_time_ticks;
    win32_systemtime time;

    level = -1;
    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    strcpy(path, directory);
    strcpy(path + strlen(path), name);
    strcpy(path + strlen(path), ".sav");

    file = fopen(path, network_ban_file_read_mode_string);
    if (file != 0) {
        time.year = 0; time.month = 0; time.day_of_week = 0; time.day = 0;
        time.hour = 0; time.minute = 0; time.second = 0; time.milliseconds = 0;

        fscanf((FILE *)file, "%d,%d,%d", &level, &difficulty, &game_time_ticks);
        fscanf((FILE *)file, "%hu,%hu,%hu\n", &time.month, &time.day, &time.year);
        fscanf((FILE *)file, "%hu,%hu,%hu\n", &time.hour, &time.minute, &time.second);
        fclose((FILE *)file);

        if (out_difficulty != 0) {
            *out_difficulty = difficulty;
        }
        if (out_game_time != 0) {
            *out_game_time = game_time_ticks;
        }
        if (out_time != 0) {
            *out_time = time;
        }
    }
    return (int16_t)(level & 0xffff);
}

#if 0
Original Ghidra decompilation (0x538c60):

uint game_checkpoint_read_stats_file(char *param_1,undefined4 *param_2,undefined4 *param_3)

{
  char cVar1;
  undefined4 *puVar2;
  char *pcVar3;
  FILE *_File;
  uint uVar4;
  undefined4 *unaff_EBX;
  char *pcVar5;
  char *pcVar6;
  undefined4 *puVar7;
  uint local_11c;
  undefined2 local_118;
  undefined2 uStack_116;
  undefined2 uStack_114;
  undefined2 uStack_112;
  undefined2 uStack_110;
  undefined2 local_10e;
  undefined2 uStack_10c;
  undefined2 local_10a;
  undefined4 local_108;
  undefined1 local_104 [4];
  char local_100 [4];
  undefined1 local_fc [252];

  local_11c = 0xffffffff;
  FUN_0053d080();
  pcVar3 = param_1;
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  pcVar6 = local_104 + 3;
  do {
    pcVar5 = pcVar6 + 1;
    pcVar6 = pcVar6 + 1;
  } while (*pcVar5 != '\0');
  pcVar5 = param_1;
  for (uVar4 = (uint)((int)pcVar3 - (int)param_1) >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar6 = *(undefined4 *)pcVar5;
    pcVar5 = pcVar5 + 4;
    pcVar6 = pcVar6 + 4;
  }
  for (uVar4 = (int)pcVar3 - (int)param_1 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar6 = *pcVar5;
    pcVar5 = pcVar5 + 1;
    pcVar6 = pcVar6 + 1;
  }
  puVar2 = (undefined4 *)(local_104 + 3);
  do {
    puVar7 = puVar2;
    puVar2 = (undefined4 *)((int)puVar7 + 1);
  } while (*(char *)((int)puVar7 + 1) != '\0');
  *(undefined4 *)((int)puVar7 + 1) = 0x7661732e;
  *(undefined1 *)((int)puVar7 + 5) = 0;
  _File = (FILE *)FUN_00624186(local_100,&DAT_0066d81c);
  if (_File != (FILE *)0x0) {
    uStack_116 = 0;
    uStack_114 = 0;
    uStack_112 = 0;
    uStack_110 = 0;
    local_10e = 0;
    uStack_10c = 0;
    local_10a = 0;
    local_118 = 0;
    _fscanf(_File,"%d,%d,%d",&local_11c,local_104,&local_108);
    _fscanf(_File,"%hu,%hu,%hu\n",&uStack_116,&uStack_112,&local_118);
    _fscanf(_File,"%hu,%hu,%hu\n",&uStack_110,&local_10e,&uStack_10c);
    _fclose(_File);
    if (unaff_EBX != (undefined4 *)0x0) {
      *unaff_EBX = local_104;
    }
    if (param_2 != (undefined4 *)0x0) {
      *param_2 = local_108;
    }
    if (param_3 != (undefined4 *)0x0) {
      *param_3 = CONCAT22(uStack_116,local_118);
      param_3[1] = CONCAT22(uStack_112,uStack_114);
      param_3[2] = CONCAT22(local_10e,uStack_110);
      param_3[3] = CONCAT22(local_10a,uStack_10c);
    }
  }
  return local_11c & 0xffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
