// player_profile_get  (Ghidra: player_profile_get, already named)
// address 0x53a770, size 470 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x53a770 re-derived by hand
// because Ghidra's own decompilation invents a bogus local ("local_4 = 0x53a77a") from stack-
// frame analysis noise and compares it to the crc, which cannot be right (a per-function code
// address can never legitimately equal a crc32 of file content); the real comparison, at
// 0x53a856..0x53a86a, is the freshly computed crc against the checksum field that
// file_reference_read (0x555a20) just read into the tail of the same buffer, plus a version ==
// k_saved_player_profile_version check (0x53a86c) -- exactly the saved_player_profile_file
// layout. Also recovers: out_buffer in ECX (mov edi,ecx at entry, matching in_ECX in Ghidra's
// text); handle -1 both selects the mutex-guarded "real" read path (index < 0) and is the
// handle saved_game_open_file_by_handle/FUN_0053c600 are called with, matching
// saved_game_handle_bits' "-1 means none (and, for the profile slot, the built-in default
// profile)"; index >= 0 instead just fabricates an in-memory default profile named from
// FUN_0053c600(index), no disk access. Also joins/clears any pending profile-verification
// thread first, exactly like player_profile_verify_thread_wait_and_clear (0x539a40).
// register convention: out_buffer in ECX; index is the recognized stack parameter.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern network_thread_record *player_profile_thread; // 0x0072127c
extern network_mutex_record *saved_game_files_mutex; // 0x0072143c

extern void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index,
    uint8_t merge_existing); // 0x53a1c0
extern uint16_t *saved_game_get_display_name(int32_t handle); // 0x53c600, not in this batch
extern uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_reference); // 0x53c9f0, not in this batch
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // this module
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern uint8_t file_reference_close(file_reference_record *ref); // this module

static void player_profile_build_default(saved_player_profile *profile, int32_t handle)
{
    uint16_t *name;

    player_profile_initialize(profile, 0, 0);
    name = saved_game_get_display_name(handle);
    wcsncpy((wchar_t *)profile->name, (const wchar_t *)name, 0xb);
}

// blam-cc: out_buffer in ECX, then the recognized stack parameter (index)
// index >= 0: fabricates a fresh default profile named from index's display name, into
// *out_buffer, always succeeding (no disk access). index == -1 (or any negative): joins any
// pending profile-verification thread, then, holding saved_game_files_mutex, reads and
// crc/version-validates the real default profile file (handle -1), falling back to a
// fabricated default profile on any read or validation failure. Returns success.
uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer)
{
    uint8_t result;
    file_reference_record ref;
    saved_player_profile_file file;
    uint32_t running_crc;
    uint32_t wait_result;

    result = 0;

    if (player_profile_thread != 0) {
        uint32_t exit_code;
        do {
            while (GetExitCodeThread(player_profile_thread->handle, (LPDWORD)&exit_code) == 0) {
                /* keep polling */
            }
        } while (exit_code == 0x103 /* STILL_ACTIVE */);
        CloseHandle(player_profile_thread->handle);
        player_profile_thread->handle = 0;
        player_profile_thread->in_use = 0;
        player_profile_thread = 0;
    }

    if (0 <= index) {
        player_profile_build_default(out_buffer, index);
        return 1;
    }

    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80 /* WAIT_ABANDONED */) {
        if (saved_game_open_file_by_handle(index, &ref) != 0) {
            if (file_reference_read(&ref, &file, sizeof(file)) != 0) {
                running_crc = 0xffffffff;
                crc32_update(&running_crc, (uint8_t *)&file.profile, k_saved_player_profile_size);
                if (running_crc == file.checksum && file.profile.version == k_saved_player_profile_version) {
                    *out_buffer = file.profile;
                } else {
                    player_profile_build_default(out_buffer, index);
                }
                result = 1;
            }
            file_reference_close(&ref);
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x53a770):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined1 player_profile_get(int param_1)

{
  undefined4 *puVar1;
  char cVar2;
  BOOL BVar3;
  DWORD DVar4;
  wchar_t *pwVar5;
  undefined4 *in_ECX;
  int iVar6;
  char *pcVar7;
  undefined1 local_410d;
  DWORD local_410c;
  undefined1 local_4108 [268];
  char local_3ffc [2];
  wchar_t local_3ffa [11];
  undefined2 local_3fe4;
  undefined2 local_3ee0;
  char local_2000 [8188];
  DWORD local_4;

  local_4 = 0x53a77a;
  local_410d = 0;
  if (DAT_0072127c != (undefined4 *)0x0) {
    do {
      do {
        BVar3 = GetExitCodeThread((HANDLE)*DAT_0072127c,&local_410c);
        puVar1 = DAT_0072127c;
      } while (BVar3 == 0);
    } while (local_410c == 0x103);
    CloseHandle((HANDLE)*DAT_0072127c);
    *puVar1 = 0;
    *(undefined1 *)(puVar1 + 1) = 0;
    DAT_0072127c = (undefined4 *)0x0;
  }
  if (-1 < param_1) {
    player_profile_initialize(local_3ffc,0,0);
    local_3ee0 = 0;
    pwVar5 = (wchar_t *)FUN_0053c600();
    _wcsncpy((wchar_t *)(local_3ffc + 2),pwVar5,0xb);
    local_3fe4 = 0;
    pcVar7 = local_3ffc;
    for (iVar6 = 0x7ff; iVar6 != 0; iVar6 = iVar6 + -1) {
      *in_ECX = *(undefined4 *)pcVar7;
      pcVar7 = pcVar7 + 4;
      in_ECX = in_ECX + 1;
    }
    return 1;
  }
  DVar4 = WaitForSingleObject((HANDLE)*DAT_0072143c,5000);
  if ((DVar4 == 0) || (DVar4 == 0x80)) {
    cVar2 = saved_game_open_file_by_handle(local_4108);
    if (cVar2 != '\0') {
      cVar2 = file_reference_read();
      if (cVar2 != '\0') {
        local_410c = 0xffffffff;
        crc32_update(&local_410c,local_2000,0x1ffc);
        if ((local_410c == local_4) && (local_2000[0] == '\t')) {
          pcVar7 = local_2000;
        }
        else {
          player_profile_initialize(local_3ffc,0,0);
          local_3ee0 = 0;
          pwVar5 = (wchar_t *)FUN_0053c600();
          _wcsncpy((wchar_t *)(local_3ffc + 2),pwVar5,0xb);
          local_3fe4 = 0;
          pcVar7 = local_3ffc;
        }
        for (iVar6 = 0x7ff; iVar6 != 0; iVar6 = iVar6 + -1) {
          *in_ECX = *(undefined4 *)pcVar7;
          pcVar7 = pcVar7 + 4;
          in_ECX = in_ECX + 1;
        }
        local_410d = 1;
      }
      file_reference_close();
    }
    ReleaseMutex((HANDLE)*DAT_0072143c);
  }
  return local_410d;
}
#endif
