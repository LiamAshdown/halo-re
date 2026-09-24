// player_profile_copy_files  (Ghidra: player_profile_copy_files, already named)
// address 0x53cb70, size 769 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_functions.md summary "Copies a
// player profile's blam.sav, savegame.bin, and all checkpoint save files into a new destination
// directory." Ghidra's decompiled body drops a parameter: two of the __snprintf calls it shows
// with only 3 of their 5 real arguments (`__snprintf(local_340,0xff,"%s%s")`) actually push a
// second format argument from ESI, confirmed by objdump 0x53cb70..0x53ce7c never assigning ESI
// before its first use -- it is a genuine second incoming register argument (the source
// directory), separate from the stack-passed destination directory (param_1/dest_dir). Every
// "source" path (the copy's existing-file argument, and the checkpoint FindFirstFileA
// enumeration root) is built from ESI; every "destination" path from the stack argument.
// After copying savegame.bin the binary truncates both paths at their last '.' and appends
// ".sav" (0x00670a24), then copies "<dir>savegame.sav" as well; kept as found.
// Phase 4 review (objdump 0x53cb70..0x53ce7c, line by line) fixed three drifts in the first
// rewrite: (1) the return value is the savegame.bin copy result saved at [esp+0xb] (0x53cc45)
// and returned at 0x53ce6f on every path past that copy, so a failed savegame.sav or
// checkpoint copy never changes it; (2) the savegame.sav copy result (BL) gates both checkpoint
// phases, and a failed checkpoints\*.sav copy leaves BL = 0, which skips the whole
// checkpoints\*.bin phase (0x53cdb9); (3) both path buffers get [0xff] = 0 after each
// blam.sav / savegame.bin _snprintf pair (0x53cbce, 0x53cc31), which matters because MSVC
// _snprintf does not terminate a truncated result.
// register convention: source directory in ESI; destination directory as the one stack
// argument.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t CopyFileA(const char *existing_path, const char *new_path, int32_t fail_if_exists); // Win32
extern void *FindFirstFileA(const char *path, win32_find_dataa *out_data); // Win32
extern int32_t FindNextFileA(void *handle, win32_find_dataa *out_data); // Win32
extern int32_t FindClose(void *handle); // Win32
extern int32_t __snprintf(char *buffer, uint32_t count, const char *format, ...); // CRT
extern char *_strrchr(char *s, int32_t c); // CRT

// blam-cc: source directory in ESI; destination directory as the one stack argument
// Copies blam.sav, savegame.bin (and, alongside it, a same-stem "*.sav" file -- see the header
// note above) and every checkpoints\*.sav and checkpoints\*.bin file from source_dir into
// dest_dir. Stops at the first failed copy; a failed savegame.sav copy skips both checkpoint
// phases and a failed checkpoints\*.sav copy skips the *.bin phase. Returns the blam.sav /
// savegame.bin copy result (0 if either failed), never the later ones.
uint32_t player_profile_copy_files(const char *source_dir, char *dest_dir)
{
    char dest_path[0x100];
    char source_path[0x100];
    char search_path[0x100];
    uint8_t result;
    uint8_t copy_ok;
    char *dot;
    void *find_handle;
    win32_find_dataa find_data;

    __snprintf(dest_path, 0xff, "%s%s", dest_dir, "blam.sav");
    __snprintf(source_path, 0xff, "%s%s", source_dir, "blam.sav");
    dest_path[0xff] = '\0';
    source_path[0xff] = '\0';
    result = (uint8_t)CopyFileA(source_path, dest_path, 0);
    if (result == 0) {
        return 0;
    }

    __snprintf(dest_path, 0xff, "%s%s", dest_dir, "savegame.bin");
    __snprintf(source_path, 0xff, "%s%s", source_dir, "savegame.bin");
    dest_path[0xff] = '\0';
    source_path[0xff] = '\0';
    result = (uint8_t)CopyFileA(source_path, dest_path, 0);
    if (result == 0) {
        return 0;
    }

    dot = _strrchr(dest_path, '.');
    if (dot != 0) {
        *dot = '\0';
    }
    strcat(dest_path, ".sav");
    dot = _strrchr(source_path, '.');
    if (dot != 0) {
        *dot = '\0';
    }
    strcat(source_path, ".sav");
    copy_ok = (uint8_t)CopyFileA(source_path, dest_path, 0);
    if (copy_ok == 0) {
        return result;
    }

    __snprintf(search_path, 0xff, "%scheckpoints\\*.sav", source_dir);
    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle != (void *)-1) {
        do {
            __snprintf(dest_path, 0xff, "%scheckpoints\\%s", dest_dir, find_data.cFileName);
            __snprintf(source_path, 0xff, "%scheckpoints\\%s", source_dir, find_data.cFileName);
            copy_ok = (uint8_t)CopyFileA(source_path, dest_path, 0);
            if (copy_ok == 0) {
                break;
            }
        } while (FindNextFileA(find_handle, &find_data) != 0);
        FindClose(find_handle);
    }
    if (copy_ok == 0) {
        return result;
    }

    __snprintf(search_path, 0xff, "%scheckpoints\\*.bin", source_dir);
    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle != (void *)-1) {
        do {
            __snprintf(dest_path, 0xff, "%scheckpoints\\%s", dest_dir, find_data.cFileName);
            __snprintf(source_path, 0xff, "%scheckpoints\\%s", source_dir, find_data.cFileName);
            if ((uint8_t)CopyFileA(source_path, dest_path, 0) == 0) {
                break;
            }
        } while (FindNextFileA(find_handle, &find_data) != 0);
        FindClose(find_handle);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x53cb70):

