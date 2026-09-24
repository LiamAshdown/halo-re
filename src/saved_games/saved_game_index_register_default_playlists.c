// saved_game_index_register_default_playlists  (Ghidra: saved_game_index_register_default_playlists,
// already named)
// address 0x53db40, size 667 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_functions.md summary "Validates
// and re-registers each built-in default playlist's blam.lst against its expected tag data into
// the shared save-game index." Shares the tag-name-lookup pattern (ui\\default_multiplayer_
// game_setting_names, same fallback global and stripped-last-character handling) already
// established in playlist_profile_create_default_profiles_on_disk.c (0x53bc70), and the same
// saved_game_index_entry layout established throughout this module (type at 0x200, index at
// 0x202, builtin at 0x204, checksum_valid at 0x205), confirming entry.type = 1
// (_saved_game_type_game_variant) and entry.builtin = 1 here.
// UNSURE: on either failure inside the loop (index-count overflow, index-file write failure) the
// binary returns the CURRENT loop index (before it would have been incremented), not the
// running "entries processed" counter the normal loop exit returns; reproduced exactly as two
// distinct local variables, not unified.
// Phase 4 review (objdump 0x53db40..0x53ddda): the body read is a fixed 0x2000-byte request
// (0x53dd21), the size every blam.lst writer produces (the first rewrite
// read 0x9c bytes into a 0x98-byte local, a stack overrun); the inlined path_append_component
// writes one separator and bounds the copy by the new length (the first rewrite advanced twice);
// 0x00671fac is the L"<missing string>" array, not a pointer.
// register convention: no parameters (already resolved: void).

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int16_t default_game_variant_count; // 0x00721328
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac, the characters of L"<missing string>" (an array, not a pointer: mov reg,0x671fac; src/game uses the same name)
extern char default_playlists_directory[0x100]; // 0x00721a49
extern int16_t savegame_index_write_count; // 0x00721444
extern file_reference_record savegame_index_file; // 0x00721330

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void path_remove_last_component(char *path); // 0x555f80, this module
extern uint8_t file_reference_exists(file_reference_record *ref); // 0x555720, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern int32_t __snprintf(char *dest, uint32_t count, const char *format, ...); // CRT
extern void _strncpy(char *dest, const char *source, uint32_t count); // CRT
extern void _wcsncpy(uint16_t *dest, const uint16_t *source, uint32_t count); // CRT

// blam-cc: no parameters
// For each built-in playlist index 0..default_game_variant_count-1: builds
// default_playlists_directory\NN\blam.lst's path, and if that file exists, registers a
// saved_game_index_entry for it (path, a localized display name from the ui\\default_
// multiplayer_game_setting_names ustr tag with the same fallback/strip-last-char handling as
// playlist_profile_create_default_profiles_on_disk.c, type = game_variant, builtin = 1,
// checksum_valid set if the file's own body crc matches its stored checksum), consuming an
// index-file slot. Returns the loop index reached (early, on an index-count overflow or a write
// failure) or the count of indices iterated (on normal completion).
int16_t saved_game_index_register_default_playlists(void)
{
    datum_index tag_id;
    int16_t count;
    int16_t i;
    int16_t last;
    UnicodeStringList *name_list;
    UnicodeStringListString *string_entry;
    uint16_t *source_name;
    uint32_t source_size;
    char path[256];
    file_reference_record ref;
    char *end;
    uint8_t exists;
    saved_game_index_entry entry;
    uint8_t opened;
    uint8_t read_ok;
    uint32_t checksum;
    uint8_t body[0x2000];
    uint8_t written;

    count = default_game_variant_count;
    tag_id = tag_lookup(0x75737472, "ui\\default_multiplayer_game_setting_names"); // 'ustr'
    i = 0;
    last = 0;
    if (tag_id != k_datum_index_none && 0 < count) {
        name_list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
        do {
            source_name = missing_string_text;
            if (0 <= i && i < (int32_t)name_list->strings.count) {
                string_entry = (UnicodeStringListString *)name_list->strings.pointer + i;
                source_size = string_entry->string.size;
                if (0 < (int32_t)source_size) {
                    source_name = (uint16_t *)string_entry->string.pointer;
                    *(uint16_t *)((uint8_t *)source_name + ((source_size & 0xfffffffe) - 2)) = 0;
                }
            }

            __snprintf(path, 0xff, "%s\\%02d\\blam.lst", default_playlists_directory, (int32_t)i);

            memset(&ref, 0, sizeof(ref));
            ref.signature = k_file_reference_signature;
            ref.location = _file_location_absolute;
            if ((ref.flags & _file_reference_is_file_bit) != 0) {
                path_remove_last_component(ref.path); // unreachable: ref.flags was just zeroed above
            }
            if (path[0] != '\0') {
                // inlined path_append_component: one separator, then the bound is recomputed
                // from the new length and the last byte is forced to 0
                end = ref.path + strlen(ref.path);
                if (end != ref.path) {
                    *end = '\\';
                    end++;
                    *end = '\0';
                }
                _strncpy(end, path, 0xff - (uint32_t)strlen(ref.path));
                ref.path[0xff] = '\0';
            }
            ref.flags |= _file_reference_is_file_bit;

            exists = file_reference_exists(&ref);
            if (exists != 0) {
                memset(&entry, 0, sizeof(entry));
                _strncpy(entry.path, path, 0xff);
                _wcsncpy(entry.display_name, source_name, 0x7f);
                entry.type = _saved_game_type_game_variant;
                entry.builtin = 1;

                opened = file_reference_open(&ref, 1);
                if (opened != 0) {
                    // always a 0x2000-byte request (mov esi,0x2000 at 0x53dd21), the size
                    // every blam.lst writer produces
                    read_ok = file_reference_read(&ref, body, sizeof(body));
                    if (read_ok != 0) {
                        checksum = 0xffffffff;
                        crc32_update(&checksum, body, sizeof(game_variant));
                        if (checksum == *(uint32_t *)(body + sizeof(game_variant))) {
                            entry.checksum_valid = 1;
                        }
                    }
                    file_reference_close(&ref);
                }
                if (0x3e6 < savegame_index_write_count) {
                    return i;
                }
                entry.index = savegame_index_write_count;
                savegame_index_write_count = savegame_index_write_count + 1;
                written = file_reference_write(&savegame_index_file, &entry, sizeof(entry));
                if (written == 0) {
                    return i;
                }
            }
            i = i + 1;
            last = i;
        } while (i < count);
    }
    return last;
}

