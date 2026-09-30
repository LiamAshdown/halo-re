// player_profile_write_data  (Ghidra: player_profile_write_data, already named)
// address 0x53a950, size 194 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; Ghidra recognized both stack parameters
// (handle, profile). The `local_c[0] = 0x53a960` line is the same __chkstk-frame decompiler
// artifact seen in player_profile_get (0x53a770) and saved_game_create_default_profile
// (0x539ab0); harmless here since it is unconditionally overwritten with -1 before the crc is
// computed. file_reference_seek/_write/_close/saved_game_open_file_by_handle conventions as
// established for the other file-writer functions in this module.
// register convention: __cdecl; handle and profile are the recognized stack parameters.
// Phase 4 review (objdump 0x53a9e8..0x53a9f4): player_profile_rename gets the handle in EAX and
// the caller's own profile + 2 (its name) on the stack; the first rewrite dropped the handle.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_reference); // 0x53c9f0, not in this batch
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // this module
extern uint8_t file_reference_close(file_reference_record *ref); // this module


// Checksums `profile` and writes it to the save slot named by `handle`. On a successful close,
// re-syncs the slot's name/index via player_profile_rename(profile->name) (keeps the on-disk
// slot name in step with a profile whose name field changed since it was created). Rolls the
// slot back (deletes it) if the seek or write failed.
void player_profile_write_data(int32_t handle, saved_player_profile *profile)
{
    file_reference_record ref;
    saved_player_profile_file file;
    uint8_t write_failed;

    write_failed = 0;
    if (saved_game_open_file_by_handle(handle, &ref) == 0) {
        return;
    }

    file.profile = *profile;
    file.checksum = 0xffffffff;
    crc32_update(&file.checksum, (uint8_t *)&file.profile, k_saved_player_profile_size);

    if (file_reference_seek(0, &ref) == 0 || file_reference_write(&ref, &file, sizeof(file)) == 0) {
        write_failed = 1;
    }
    if (file_reference_close(&ref) != 0) {
        player_profile_rename(handle, profile->name);
    }
    if (write_failed != 0) {
        saved_game_delete_by_handle(handle);
    }
}

#if 0
Original Ghidra decompilation (0x53a950):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void player_profile_write_data(undefined4 param_1,undefined4 *param_2)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined1 local_2118 [272];
  undefined4 local_2008 [2047];
  undefined4 local_c [2];

  local_c[0] = 0x53a960;
  bVar1 = false;
  cVar2 = saved_game_open_file_by_handle(local_2118);
  if (cVar2 != '\0') {
    puVar4 = param_2;
    puVar5 = local_2008;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    local_c[0] = 0xffffffff;
    crc32_update(local_c,local_2008,0x1ffc);
    cVar2 = file_reference_seek();
    if ((cVar2 == '\0') || (cVar2 = file_reference_write(), cVar2 == '\0')) {
      bVar1 = true;
    }
    cVar2 = file_reference_close();
    if (cVar2 != '\0') {
      player_profile_rename((int)param_2 + 2);
    }
    if (bVar1) {
      saved_game_delete_by_handle();
    }
    return;
  }
  return;
}
#endif
