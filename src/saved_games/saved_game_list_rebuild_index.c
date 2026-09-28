// saved_game_list_rebuild_index  (Ghidra: saved_game_list_rebuild_index, already named)
// address 0x53d720, size 887 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_functions.md summary "Rescans
// the saved-games directory tree, validates and re-registers every save/playlist/profile entry
// (including the built-in defaults) into the in-memory index, and clears the index-dirty flag."
// savegame_find_first/_next/user_save_path_remove/FindClose signatures and the handle-aliasing of
// Ghidra's uninitialized-looking `hFindFile` are all confirmed via objdump by the sibling function
// saved_game_check_storage_availability.c (0x53d120), which shares the exact same
// find-first/find-next/remove/close tail. The scan-loop's local stack layout (a 0x206-byte region
// zeroed once per iteration, built up field by field) matches saved_game_index_entry exactly,
// per types/saved_games.h's own note on this function's frame ("record at local_2f58, find data
// at local_2d50 (+0x2c cFileName, +0x140 directory, +0x244 wide name)" -- i.e. xgame_find_data).
// DAT_00721438, closed near the end, is savegame_index_file.handle (0x00721330 + 0x108, the
// handle field of the file_reference_record saved_game_index_open_for_write.c just opened).
// The two separate ".sav then .lst" existence checks are two sequential inline (objdump 0x53d7e5, 0x53d961)
// copies of the same "build a fresh absolute file_reference_record naming this path and check it
// exists" pattern seen everywhere else in this module (not a call to a shared helper -- Ghidra
// shows the zero/signature/append/exists sequence twice, verbatim).
// Phase 4 review (objdump 0x53d720..0x53da96, line by line): the candidate body is always
// read with a fixed 0x2000-byte request (0x53d87e), not body_size + 4: both blam.sav and
// blam.lst are 0x2000-byte files (see game_variant_file in types/saved_games.h), and a file
// of any other size fails the exact-count read and is left checksum_valid = 0.
// register convention: __cdecl, no parameters.

#include "win32.h"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern network_mutex_record *saved_game_files_mutex; // 0x0072143c
extern char savegames_directory[0x100]; // 0x00721549
extern file_reference_record savegame_index_file; // 0x00721330
extern int16_t savegame_index_write_count; // 0x00721444
extern uint8_t saved_game_index_file_open; // 0x00721448
extern uint8_t savegame_index_dirty; // 0x00721447

extern uint8_t saved_game_index_open_for_write(void); // 0x53daa0, this module
extern int16_t saved_game_index_register_default_playlists(void); // 0x53db40, this module
extern int16_t saved_game_index_register_default_profiles(void); // 0x53dde0, this module
extern void path_remove_last_component(char *path); // 0x555f80, this module
extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern uint8_t file_reference_exists(file_reference_record *ref); // 0x555720, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern void saved_games_report_last_error(void); // 0x556170, this module

extern int32_t savegame_find_first(const char *root, void *out_find_data); // 0x551bc0, game module; blam-cc: EAX out_find_data, stack root
extern uint8_t savegame_find_next(void *out_find_data, int32_t handle); // 0x551d30, game module; blam-cc: EAX out_find_data, ECX handle
extern uint8_t user_save_path_remove(int32_t handle); // 0x5516d0, game module; blam-cc: EAX handle
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern int32_t __snprintf(char *dest, uint32_t count, const char *format, ...); // CRT
extern void _wcsncpy(uint16_t *dest, const uint16_t *source, uint32_t count); // CRT
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count