#if 0
Original Ghidra decompilation (0x53db40):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

short saved_game_index_register_default_playlists(void)

{
  uint *puVar1;
  undefined4 uVar2;
  char cVar3;
  uint uVar4;
  char *pcVar5;
  int iVar6;
  char *_Dest;
  undefined **_Source;
  short sVar7;
  short sVar8;
  undefined4 *puVar9;
  int local_242c;
  undefined4 local_2428;
  int local_2424;
  undefined4 local_2420;
  byte local_241c;
  undefined2 local_241a;
  char local_2418 [255];
  undefined1 local_2319;
  char local_2310;
  undefined4 local_230f [63];
  undefined1 local_2211;
  wchar_t local_2210 [127];
  undefined2 local_2112;
  undefined2 local_2110;
  short local_210e;
  undefined1 local_210c;
  undefined1 local_210b;
  char local_2108 [256];
  undefined1 local_2008 [152];
  int local_1f70;
  undefined4 uStack_c;

  uVar2 = DAT_00721328;
  uStack_c = 0x53db50;
  local_2428 = DAT_00721328;
  uVar4 = tag_lookup("ui\\default_multiplayer_game_setting_names");
  sVar7 = 0;
  sVar8 = 0;
  if ((uVar4 != 0xffffffff) && (0 < (short)uVar2)) {
    local_2424 = (uVar4 & 0xffff) * 0x20 + 0x14;
    do {
      _Source = &PTR_DAT_00671fac;
      if ((-1 < sVar7) && ((int)sVar7 < **(int **)(local_2424 + DAT_0087bc14))) {
        puVar1 = (uint *)((*(int **)(local_2424 + DAT_0087bc14))[1] + sVar7 * 0x14);
        uVar4 = *puVar1;
        if (0 < (int)uVar4) {
          _Source = (undefined **)puVar1[3];
          *(undefined2 *)((int)_Source + ((uVar4 & 0xfffffffe) - 2)) = 0;
        }
      }
      __snprintf(local_2108,0xff,"%s\\%02d\\blam.lst",&DAT_00721a49,(int)sVar7);
      puVar9 = &local_2420;
      for (iVar6 = 0x43; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar9 = 0;
        puVar9 = puVar9 + 1;
      }
      local_2420 = 0x66696c6f;
      local_241a = 2;
      if ((local_241c & 1) != 0) {
        path_remove_last_component();
      }
      if (local_2108[0] != '\0') {
        pcVar5 = local_2418;
        do {
          _Dest = pcVar5;
          pcVar5 = _Dest + 1;
        } while (*_Dest != '\0');
        if (_Dest != local_2418) {
          *_Dest = '\\';
          *pcVar5 = '\0';
          _Dest = pcVar5;
        }
        pcVar5 = local_2418;
        do {
          cVar3 = *pcVar5;
          pcVar5 = pcVar5 + 1;
        } while (cVar3 != '\0');
        _strncpy(_Dest,local_2108,0xff - ((int)pcVar5 - (int)(local_2418 + 1)));
        local_2319 = 0;
      }
      local_241c = local_241c | 1;
      cVar3 = file_reference_exists();
      if (cVar3 != '\0') {
        local_2310 = '\0';
        puVar9 = local_230f;
        for (iVar6 = 0x81; iVar6 != 0; iVar6 = iVar6 + -1) {
          *puVar9 = 0;
          puVar9 = puVar9 + 1;
        }
        *(undefined1 *)puVar9 = 0;
        _strncpy(&local_2310,local_2108,0xff);
        local_2211 = 0;
        _wcsncpy(local_2210,(wchar_t *)_Source,0x7f);
        local_2112 = 0;
        local_2110 = 1;
        local_210c = 1;
        cVar3 = file_reference_open(1);
        if (cVar3 != '\0') {
          cVar3 = file_reference_read();
          if (cVar3 != '\0') {
            local_242c = -1;
            crc32_update(&local_242c,local_2008,0x98);
            if (local_242c == local_1f70) {
              local_210b = 1;
            }
          }
          file_reference_close();
        }
        if (0x3e6 < DAT_00721444) {
          return sVar7;
        }
        local_210e = DAT_00721444;
        DAT_00721444 = DAT_00721444 + 1;
        cVar3 = file_reference_write();
        if (cVar3 == '\0') {
          return sVar7;
        }
      }
      sVar7 = sVar7 + 1;
      sVar8 = sVar7;
    } while (sVar7 < (short)local_2428);
  }
  return sVar8;
}
#endif
