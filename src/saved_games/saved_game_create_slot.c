// saved_game_create_slot  (Ghidra: saved_game_create_slot, already named)
// address 0x53c660, size 764 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: already named by Ghidra/CEA (__cdecl, both parameters fully resolved: ushort type,
// wchar_t *name). out/phase4/saved_games_functions.md summary "Creates and registers a new
// saved-game (blam.sav) or playlist (blam.lst) slot with the given type and display name,
// rolling back the XCreateSaveGame allocation on failure."
// Phase 4 review (objdump 0x53c660..0x53c95b, line by line):
//   - The record is a plain saved_game_index_entry at [esp+0x18] (0x206-byte memset at
//     0x53c72b; path +0x000, display name +0x100, [0x7f] cleared, type +0x200, index +0x202 =
//     the slot count from savegame_index_get_slot_count, builtin +0x204 = 0, checksum_valid
//     +0x205 set when the body write succeeds). The first rewrite modelled a 0x20d-byte
//     "type dword + record" struct; the dword before the entry is only a reused stack slot
//     that savegame_index_append_slot (0x53e300) receives as its out pointer and fills with
//     the slot it appended (0x53e3dd), never read back.
//   - savegame_index_append_slot takes (&entry, &out_slot) on the stack; the handle is then
//     packed from entry.index (EAX), the caller's type (ECX, kept in EDI), entry.builtin (DL)
//     and the dword at entry+0x205 (stack, only its low byte is tested).
//   - XCreateSaveGame gets the caller's wide name in EAX (mov eax,esi, esi = [ebp+0xc], call
//     at 0x53c71e); the first rewrite passed -1. Its failure returns -1 without a rollback.
//   - The empty body is always written as a full 0x2000-byte zero buffer (mov esi,0x2000 at
//     0x53c8a0) with the crc stored at body + body_size, so new blam.lst files are 0x2000
//     bytes, not 0x9c. The first rewrite wrote body_size + 4.
//   - Every failure after XCreateSaveGame (unknown type, file_reference_init / create
//     failure, append failure) reaches the rollback at 0x53c90b / 0x53c90e with EAX = name;
//     a failed open skips the write but still appends the entry.
// register convention: __cdecl, ushort type and wchar_t *name as ordinary stack arguments.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint8_t savegame_index_dirty; // 0x00721447
extern int16_t saved_game_pending_error; // 0x00718fac, UNSURE name (interface error code slot)
extern int16_t saved_game_pending_error_arg; // 0x00718fae
extern uint8_t saved_game_pending_error_flag1; // 0x00718fb0
extern uint8_t saved_game_pending_error_flag2; // 0x00718fb1
extern char savegames_directory[0x100]; // 0x00721549

extern void saved_game_list_rebuild_index(void); // 0x53d720, this module
extern int32_t saved_game_check_storage_availability(void); // 0x53d120, this module
extern int32_t savegame_index_get_slot_count(void); // 0x53e420, FUN_0053e420 (src/game)
extern uint32_t savegame_slot_handle_pack(uint32_t slot_index, uint32_t type_nibble, uint8_t flag_bit_30,
    uint8_t flag_bit_31); // 0x53e630, FUN_0053e630 (src/game); blam-cc: EAX slot, ECX type, DL bit 30, stack byte bit 31
extern uint8_t savegame_index_append_slot(const saved_game_index_entry *entry, int32_t *out_slot); // 0x53e300, FUN_0053e300 (src/game); bool in AL
extern void *game_state_open_persistent_storage(char *directory_path); // 0x5398e0, outside this batch
extern file_reference_record *file_reference_init(file_reference_record *ref, const char *component, uint8_t is_file); // 0x5554c0, this module
extern uint8_t file_reference_create(file_reference_record *ref); // 0x5555b0, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module

extern uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path,
    uint32_t out_path_size); // 0x551710, blam-cc: EAX save_game_name (src/game/XCreateSaveGame.c: validity_token)
                              // and saved_game_allocate_new_slot.c for the other observed uses)
extern uint32_t XDeleteSaveGame(const uint16_t *save_game_name, const char *root_path); // 0x5519a0, blam-cc: EAX save_game_name, ECX root_path
                                                                    // (see saved_game_delete_by_handle.c)
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern int32_t __snprintf(char *buffer, uint32_t count, const char *format, ...); // CRT
extern void _wcsncpy(uint16_t *dest, const uint16_t *source, uint32_t count); // CRT
extern int32_t __stdcall CloseHandle(void *object); // Win32

