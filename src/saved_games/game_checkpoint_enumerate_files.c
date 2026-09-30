// game_checkpoint_enumerate_files  (Ghidra: game_checkpoint_enumerate_files, already named)
// address 0x538e70, size 671 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// checkpoint_file_entry field table (established by this function) and its "0x48 checkpoint
// entry" / qsort-width evidence; GlobalAlloc(0x1cb0) == k_maximum_checkpoint_files *
// sizeof(checkpoint_file_entry) exactly. All four stack parameters (include_autosaves,
// sort_newest_first, callback, user_data) are Ghidra-recognized and match
// checkpoint_enumerate_proc plus the checkpoint_sort_newest_first global. The hand-rolled
// path/name-building and extension-stripping loops are rewritten as sprintf/strrchr/strcpy.
// strstr is called here as strstr("checkpoints\\<name>", "autosave") and compared
// only to zero/non-zero: a whole-string compare (src/game/cheat_spawn_warthog.c's precedent
// calls it "CRT strcmp-shaped") could never be zero given the "checkpoints\\" prefix, which
// would make the autosave filter a no-op, so here it must be a substring search; treated as
// strstr-shaped (extern, not renamed: not in this module) and used again, with a
// pointer-equals-haystack-plus-length check, to reproduce the original's two fixed-length
// exact-match loops against "autosave" / "autosave1".
// Phase 4 review: matched objdump 0x538e70..0x53910e; the callback gets the level zero-extended
// (xor edx,edx / mov dx at 0x5390de), now cast the same way.
// register convention: __cdecl; include_autosaves, sort_newest_first, callback and user_data
// are the recognized stack parameters (Ghidra's own param_1..param_4).

#include "crt.h"
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
extern uint8_t checkpoint_sort_newest_first; // 0x0069e7e8

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL


int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first,
    checkpoint_enumerate_proc callback, void *user_data)
{
    checkpoint_file_entry *entries;
    checkpoint_file_entry *entry;
    char directory[264];
    char search_path[264];
    win32_find_dataa find_data;
    void *find_handle;
    uint32_t found_count;
    char name[264];
    char *extension;
    char *basename;
    int32_t difficulty;
    int32_t game_time_ticks;
    win32_systemtime time;
    int16_t level;
    int32_t accepted;
    uint32_t remaining;

    entries = (checkpoint_file_entry *)GlobalAlloc(0, k_maximum_checkpoint_files * sizeof(checkpoint_file_entry));
    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(search_path, "%s%s", directory, "checkpoints\\*.sav");

    found_count = 0;
    find_handle = FindFirstFileA(search_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)0xffffffff) {
        entry = entries;
        do {
            sprintf(name, "%s%s", "checkpoints\\", find_data.cFileName);
            extension = strchr(name, '.');
            if (extension != 0) {
                *extension = 0;
            }

            level = game_checkpoint_read_stats_file(&difficulty, name, &game_time_ticks, &time);
            if (level != -1) {
                if (strstr(name, "autosave") == 0 || include_autosaves != 0) {
                    entry->level_index = level;
                    entry->game_time = game_time_ticks;
                    entry->difficulty = difficulty;
                    entry->time = time;
                    entry->last_write_time[0] = find_data.ftLastWriteTime[0];
                    entry->last_write_time[1] = find_data.ftLastWriteTime[1];
                    entry->kind = _checkpoint_kind_checkpoint;

                    basename = strchr(name, '\\') + 1;
                    sprintf(entry->name, "%s", basename);
                    if (strstr(basename, "autosave") == basename && basename[8] == 0) {
                        entry->kind = _checkpoint_kind_autosave;
                    } else if (strstr(basename, "autosave1") == basename && basename[9] == 0) {
                        entry->kind = _checkpoint_kind_autosave1;
                    }

                    found_count = found_count + 1;
                    entry = entry + 1;
                }
            }
        } while (FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data) != 0 && found_count < k_maximum_checkpoint_files);
        FindClose(find_handle);
    }

    checkpoint_sort_newest_first = sort_newest_first;
    qsort(entries, found_count, sizeof(checkpoint_file_entry),
        (int32_t (*)(const void *, const void *))saved_game_checkpoint_compare);

    accepted = 0;
    remaining = found_count;
    entry = entries;
    if (0 < (int32_t)remaining) {
        do {
            if (callback == 0) {
                accepted = accepted + 1;
            } else if (callback(accepted, entry->name, (int32_t)(uint16_t)entry->level_index, entry->difficulty,
                           entry->game_time, &entry->time, user_data) != 0) {
                accepted = accepted + 1;
            }
            entry = entry + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    GlobalFree(entries);
    return accepted;
}

#if 0
Original Ghidra decompilation (0x538e70):

int game_checkpoint_enumerate_files
              (char param_1,undefined1 param_2,code *param_3,undefined4 param_4)

{
  CHAR *pCVar1;
  char cVar2;
  short sVar3;
  HGLOBAL _Base;
  HANDLE hFindFile;
  CHAR *pCVar4;
  undefined1 *puVar5;
  int iVar6;
  char *pcVar7;
  BOOL BVar8;
  uint uVar9;
  char *pcVar10;
  size_t _NumOfElements;
  undefined4 *puVar11;
  CHAR *pCVar12;
  CHAR *pCVar13;
  char *pcVar14;
  bool bVar15;
  size_t local_364;
  undefined4 local_358;
  undefined4 local_354;
  undefined4 local_350;
  undefined4 local_34c;
  undefined4 local_348;
  undefined4 local_344;
  char local_340 [256];
  _WIN32_FIND_DATAA local_240;
  CHAR local_100 [256];

  local_364 = 0;
  _Base = GlobalAlloc(0,0x1cb0);
  FUN_0053d080();
  pcVar10 = &local_240.field_0x13f;
  do {
    pcVar7 = pcVar10;
    pcVar10 = pcVar7 + 1;
  } while (pcVar7[1] != '\0');
  builtin_strncpy(pcVar7 + 1,"checkpoints\\*.sav",0x12);
  hFindFile = FindFirstFileA(local_100,&local_240);
  _NumOfElements = 0;
  if (hFindFile != (HANDLE)0xffffffff) {
    puVar11 = (undefined4 *)((int)_Base + 4);
    do {
      builtin_strncpy(local_340,"checkpoi",8);
      builtin_strncpy(local_340 + 8,"nts\\",5);
      pCVar1 = local_240.cFileName;
      pCVar4 = pCVar1;
      do {
        cVar2 = *pCVar4;
        pCVar4 = pCVar4 + 1;
      } while (cVar2 != '\0');
      pCVar13 = (CHAR *)((int)&local_344 + 3);
      do {
        pcVar10 = pCVar13 + 1;
        pCVar13 = pCVar13 + 1;
      } while (*pcVar10 != '\0');
      pCVar12 = pCVar1;
      for (uVar9 = (uint)((int)pCVar4 - (int)pCVar1) >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
        *(undefined4 *)pCVar13 = *(undefined4 *)pCVar12;
        pCVar12 = pCVar12 + 4;
        pCVar13 = pCVar13 + 4;
      }
      for (uVar9 = (int)pCVar4 - (int)pCVar1 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
        *pCVar13 = *pCVar12;
        pCVar12 = pCVar12 + 1;
        pCVar13 = pCVar13 + 1;
      }
      puVar5 = (undefined1 *)FUN_006257e0(local_340,0x2e);
      if (puVar5 != (undefined1 *)0x0) {
        *puVar5 = 0;
      }
      sVar3 = game_checkpoint_read_stats_file(local_340,&local_358,&local_350);
      if (sVar3 != -1) {
        iVar6 = FUN_00625430(local_340,"autosave");
        if ((iVar6 == 0) || (param_1 != '\0')) {
          puVar11[1] = local_354;
          *puVar11 = local_358;
          puVar11[2] = local_350;
          puVar11[3] = local_34c;
          puVar11[4] = local_348;
          puVar11[5] = local_344;
          *(short *)(puVar11 + -1) = sVar3;
          puVar11[8] = 0;
          puVar11[6] = local_240.ftLastWriteTime.dwLowDateTime;
          puVar11[7] = local_240.ftLastWriteTime.dwHighDateTime;
          iVar6 = FUN_006257e0(local_340,0x5c);
          pcVar7 = (char *)(iVar6 + 1);
          pcVar10 = pcVar7;
          do {
            cVar2 = *pcVar10;
            pcVar10[(int)puVar11 + (0x24 - (int)pcVar7)] = cVar2;
            pcVar10 = pcVar10 + 1;
          } while (cVar2 != '\0');
          iVar6 = 9;
          bVar15 = true;
          pcVar10 = pcVar7;
          pcVar14 = "autosave";
          do {
            if (iVar6 == 0) break;
            iVar6 = iVar6 + -1;
            bVar15 = *pcVar10 == *pcVar14;
            pcVar10 = pcVar10 + 1;
            pcVar14 = pcVar14 + 1;
          } while (bVar15);
          if (bVar15) {
            puVar11[8] = 2;
          }
          else {
            iVar6 = 10;
            bVar15 = true;
            pcVar10 = "autosave1";
            do {
              if (iVar6 == 0) break;
              iVar6 = iVar6 + -1;
              bVar15 = *pcVar7 == *pcVar10;
              pcVar7 = pcVar7 + 1;
              pcVar10 = pcVar10 + 1;
            } while (bVar15);
            if (bVar15) {
              puVar11[8] = 1;
            }
          }
          local_364 = local_364 + 1;
          puVar11 = puVar11 + 0x12;
        }
      }
      BVar8 = FindNextFileA(hFindFile,&local_240);
    } while ((BVar8 != 0) && ((int)local_364 < 0x66));
    FindClose(hFindFile);
    _NumOfElements = local_364;
  }
  DAT_0069e7e8 = param_2;
  _qsort(_Base,_NumOfElements,0x48,saved_game_checkpoint_compare);
  iVar6 = 0;
  if (0 < (int)_NumOfElements) {
    puVar11 = (undefined4 *)((int)_Base + 4);
    do {
      if (param_3 == (code *)0x0) {
LAB_005390f4:
        iVar6 = iVar6 + 1;
      }
      else {
        cVar2 = (*param_3)(iVar6,puVar11 + 9,*(undefined2 *)(puVar11 + -1),puVar11[1],*puVar11,
                           puVar11 + 2,param_4);
        if (cVar2 != '\0') goto LAB_005390f4;
      }
      puVar11 = puVar11 + 0x12;
      _NumOfElements = _NumOfElements - 1;
    } while (_NumOfElements != 0);
  }
  GlobalFree(_Base);
  return iVar6;
}
#endif
