// saved_game_create_default_profile  (Ghidra: FUN_00539ab0, renamed)
// address 0x539ab0, size 268 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/saved_games_functions.md summary corrected by
// out/phase4/saved_games_types_notes.md ("0x539ab0 does not build a network packet, it creates
// a new profile slot (saved_game_create_slot(0, name)) and writes a fresh default profile to
// it"). in_ECX is the wide display name (wcsncpy'd into the profile at +2, matching
// saved_player_profile::name). Builds a fresh saved_player_profile_file (zeroed, then
// player_profile_initialize'd, named, crc'd) and writes it to the new slot's file, rolling the
// slot back on failure. file_reference_seek/_write/_close signatures taken from this session's
// sibling files (src/saved_games/file_reference_*.c, already rewritten).
// register convention: name in ECX; no recognized stack parameters.
// UNSURE: file_reference_seek/_write/_close's exact arguments here (offset 0, the whole
// profile_file buffer and its size, and the reference) are inferred from the zeroed/crc'd
// buffer and the file_reference helpers' own signatures, not confirmed in objdump for this
// specific call site.

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

extern uint32_t saved_game_create_slot(uint16_t type, uint16_t *display_name); // 0x53c660, this module
extern uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_reference); // 0x53c9f0, not in this batch
extern void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index,
    uint8_t merge_existing); // 0x53a1c0
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // 0x5558f0, not in this batch
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, not in this batch
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, not in this batch
extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, this module

// blam-cc: name in ECX
// Creates a new player-profile save slot named `name`, writes a freshly initialized default
// profile into it, and returns its handle. Rolls the slot back (deleting it) if the file
// couldn't be opened or written.
uint32_t saved_game_create_default_profile(uint16_t *name)
{
    uint32_t handle;
    file_reference_record ref;
    saved_player_profile_file file;

    handle = saved_game_create_slot(_saved_game_type_player_profile, name);
    if (handle == 0xffffffff) {
        return 0xffffffff;
    }

    if (saved_game_open_file_by_handle(handle, &ref) == 0) {
        saved_game_delete_by_handle(handle);
        return 0xffffffff;
    }

    memset(&file, 0, sizeof(file));
    player_profile_initialize(&file.profile, 0, 1);
    wcsncpy((wchar_t *)file.profile.name, (const wchar_t *)name, 0xb);

    file.checksum = 0xffffffff;
    crc32_update(&file.checksum, (uint8_t *)&file.profile, k_saved_player_profile_size);

    if (file_reference_seek(0, &ref) == 0 || file_reference_write(&ref, &file, sizeof(file)) == 0) {
        saved_game_delete_by_handle(handle);
        handle = 0xffffffff;
    }
    file_reference_close(&ref);
    return handle;
}

#if 0
Original Ghidra decompilation (0x539ab0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_00539ab0(void)

{
  char cVar1;
  uint uVar2;
  wchar_t *in_ECX;
  int iVar3;
  undefined4 *puVar4;
  undefined1 local_2118 [272];
  undefined1 local_2008;
  undefined4 local_2007;
  undefined2 local_1ff0;
  undefined2 local_1eec;
  undefined4 local_c [2];

  local_c[0] = 0x539ac0;
  uVar2 = saved_game_create_slot(0,in_ECX);
  if (uVar2 == 0xffffffff) {
    return 0xffffffff;
  }
  cVar1 = saved_game_open_file_by_handle(local_2118);
  if (cVar1 != '\0') {
    local_2008 = 0;
    puVar4 = &local_2007;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)puVar4 = 0;
    *(undefined1 *)((int)puVar4 + 2) = 0;
    player_profile_initialize(&local_2008,0,1);
    local_1eec = 0;
    _wcsncpy((wchar_t *)((int)&local_2007 + 1),in_ECX,0xb);
    local_1ff0 = 0;
    local_c[0] = 0xffffffff;
    crc32_update(local_c,&local_2008,0x1ffc);
    cVar1 = file_reference_seek();
    if ((cVar1 == '\0') || (cVar1 = file_reference_write(), cVar1 == '\0')) {
      saved_game_delete_by_handle();
      uVar2 = 0xffffffff;
    }
    file_reference_close();
    return uVar2;
  }
  saved_game_delete_by_handle();
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