// blam-cc: __cdecl, plain stack arguments (type, name)
// Registers a new saved-game (type 0: blam.sav) or playlist (type 1: blam.lst) slot named
// name. Checks storage availability and the 999-slot limit (reporting a UI error the first time
// either trips), asks the Xbox save API for a fresh directory, builds and writes the empty
// body+checksum file there (creating savegame.bin too for a profile), then registers the entry
// via savegame_index_append_slot and packs its handle via savegame_slot_handle_pack. Rolls
// back (XDeleteSaveGame) on any
// failure after the directory was created. Returns the packed handle, or 0xffffffff on failure.

uint32_t saved_game_create_slot(uint16_t type, uint16_t *name)
{
    int32_t storage_status;
    int32_t entry_count;
    char directory[0x100];
    int32_t create_result;
    saved_game_index_entry entry;
    int32_t appended_slot;
    void *storage_handle;
    file_reference_record ref;
    uint8_t body[0x2000];
    uint32_t body_size;
    uint8_t ok;
    int32_t registered;
    int32_t handle;
    int32_t saved_type;

    if (savegame_index_dirty != 0) {
        saved_game_list_rebuild_index();
    }
    storage_status = saved_game_check_storage_availability();
    if (storage_status == 1) {
        if (saved_game_pending_error == -1) {
            saved_game_pending_error = 0x21;
            saved_game_pending_error_arg = -1;
            saved_game_pending_error_flag1 = 1;
            saved_game_pending_error_flag2 = 0;
        }
    } else if (storage_status == 2 && saved_game_pending_error == -1) {
        saved_game_pending_error = 0x22;
        saved_game_pending_error_arg = -1;
        saved_game_pending_error_flag1 = 1;
        saved_game_pending_error_flag2 = 0;
    }
    if (storage_status != 0) {
        return 0xffffffff;
    }

    entry_count = savegame_index_get_slot_count();
    if (0x3e6 < entry_count) {
        if (saved_game_pending_error != -1) {
            return 0xffffffff;
        }
        saved_game_pending_error = 0x24;
        saved_game_pending_error_arg = -1;
        saved_game_pending_error_flag1 = 1;
        saved_game_pending_error_flag2 = 0;
        return 0xffffffff;
    }

    memset(directory, 0, sizeof(directory));
    create_result = XCreateSaveGame(name, savegames_directory, 1, directory, 0x100);
    if (create_result != 0) {
        return 0xffffffff;
    }

    memset(&entry, 0, sizeof(entry));
    _wcsncpy(entry.display_name, name, 0x7f);
    entry.display_name[0x7f] = 0;
    entry.type = (int16_t)type;
    entry.index = (int16_t)entry_count;
    entry.builtin = 0;
    entry.checksum_valid = 0;

    saved_type = (int32_t)type;

    if (type == 0) {
        __snprintf(entry.path, 0xff, "%s%s", directory, "blam.sav");
        body_size = 0x1ffc;
        storage_handle = game_state_open_persistent_storage(directory);
        if (storage_handle != (void *)-1) {
            CloseHandle(storage_handle);
        }
    } else {
        if (type != 1) {
            saved_type = -1;
            goto rollback;
        }
        __snprintf(entry.path, 0xff, "%s%s", directory, "blam.lst");
        body_size = 0x98;
    }

    if (saved_type != -1) {
        ok = file_reference_init(&ref, entry.path, 0) != 0;
        if (ok) {
            ok = file_reference_create(&ref);
        }
        if (ok) {
            ok = file_reference_open(&ref, 2);
            if (ok) {
                memset(body, 0, sizeof(body));
                *(uint32_t *)(body + body_size) = 0xffffffff;
                crc32_update((uint32_t *)(body + body_size), body, body_size);
                ok = file_reference_write(&ref, body, sizeof(body)); // always 0x2000 bytes
                if (ok) {
                    entry.checksum_valid = 1;
                }
                file_reference_close(&ref);
            }
            registered = savegame_index_append_slot(&entry, &appended_slot);
            if (registered != 0) {
                // EAX entry.index, ECX the caller's type, DL entry.builtin, stack the dword at
                // entry + 0x205 (only its low byte, checksum_valid, is tested)
                handle = (int32_t)savegame_slot_handle_pack((uint32_t)(int32_t)entry.index, type,
                    entry.builtin, entry.checksum_valid);
                return (uint32_t)handle;
            }
        }
    }

rollback:
    XDeleteSaveGame(name, savegames_directory);
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x53c660):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint __cdecl saved_game_create_slot(ushort param_1,wchar_t *param_2)

