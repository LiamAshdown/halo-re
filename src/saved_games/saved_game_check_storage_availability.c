// saved_game_check_storage_availability  (Ghidra: saved_game_check_storage_availability,
// already named)
// address 0x53d120, size 182 bytes
// name confidence: 0.8   rewrite confidence: 0.6
// evidence: already named by Ghidra/CEA (__cdecl, no arguments). Confirmed against objdump
// 0x53d120..0x53d1d5: savegame_find_first takes (root directory on the stack, out find-data
// buffer in EAX) and returns a find handle in EAX; savegame_find_next takes (out find-data
// buffer in EAX, handle in ECX) and returns a bool in AL; user_save_path_remove takes (handle in
// EAX) and returns a bool in AL. Ghidra's final `return (uint)pvVar2 & 0xffff0000` is a
// decompiler artifact: EBX is zeroed once at entry and never written again, so the fall-through
// exit at 0x53d1c9 (`mov ax,bx`) always returns 0 (_saved_game_storage_ok), not a masked
// leftover register value.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern char savegames_directory[0x100]; // 0x00721549

extern int32_t __stdcall GetDiskFreeSpaceExA(const char *directory, uint64_t *free_bytes_available,
    uint64_t *total_bytes, uint64_t *total_free_bytes); // Win32
extern int32_t savegame_find_first(const char *root, void *out_find_data); // 0x551bc0, game module
extern uint8_t savegame_find_next(void *out_find_data, int32_t handle); // 0x551d30, game module
extern uint8_t user_save_path_remove(int32_t handle); // 0x5516d0, game module
extern int32_t __stdcall FindClose(void *handle); // Win32

// blam-cc: __cdecl, no arguments
// Returns _saved_game_storage_ok if there is at least 0x2800000 bytes of free disk space.
// Otherwise counts the entries under savegames_directory (up to 999); if there are already at
// least 999, returns _saved_game_storage_too_many_saves, else _saved_game_storage_ok (0 by way
// of the always-zero EBX return, matching the binary).
int32_t saved_game_check_storage_availability(void)
{
    uint64_t free_bytes_available;
    uint64_t total_bytes;
    uint64_t total_free_bytes;
    int32_t ok;
    int32_t handle;
    int32_t count;
    uint8_t find_data[0x344]; // xgame_find_data-sized scratch, large enough for either finder
    uint8_t found;
    uint8_t removed;

    ok = GetDiskFreeSpaceExA(savegames_directory, &free_bytes_available, &total_bytes, &total_free_bytes);
    if (ok != 0 && (free_bytes_available >> 32) == 0 && (uint32_t)free_bytes_available < 0x2800000) {
        return _saved_game_storage_ok;
    }

    handle = savegame_find_first(savegames_directory, find_data);
    count = 1;
    if (handle != -1) {
        do {
            if (0x3e6 < count) {
                break;
            }
            count++;
            found = savegame_find_next(find_data, handle);
        } while (found == 1);
        if (handle != 0) {
            removed = user_save_path_remove(handle);
            if (removed != 0) {
                FindClose((void *)handle);
            }
        }
        if (0x3e6 < count) {
            return _saved_game_storage_too_many_saves;
        }
    }
    return _saved_game_storage_ok;
}

#if 0
Original Ghidra decompilation (0x53d120):

uint __cdecl saved_game_check_storage_availability(void)

{
  BOOL BVar1;
  HANDLE hFindFile;
  HANDLE pvVar2;
  uint uVar3;
  ULARGE_INTEGER local_35c;
  ULARGE_INTEGER local_354;
  ULARGE_INTEGER local_34c [105];

  BVar1 = GetDiskFreeSpaceExA(&DAT_00721549,&local_35c,local_34c,&local_354);
  if (((BVar1 != 0) && (local_35c.s.HighPart == 0)) && (local_35c.s.LowPart < 0x2800000)) {
    return 1;
  }
  hFindFile = (HANDLE)savegame_find_first(&DAT_00721549);
  uVar3 = 1;
  pvVar2 = hFindFile;
  if (hFindFile != (HANDLE)0xffffffff) {
    do {
      if (0x3e6 < uVar3) break;
      uVar3 = uVar3 + 1;
      pvVar2 = (HANDLE)savegame_find_next();
    } while ((char)pvVar2 == '\x01');
    if ((hFindFile != (HANDLE)0x0) &&
       (pvVar2 = (HANDLE)user_save_path_remove(), (char)pvVar2 != '\0')) {
      pvVar2 = (HANDLE)FindClose(hFindFile);
    }
    if (0x3e6 < uVar3) {
      return 2;
    }
  }
  return (uint)pvVar2 & 0xffff0000;
}
#endif