uint __cdecl player_profile_copy_files(char *param_1)

{
  undefined4 *puVar1;
  uint uVar2;
  char *pcVar3;
  HANDLE hFindFile;
  BOOL BVar4;
  BOOL BVar5;
  char cVar6;
  undefined4 *puVar7;
  undefined1 local_444 [4];
  char local_440 [4];
  undefined1 local_43c [251];
  undefined1 local_341;
  char local_340 [4];
  undefined1 local_33c [251];
  undefined1 local_241;
  _WIN32_FIND_DATAA local_240;
  char local_100 [256];

  __snprintf(local_440,0xff,"%s%s",param_1,"blam.sav");
  __snprintf(local_340,0xff,"%s%s");
  local_341 = 0;
  local_241 = 0;
  uVar2 = CopyFileA(local_340,local_440,0);
  if ((char)uVar2 != '\0') {
    __snprintf(local_440,0xff,"%s%s",param_1,"savegame.bin");
    __snprintf(local_340,0xff,"%s%s");
    local_341 = 0;
    local_241 = 0;
    uVar2 = CopyFileA(local_340,local_440,0);
    if ((char)uVar2 != '\0') {
      pcVar3 = _strrchr(local_440,0x2e);
      if (pcVar3 != (char *)0x0) {
        *pcVar3 = '\0';
      }
      puVar1 = (undefined4 *)(local_444 + 3);
      do {
        puVar7 = puVar1;
        puVar1 = (undefined4 *)((int)puVar7 + 1);
      } while (*(char *)((int)puVar7 + 1) != '\0');
      *(undefined4 *)((int)puVar7 + 1) = 0x7661732e;
      *(undefined1 *)((int)puVar7 + 5) = 0;
      pcVar3 = _strrchr(local_340,0x2e);
      if (pcVar3 != (char *)0x0) {
        *pcVar3 = '\0';
      }
      puVar1 = (undefined4 *)&local_341;
      do {
        puVar7 = puVar1;
        puVar1 = (undefined4 *)((int)puVar7 + 1);
      } while (*(char *)((int)puVar7 + 1) != '\0');
      *(undefined4 *)((int)puVar7 + 1) = 0x7661732e;
      *(undefined1 *)((int)puVar7 + 5) = 0;
      hFindFile = (HANDLE)CopyFileA(local_340,local_440,0);
      cVar6 = (char)hFindFile;
      if (cVar6 != '\0') {
        __snprintf(local_100,0xff,"%scheckpoints\\*.sav");
        local_444 = (undefined1  [4])FindFirstFileA(local_100,&local_240);
        if (local_444 == (undefined1  [4])0xffffffff) {
          hFindFile = (HANDLE)0xffffffff;
        }
        else {
          do {
            __snprintf(local_440,0xff,"%scheckpoints\\%s",param_1,local_240.cFileName);
            __snprintf(local_340,0xff,"%scheckpoints\\%s");
            BVar4 = CopyFileA(local_340,local_440,0);
            if ((char)BVar4 == '\0') break;
            BVar5 = FindNextFileA((HANDLE)local_444,&local_240);
          } while (BVar5 != 0);
          hFindFile = (HANDLE)FindClose((HANDLE)local_444);
          cVar6 = (char)BVar4;
        }
        if (cVar6 != '\0') {
          __snprintf(local_100,0xff,"%scheckpoints\\*.bin");
          hFindFile = FindFirstFileA(local_100,&local_240);
          if (hFindFile != (HANDLE)0xffffffff) {
            do {
              __snprintf(local_440,0xff,"%scheckpoints\\%s",param_1,local_240.cFileName);
              __snprintf(local_340,0xff,"%scheckpoints\\%s");
              BVar4 = CopyFileA(local_340,local_440,0);
              if ((char)BVar4 == '\0') break;
              BVar4 = FindNextFileA(hFindFile,&local_240);
            } while (BVar4 != 0);
            hFindFile = (HANDLE)FindClose(hFindFile);
          }
        }
      }
      uVar2 = CONCAT31((int3)((uint)hFindFile >> 8),(char)uVar2);
    }
  }
  return uVar2;
}
#endif