{
  char cVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  HANDLE hObject;
  int iVar5;
  undefined4 *puVar6;
  int local_2430;
  uint local_242c;
  char local_2428;
  undefined4 local_2427 [63];
  wchar_t local_2328 [127];
  undefined2 local_222a;
  ushort local_2228;
  undefined2 local_2226;
  undefined1 local_2224;
  uint local_2223;
  char local_2218;
  undefined4 local_2217;
  undefined1 local_2118 [272];
  undefined4 local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x53c670;
  if (DAT_00721447 != '\0') {
    saved_game_list_rebuild_index();
  }
  uVar3 = saved_game_check_storage_availability();
  sVar2 = (short)uVar3;
  if (sVar2 == 1) {
    if (DAT_00718fac == -1) {
      DAT_00718fac = 0x21;
      goto LAB_0053c6ba;
    }
  }
  else if ((sVar2 == 2) && (DAT_00718fac == -1)) {
    DAT_00718fac = 0x22;
LAB_0053c6ba:
    DAT_00718fae = 0xffff;
    DAT_00718fb0 = 1;
    DAT_00718fb1 = 0;
  }
  if (sVar2 != 0) {
    return 0xffffffff;
  }
  iVar4 = FUN_0053e420();
  if (0x3e6 < iVar4) {
    if (DAT_00718fac != -1) {
      return 0xffffffff;
    }
    DAT_00718fac = 0x24;
    DAT_00718fae = 0xffff;
    DAT_00718fb0 = 1;
    DAT_00718fb1 = 0;
    return 0xffffffff;
  }
  local_2218 = '\0';
  puVar6 = &local_2217;
  for (iVar5 = 0x3f; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  *(undefined2 *)puVar6 = 0;
  *(undefined1 *)((int)puVar6 + 2) = 0;
  iVar5 = XCreateSaveGame(&DAT_00721549,1,&local_2218,0x100);
  if (iVar5 != 0) {
    return 0xffffffff;
  }
  local_2428 = '\0';
  puVar6 = local_2427;
  for (iVar5 = 0x81; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  *(undefined1 *)puVar6 = 0;
  _wcsncpy(local_2328,param_2,0x7f);
  local_2430._0_2_ = (undefined2)iVar4;
  local_242c = (uint)param_1;
  local_2228 = param_1;
  local_222a = 0;
  local_2226 = (undefined2)local_2430;
  local_2224 = 0;
  local_2223 = local_2223 & 0xffffff00;
  if (local_242c == 0) {
    __snprintf(&local_2428,0xff,"%s%s",&local_2218,"blam.sav");
    local_2430 = 0x1ffc;
    hObject = game_state_open_persistent_storage(&local_2218);
    if (hObject != (HANDLE)0xffffffff) {
      CloseHandle(hObject);
    }
  }
  else {
    if (local_242c != 1) {
      local_2228 = 0xffff;
      goto LAB_0053c90e;
    }
    __snprintf(&local_2428,0xff,"%s%s",&local_2218,"blam.lst");
    local_2430 = 0x98;
  }
  if (((local_2228 != 0xffff) && (iVar4 = FUN_005554c0(local_2118,&local_2428,0), iVar4 != 0)) &&
     (cVar1 = file_reference_create(), cVar1 != '\0')) {
    cVar1 = file_reference_open(2);
    if (cVar1 != '\0') {
      puVar6 = local_2008;
      for (iVar4 = 0x800; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      *(undefined4 *)((int)local_2008 + local_2430) = 0xffffffff;
      crc32_update((undefined4 *)((int)local_2008 + local_2430),local_2008,local_2430);
      cVar1 = file_reference_write();
      if (cVar1 != '\0') {
        local_2223 = CONCAT31(local_2223._1_3_,1);
      }
      file_reference_close();
    }
    cVar1 = FUN_0053e300(&local_2428,&local_242c);
    if (cVar1 != '\0') {
      uVar3 = FUN_0053e630(local_2223);
      return uVar3;
    }
  }
LAB_0053c90e:
  XDeleteSaveGame();
  return 0xffffffff;
}
#endif
