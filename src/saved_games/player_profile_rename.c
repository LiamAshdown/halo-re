// player_profile_rename  (Ghidra: player_profile_rename, already named)
// address 0x53ce80, size 509 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: already named by Ghidra/CEA; out/phase4/saved_games_types_notes.md corrects the
// summary: "renames any saved game, both types" (not profile-only). Confirmed against objdump
// 0x53ce80..0x53d07c: handle is in EAX (register); XCreateSaveGame gets new_name in EAX (the
// wide save-game name, same as saved_game_create_slot / saved_game_allocate_new_slot /
// saved_game_name_is_available); both XDeleteSaveGame calls pass a wide name in EAX with
// ECX = root: the old entry's display name (+0x100, lea eax,[esp+0x310] at 0x53cffc) on the
// success path and new_name on the rollback path (phase-4 review: the first rewrite passed
// entry.path, which is the wrong field). The success path also terminates entry.path[0xff] and
// entry.display_name[0x7f] after the two copies (0x53d02f, 0x53d049).
// player_profile_copy_files is called with the
// truncated old directory in ESI (its source_dir) and the newly-created directory as the stack
// argument (its dest_dir).
// UNSURE: if strstr(an apparent case-sensitive substring search, outside this batch)
// fails to find "blam.sav" inside the resolved entry's own path, the binary skips the file copy
// entirely and falls straight into the success path anyway (renaming the index entry without
// ever having copied any file) -- kept exactly as found, not "fixed".
// register convention: saved-game handle in EAX; new_name as the one stack argument.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern char savegames_directory[0x100]; // 0x00721549

extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry); // 0x53e0e0, FUN_0053e0e0 (src/game)
extern uint8_t savegame_index_write_slot(int32_t slot_index, saved_game_index_entry *entry); // 0x53e1f0, FUN_0053e1f0 (src/game): writes the entry back
extern uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path,
    uint32_t out_path_size); // 0x551710, blam-cc: EAX save_game_name (src/game/XCreateSaveGame.c: validity_token)
extern uint32_t XDeleteSaveGame(const uint16_t *save_game_name, const char *root_path); // 0x5519a0, blam-cc: EAX save_game_name, ECX root_path
extern uint32_t player_profile_copy_files(const char *source_dir, char *dest_dir); // 0x53cb70, this module
extern int32_t CopyFileA(const char *existing_path, const char *new_path, int32_t fail_if_exists); // Win32
extern int32_t __snprintf(char *buffer, uint32_t count, const char *format, ...); // CRT
extern void _strncpy(char *dest, const char *source, uint32_t count); // CRT
extern char *strstr(const char *haystack, const char *needle); // CRT strstr (0x625430: the MSVC asm strstr, haystack then needle; case-sensitive)
extern void _wcsncpy(uint16_t *dest, const uint16_t *source, uint32_t count); // CRT
extern int32_t _wcscmp(const uint16_t *a, const uint16_t *b); // CRT

