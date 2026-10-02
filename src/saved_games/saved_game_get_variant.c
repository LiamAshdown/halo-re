// saved_game_get_variant  (Ghidra: FUN_0053bee0, renamed)
// address 0x53bee0, size 459 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/saved_games_functions.md summary "Returns the wide display name for a
// given profile handle, either by direct lookup or, for the 'current' profile, by validating the
// on-disk record and falling back to a generated default name" undersells what the binary does:
// every path fills the CALLER's full game_variant (0x26-dword copy), not just a name. Callers
// confirm the shape: src/game/game_engine_get_variant_by_name.c and
// src/interface/saved_item_select.c declare it `uint8_t FUN_0053bee0(int32_t handle,
// game_variant *out)`, __cdecl (plain stack arguments, both fully resolved by Ghidra here too).
// The sign check ("-1 < param_1", i.e. handle >= 0) is read as testing
// k_saved_game_handle_valid_bit (0x80000000): a handle packed by savegame_slot_handle_pack (FUN_0053e630) for a
// checksum-confirmed index entry has that bit set and is therefore NEGATIVE as a signed int32,
// and only that branch actually opens and crc-validates the on-disk blam.lst; a non-negative
// handle (not yet checksum-confirmed, e.g. a raw candidate id) skips straight to the
// defaults+cached-name fallback. This also explains why callers that pass the -1 "use current
// custom variant" sentinel (e.g. game_variant_list_matching_substring.c) special-case it and
// never call this function with it: -1 would take the disk-read branch and simply fail to
// resolve a slot.
// This function unconditionally waits for and clears the asynchronous game-variant writer thread
// first, exactly like control_profile_variant_write_wait_and_clear.c (0x53bae0), except it also
// clears the thread pointer itself (DAT_00721324 = 0) here, where 0x53bae0 leaves that to its own
// later bulk zero of the whole variant_write_request block; 0x53bee0 has no such bulk zero, so
// the explicit pointer clear is kept.
// UNSURE: the extra dword Ghidra writes immediately after each defaults-variant copy (its
// `local_21b4 = 0;`, in the stack slot right past the copied 0x98-byte struct) has no effect on
// the data actually returned (the returned copy is bounded to sizeof(game_variant) dwords) and
// is reproduced as a dead write to an unused local rather than folded into the struct.
// register convention: __cdecl, plain stack arguments (handle, out).

#include "crt.h"
#include "win32.h"
#include <string.h>
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

extern network_thread_record *variant_write_thread; // 0x00721324
extern network_mutex_record *saved_game_files_mutex; // 0x0072143c

extern game_variant *game_engine_variant_defaults_classic_slayer(game_variant *out); // 0x463c40
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern uint16_t *saved_game_get_display_name(int32_t handle); // 0x53c600, this module
extern uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_ref); // 0x53c9f0, this module
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module