// blam-cc: __cdecl, no parameters
// Under saved_game_files_mutex, (re)opens the index file for writing and enumerates every entry
// under savegames_directory. For each found directory, tries "<dir>blam.sav" then, if that
// doesn't exist, "<dir>blam.lst"; whichever exists becomes a saved_game_index_entry (path, the
// found display name, its type, checksum_valid set if the body's crc matches), written to the
// index (subject to both the per-scan 999-entry cap and the persistent index-write-count cap).
// A directory matching neither is logged and skipped. After the scan (successful or not), always
// re-registers the built-in default playlists and profiles, closes the index file, resets
// savegame_index_write_count to -1 and saved_game_index_file_open to 0, and finally clears
// savegame_index_dirty.
void saved_game_list_rebuild_index(void)
{
    int32_t written_count;
    uint32_t wait_result;
    uint8_t index_opened;
    int32_t find_handle;
    xgame_find_data find_data;
    saved_game_index_entry entry;
    int32_t path_len;
    int16_t entry_type;
    uint32_t body_size;
    file_reference_record ref;
    char *end;
    uint8_t exists;
    uint16_t log_scratch[255];
    uint8_t opened;
    uint8_t read_ok;
    uint32_t checksum;
    uint8_t body[0x2000];
    uint8_t write_ok;
    uint8_t find_ok;
    uint8_t removed;
    int32_t closed;

    written_count = 0;
    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80) {
        index_opened = saved_game_index_open_for_write();
        if (index_opened != 0) {
            find_handle = savegame_find_first(savegames_directory, &find_data);
            if (find_handle != -1) {
                do {
                    if (0x3e6 < written_count) {
                        break;
                    }
                    memset(&entry, 0, sizeof(entry));

                    path_len = __snprintf(entry.path, 0xff, "%s%s", find_data.save_game_directory, "blam.sav");
                    entry_type = -1;
                    if (path_len < 1) {
                        goto try_variant;
                    }

                    memset(&ref, 0, sizeof(ref));
                    ref.signature = k_file_reference_signature;
                    ref.location = _file_location_absolute;
                    if ((ref.flags & _file_reference_is_file_bit) != 0) {
                        path_remove_last_component(ref.path); // unreachable: ref.flags was just zeroed above
                    }
                    path_append_component(ref.path, entry.path);
                    ref.flags |= _file_reference_is_file_bit;
                    exists = file_reference_exists(&ref);
                    if (exists == 0) {
                        goto try_variant;
                    }
                    entry_type = _saved_game_type_player_profile;
                    body_size = k_saved_player_profile_size;
                    goto have_candidate;

                try_variant:
                    path_len = __snprintf(entry.path, 0xff, "%s%s", find_data.save_game_directory, "blam.lst");
                    if (0 < path_len) {
                        memset(&ref, 0, sizeof(ref));
                        ref.signature = k_file_reference_signature;
                        ref.location = _file_location_absolute;
                        if ((ref.flags & _file_reference_is_file_bit) != 0) {
                            path_remove_last_component(ref.path); // unreachable: ref.flags was just zeroed above
                        }
                        path_append_component(ref.path, entry.path);
                        ref.flags |= _file_reference_is_file_bit;
                        exists = file_reference_exists(&ref);
                        if (exists != 0) {
                            entry_type = _saved_game_type_game_variant;
                            body_size = sizeof(game_variant);
                            goto have_candidate;
                        }
                    }
                    string_format_wide_va_bounded(0xff, log_scratch,
                        (const uint16_t *)L"random crap found by XFindNextSaveGame(): display name= '%s' path= '%hs'",
                        find_data.save_game_name, find_data.find_data.cFileName);
                    entry_type = -1;
                    goto next_entry;

                have_candidate:
                    _wcsncpy(entry.display_name, find_data.save_game_name, 0x7f);
                    entry.type = entry_type;

                    opened = file_reference_open(&ref, 1);
                    if (opened != 0) {
                        // always 0x2000 bytes (mov esi,0x2000 at 0x53d87e), whatever the type
                        read_ok = file_reference_read(&ref, body, sizeof(body));
                        if (read_ok != 0) {
                            checksum = 0xffffffff;
                            crc32_update(&checksum, body, body_size);
                            if (checksum == *(uint32_t *)(body + body_size)) {
                                entry.checksum_valid = 1;
                            }
                        }
                        file_reference_close(&ref);
                    }
                    if (0x3e6 < savegame_index_write_count) {
                        break;
                    }
                    entry.index = savegame_index_write_count;
                    savegame_index_write_count = savegame_index_write_count + 1;
                    write_ok = file_reference_write(&savegame_index_file, &entry, sizeof(entry));
                    if (write_ok == 0) {
                        break;
                    }
                    written_count = written_count + 1;

                next_entry:
                    find_ok = savegame_find_next(&find_data, find_handle);
                } while (find_ok != 0);

                if (find_handle != 0) {
                    removed = user_save_path_remove(find_handle);
                    if (removed != 0) {
                        FindClose((void *)find_handle);
                    }
                }
            }
            saved_game_index_register_default_playlists();
            saved_game_index_register_default_profiles();
            closed = CloseHandle(savegame_index_file.handle);
            if (closed == 0) {
                saved_games_report_last_error();
            } else {
                savegame_index_file.handle = 0;
            }
            savegame_index_write_count = -1;
            saved_game_index_file_open = 0;
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    savegame_index_dirty = 0;
}

#if 0
Original Ghidra decompilation (0x53d720):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void saved_game_list_rebuild_index(void)

{
  char cVar1;
  DWORD DVar2;
  int iVar3;
  BOOL BVar4;
  int iVar5;
  HANDLE hFindFile;
  undefined4 *puVar6;
  int *piVar7;
  int local_3074;
  int local_306c [2];
  byte local_3064;
  undefined2 local_3062;
  char local_2f58;
  undefined4 local_2f57 [63];
  wchar_t local_2e58 [127];
  undefined2 local_2d5a;
  undefined2 local_2d58;
  short local_2d56;
  undefined1 local_2d53;
  undefined1 local_2d24 [276];
  undefined1 local_2c10 [260];
  wchar_t local_2b0c [130];
  undefined1 local_2a08 [510];
  undefined2 local_280a;
  CHAR local_2808 [2048];
  int local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x53d730;
  local_3074 = 0;
  DVar2 = WaitForSingleObject((HANDLE)*DAT_0072143c,5000);
  if ((DVar2 == 0) || (DVar2 == 0x80)) {
    cVar1 = FUN_0053daa0();
    if (cVar1 != '\0') {
      iVar3 = savegame_find_first(&DAT_00721549);
      if (iVar3 != -1) {
        do {
          if (0x3e6 < local_3074) break;
          local_2f58 = '\0';
          puVar6 = local_2f57;
          for (iVar5 = 0x81; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar6 = 0;
            puVar6 = puVar6 + 1;
          }
          *(undefined1 *)puVar6 = 0;
          iVar5 = __snprintf(&local_2f58,0xff,"%s%s",local_2c10,"blam.sav");
          if (iVar5 < 1) {
LAB_0053d936:
            iVar5 = __snprintf(&local_2f58,0xff,"%s%s",local_2c10,"blam.lst");
            if (0 < iVar5) {
              piVar7 = local_306c;
              for (iVar5 = 0x43; piVar7 = piVar7 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
                *piVar7 = 0;
              }
              local_306c[1] = 0x66696c6f;
              local_3062 = 2;
              if ((local_3064 & 1) != 0) {
                path_remove_last_component();
              }
              path_append_component();
              local_3064 = local_3064 | 1;
              cVar1 = file_reference_exists();
              if (cVar1 != '\0') {
                local_2d58 = 1;
                iVar5 = 0x98;
                goto LAB_0053d844;
              }
            }
            string_format_wide_va_bounded
                      (local_2a08,
                       L"random crap found by XFindNextSaveGame(): display name= \'%s\' path= \'%hs\'"
                       ,local_2b0c,local_2d24);
            local_280a = 0;
            local_2d58 = 0xffff;
          }
          else {
            piVar7 = local_306c;
            for (iVar5 = 0x43; piVar7 = piVar7 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
              *piVar7 = 0;
            }
            local_306c[1] = 0x66696c6f;
            local_3062 = 2;
            if ((local_3064 & 1) != 0) {
              path_remove_last_component();
            }
            path_append_component();
            local_3064 = local_3064 | 1;
            cVar1 = file_reference_exists();
            if (cVar1 == '\0') goto LAB_0053d936;
            local_2d58 = 0;
            iVar5 = 0x1ffc;
LAB_0053d844:
            _wcsncpy(local_2e58,local_2b0c,0x7f);
            local_2d5a = 0;
            cVar1 = file_reference_open(1);
            if (cVar1 != '\0') {
              cVar1 = file_reference_read();
              if (cVar1 != '\0') {
                local_306c[0] = -1;
                crc32_update(local_306c,local_2008,iVar5);
                if (local_306c[0] == *(int *)((int)local_2008 + iVar5)) {
                  local_2d53 = 1;
                }
              }
              file_reference_close();
            }
            if (0x3e6 < DAT_00721444) break;
            local_2d56 = DAT_00721444;
            DAT_00721444 = DAT_00721444 + 1;
            cVar1 = file_reference_write();
            if (cVar1 == '\0') break;
            local_3074 = local_3074 + 1;
          }
          cVar1 = savegame_find_next();
        } while (cVar1 != '\0');
        if ((iVar3 != 0) && (cVar1 = user_save_path_remove(), cVar1 != '\0')) {
          FindClose(hFindFile);
        }
      }
      saved_game_index_register_default_playlists();
      saved_game_index_register_default_profiles();
      BVar4 = CloseHandle(DAT_00721438);
      if (BVar4 == 0) {
        DVar2 = GetLastError();
        FormatMessageA(0x12ff,(LPCVOID)0x0,DVar2,0,local_2808,0x800,(va_list *)0x0);
        SetLastError(0);
      }
      else {
        DAT_00721438 = (HANDLE)0x0;
      }
      DAT_00721444 = -1;
      DAT_00721448 = 0;
    }
    ReleaseMutex((HANDLE)*DAT_0072143c);
  }
  DAT_00721447 = 0;
  return;
}
#endif