// blam-cc: saved-game handle in EAX; new_name as the one stack argument
// Renames the saved-game (profile or playlist) identified by handle to new_name: creates a
// fresh XCreateSaveGame slot under that name, copies the old files into it (profile: blam.sav,
// savegame.bin and checkpoints via player_profile_copy_files; playlist: blam.lst via CopyFileA),
// deletes the old slot, and rewrites the index entry's path and display name in place. Returns
// 1 on success or if there was nothing to do (not found, no new name given, or new_name already
// matches), 0 on failure, or (in one code path) the raw playlist CopyFileA result if it is
// neither 0 nor 1.
uint8_t player_profile_rename(int32_t handle, uint16_t *new_name)
{
    uint32_t slot_index;
    saved_game_index_entry entry;
    uint8_t found;
    char directory[0x100];
    int32_t create_result;
    uint8_t result;

    slot_index = ((uint32_t)handle >> 16) & 0xfff;
    found = savegame_index_read_slot((int32_t)slot_index, &entry);
    if (found == 0) {
        return 1;
    }
    if (new_name == 0) {
        return 1;
    }
    if (*new_name == 0) {
        return 1;
    }
    if (_wcscmp(entry.display_name, new_name) == 0) {
        return 1;
    }

    memset(directory, 0, sizeof(directory));
    create_result = XCreateSaveGame(new_name, savegames_directory, 1, directory, 0x100);
    if (create_result != 0) {
        return 1;
    }

    if (entry.type == 0) {
        char dest_path[0x100];
        char old_directory[0x100];
        char *trunc;

        __snprintf(dest_path, 0xff, "%s%s", directory, "blam.sav");
        _strncpy(old_directory, entry.path, 0xff);
        trunc = strstr(old_directory, "blam.sav");
        if (trunc != 0) {
            *trunc = '\0';
            result = (uint8_t)player_profile_copy_files(old_directory, directory);
        } else {
            goto finalize;
        }
        if (result == 1) {
        finalize:
            XDeleteSaveGame(entry.display_name, savegames_directory);
            _strncpy(entry.path, dest_path, 0xff);
            _wcsncpy(entry.display_name, new_name, 0x7f);
            entry.path[0xff] = 0;
            entry.display_name[0x7f] = 0;
            savegame_index_write_slot((int32_t)slot_index, &entry);
            return 1;
        }
        if (result != 0) {
            return result;
        }
    } else {
        if (entry.type != 1) {
            goto rollback;
        }
        {
            char dest_path[0x100];

            __snprintf(dest_path, 0xff, "%s%s", directory, "blam.lst");
            result = (uint8_t)CopyFileA(entry.path, dest_path, 1);
            if (result == 1) {
                XDeleteSaveGame(entry.display_name, savegames_directory);
                _strncpy(entry.path, dest_path, 0xff);
                _wcsncpy(entry.display_name, new_name, 0x7f);
                entry.path[0xff] = 0;
                entry.display_name[0x7f] = 0;
                savegame_index_write_slot((int32_t)slot_index, &entry);
                return 1;
            }
            if (result != 0) {
                return result;
            }
        }
    }

rollback:
    XDeleteSaveGame(new_name, savegames_directory);
    return 0;
}

#if 0
Original Ghidra decompilation (0x53ce80):

char player_profile_rename(wchar_t *param_1)

{
  char cVar1;
  int in_EAX;
  uint uVar2;
  int iVar3;
  BOOL BVar4;
  undefined1 *puVar5;
  uint uVar6;
  undefined4 *puVar7;
  char local_510 [255];
  undefined1 local_411;
  char local_410;
  undefined4 local_40f;
  CHAR local_310 [255];
  undefined1 local_211;
  wchar_t local_210 [127];
  undefined2 local_112;
  short local_110;
  char local_108 [260];

  uVar2 = in_EAX >> 0x10 & 0xfff;
  cVar1 = FUN_0053e0e0(uVar2,local_310);
  if (cVar1 == '\0') {
    return '\x01';
  }
  if (param_1 == (wchar_t *)0x0) {
    return '\x01';
  }
  if (*param_1 == L'\0') {
    return '\x01';
  }
  iVar3 = _wcscmp(local_210,param_1);
  if (iVar3 == 0) {
    return '\x01';
  }
  local_410 = '\0';
  puVar7 = &local_40f;
  for (iVar3 = 0x3f; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  *(undefined2 *)puVar7 = 0;
  *(undefined1 *)((int)puVar7 + 2) = 0;
  iVar3 = XCreateSaveGame(&DAT_00721549,1,&local_410,0x100);
  if (iVar3 != 0) {
    return '\x01';
  }
  if (local_110 == 0) {
    __snprintf(local_510,0xff,"%s%s",&local_410,"blam.sav");
    local_411 = 0;
    _strncpy(local_108,local_310,0xff);
    puVar5 = (undefined1 *)FUN_00625430(local_108,"blam.sav");
    if (puVar5 == (undefined1 *)0x0) goto LAB_0053cffc;
    *puVar5 = 0;
    uVar6 = player_profile_copy_files(&local_410);
    cVar1 = (char)uVar6;
  }
  else {
    if (local_110 != 1) goto LAB_0053d068;
    __snprintf(local_510,0xff,"%s%s",&local_410,"blam.lst");
    local_411 = 0;
    BVar4 = CopyFileA(local_310,local_510,1);
    cVar1 = (char)BVar4;
  }
  if (cVar1 == '\x01') {
LAB_0053cffc:
    XDeleteSaveGame();
    _strncpy(local_310,local_510,0xff);
    local_211 = 0;
    _wcsncpy(local_210,param_1,0x7f);
    local_112 = 0;
    FUN_0053e1f0(uVar2,local_310);
    return '\x01';
  }
  if (cVar1 != '\0') {
    return cVar1;
  }
LAB_0053d068:
  XDeleteSaveGame();
  return '\0';
}
#endif