// blam-cc: __cdecl, plain stack arguments (handle, out)
// Fills *out with a full game_variant for handle: if handle is non-negative (checksum bit clear,
// not a confirmed index entry), or if the disk read/crc check below fails, *out gets the
// classic-slayer defaults with its name field overwritten by saved_game_get_display_name(handle);
// otherwise (handle negative, i.e. checksum-confirmed) it opens the handle's blam.lst, reads and
// crc-validates its body, and returns that real data on a match. Always first waits for and
// clears any in-flight asynchronous game-variant write. Returns 1 on success, 0 if the mutex
// wait/handle-open/read failed on the disk-read path.
uint8_t saved_game_get_variant(int32_t handle, game_variant *out)
{
    uint32_t exit_code;
    uint32_t wait_result;
    file_reference_record ref;
    game_variant_file file;
    uint32_t checksum;
    game_variant defaults;
    game_variant *defaults_ptr;
    uint16_t *display_name;
    uint16_t dead_word; // UNSURE: Ghidra's local_21b4/local_21b4-equivalent, see header comment
    uint8_t opened;
    uint8_t read_ok;
    uint8_t result;

    if (variant_write_thread != 0) {
        do {
            do {
            } while (GetExitCodeThread(variant_write_thread->handle, (LPDWORD)&exit_code) == 0);
        } while (exit_code == 0x103);
        CloseHandle(variant_write_thread->handle);
        variant_write_thread->handle = 0;
        variant_write_thread->in_use = 0;
        variant_write_thread = 0;
    }

    if (-1 < handle) {
        defaults_ptr = game_engine_variant_defaults_classic_slayer(&defaults);
        memcpy(&defaults, defaults_ptr, sizeof(defaults));
        dead_word = 0;
        display_name = saved_game_get_display_name(handle);
        wcsncpy((wchar_t *)defaults.name, (const wchar_t *)display_name, 0x17);
        defaults.name[0x17] = 0;
        memcpy(out, &defaults, sizeof(*out));
        return 1;
    }

    result = 0;
    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80) {
        opened = saved_game_open_file_by_handle(handle, &ref);
        if (opened != 0) {
            read_ok = file_reference_read(&ref, &file, sizeof(file));
            if (read_ok != 0) {
                checksum = 0xffffffff;
                crc32_update(&checksum, &file.variant, sizeof(file.variant));
                if (checksum == file.checksum) {
                    memcpy(out, &file.variant, sizeof(*out));
                } else {
                    defaults_ptr = game_engine_variant_defaults_classic_slayer(&defaults);
                    memcpy(&defaults, defaults_ptr, sizeof(defaults));
                    dead_word = 0;
                    display_name = saved_game_get_display_name(handle);
                    wcsncpy((wchar_t *)defaults.name, (const wchar_t *)display_name, 0x17);
                    defaults.name[0x17] = 0;
                    memcpy(out, &defaults, sizeof(*out));
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
Original Ghidra decompilation (0x53bee0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_0053bee0(int param_1,undefined4 *param_2)

{
  char cVar1;
  BOOL BVar2;
  DWORD DVar3;
  undefined4 *extraout_EAX;
  wchar_t *pwVar4;
  undefined4 *extraout_EAX_00;
  int iVar5;
  undefined4 uVar6;
  undefined4 *puVar7;
  DWORD local_224c;
  wchar_t local_2248 [23];
  undefined2 local_221a;
  undefined2 local_21b4;
  undefined1 local_21b0 [152];
  undefined1 local_2118 [272];
  wchar_t local_2008 [76];
  DWORD local_1f70;
  undefined4 uStack_c;

  uStack_c = 0x53bef0;
  uVar6 = 0;
  if (DAT_00721324 != (undefined4 *)0x0) {
    do {
      do {
        BVar2 = GetExitCodeThread((HANDLE)*DAT_00721324,&local_224c);
        puVar7 = DAT_00721324;
      } while (BVar2 == 0);
    } while (local_224c == 0x103);
    CloseHandle((HANDLE)*DAT_00721324);
    *puVar7 = 0;
    *(undefined1 *)(puVar7 + 1) = 0;
    DAT_00721324 = (undefined4 *)0x0;
  }
  if (-1 < param_1) {
    game_engine_variant_defaults_classic_slayer(local_21b0);
    puVar7 = extraout_EAX_00;
    pwVar4 = local_2248;
    for (iVar5 = 0x26; iVar5 != 0; iVar5 = iVar5 + -1) {
      *(undefined4 *)pwVar4 = *puVar7;
      puVar7 = puVar7 + 1;
      pwVar4 = pwVar4 + 2;
    }
    local_21b4 = 0;
    pwVar4 = (wchar_t *)FUN_0053c600();
    _wcsncpy(local_2248,pwVar4,0x17);
    local_221a = 0;
    pwVar4 = local_2248;
    for (iVar5 = 0x26; iVar5 != 0; iVar5 = iVar5 + -1) {
      *param_2 = *(undefined4 *)pwVar4;
      pwVar4 = pwVar4 + 2;
      param_2 = param_2 + 1;
    }
    return 1;
  }
  DVar3 = WaitForSingleObject((HANDLE)*DAT_0072143c,5000);
  if ((DVar3 == 0) || (DVar3 == 0x80)) {
    cVar1 = saved_game_open_file_by_handle(local_2118);
    if (cVar1 != '\0') {
      cVar1 = file_reference_read();
      if (cVar1 != '\0') {
        local_224c = 0xffffffff;
        crc32_update(&local_224c,local_2008,0x98);
        if (local_224c == local_1f70) {
          pwVar4 = local_2008;
        }
        else {
          game_engine_variant_defaults_classic_slayer(local_21b0);
          puVar7 = extraout_EAX;
          pwVar4 = local_2248;
          for (iVar5 = 0x26; iVar5 != 0; iVar5 = iVar5 + -1) {
            *(undefined4 *)pwVar4 = *puVar7;
            puVar7 = puVar7 + 1;
            pwVar4 = pwVar4 + 2;
          }
          local_21b4 = 0;
          pwVar4 = (wchar_t *)FUN_0053c600();
          _wcsncpy(local_2248,pwVar4,0x17);
          local_221a = 0;
          pwVar4 = local_2248;
        }
        for (iVar5 = 0x26; iVar5 != 0; iVar5 = iVar5 + -1) {
          *param_2 = *(undefined4 *)pwVar4;
          pwVar4 = pwVar4 + 2;
          param_2 = param_2 + 1;
        }
        uVar6 = 1;
      }
      file_reference_close();
    }
    ReleaseMutex((HANDLE)*DAT_0072143c);
  }
  return uVar6;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
