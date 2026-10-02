// saved_game_index_register_default_profiles  (Ghidra: saved_game_index_register_default_profiles,
// already named)
// address 0x53dde0, size 633 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_functions.md summary "Validates
// and re-registers each built-in default player profile's .sav file against its expected tag
// data into the shared save-game index." Structurally identical to its sibling
// saved_game_index_register_default_playlists.c (0x53db40) except: the tag is ui\\shell\\
// strings\\default_player_profile_names, the loop runs a fixed k_default_player_profile_count
// (2) times rather than default_game_variant_count, the path is "%02d.sav" under
// default_player_profiles_directory, the body is a saved_player_profile (crc over
// k_saved_player_profile_size, 0x1ffc), and entry.type is 0 (_saved_game_type_player_profile).
// Ghidra's `local_c` (the crc-comparison target) is the same stack slot the __chkstk return-
// address marker occupies at function entry, later overwritten by the checksum field once
// file_reference_read fills the body buffer over it -- read here directly as
// saved_player_profile_file.checksum, matching the sibling function's equivalent slot.
// UNSURE: as in the sibling function, on either in-loop failure (index-count overflow, index
// write failure) the binary returns the CURRENT loop index, not the running processed count;
// reproduced with the same two separate locals.
// Phase 4 review: same inlined path_append_component and L"<missing string>" fixes as
// saved_game_index_register_default_playlists.c; the 0x2000-byte read matches blam.sav exactly.
// register convention: no parameters (already resolved: void).

#include "crt.h"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac, the characters of L"<missing string>" (an array, not a pointer: mov reg,0x671fac; src/game uses the same name)
extern char default_player_profiles_directory[0x100]; // 0x00721849
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

// blam-cc: no parameters
// For each built-in default profile index 0..k_default_player_profile_count-1: builds
// default_player_profiles_directory\NN.sav's path, and if that file exists, registers a
// saved_game_index_entry for it (path, a localized display name from the ui\\shell\\strings\\
// default_player_profile_names ustr tag with the same fallback/strip-last-char handling as
// saved_game_index_register_default_playlists.c, type = player_profile, builtin = 1,
// checksum_valid set if the file's own body crc matches its stored checksum), consuming an
// index-file slot. Returns the loop index reached (early, on an index-count overflow or a write
// failure) or the count of indices iterated (on normal completion).
int16_t saved_game_index_register_default_profiles(void)
{
    datum_index tag_id;
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
    saved_player_profile_file file;
    uint8_t written;

    tag_id = tag_lookup(0x75737472, (char *)"ui\\shell\\strings\\default_player_profile_names"); // 'ustr'
    i = 0;
    last = 0;
    if (tag_id != k_datum_index_none) {
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

            _snprintf(path, 0xff, "%s\\%02d.sav", default_player_profiles_directory, (int32_t)i);

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
                strncpy(end, path, 0xff - (uint32_t)strlen(ref.path));
                ref.path[0xff] = '\0';
            }
            ref.flags |= _file_reference_is_file_bit;

            exists = file_reference_exists(&ref);
            if (exists != 0) {
                memset(&entry, 0, sizeof(entry));
                strncpy(entry.path, path, 0xff);
                wcsncpy((wchar_t *)entry.display_name, (const wchar_t *)source_name, 0x7f);
                entry.type = _saved_game_type_player_profile;
                entry.builtin = 1;

                opened = file_reference_open(&ref, 1);
                if (opened != 0) {
                    read_ok = file_reference_read(&ref, &file, sizeof(file));
                    if (read_ok != 0) {
                        checksum = 0xffffffff;
                        crc32_update(&checksum, &file.profile, sizeof(file.profile));
                        if (checksum == file.checksum) {
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
        } while (i < (int16_t)k_default_player_profile_count);
    }
    return last;
}

#if 0
Original Ghidra decompilation (0x53dde0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

short saved_game_index_register_default_profiles(void)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  char cVar4;
  char *pcVar5;
  int iVar6;
  char *_Dest;
  undefined **_Source;
  short sVar7;
  short sVar8;
  undefined4 *puVar9;
  int local_2428;
  uint local_2424;
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
  undefined1 local_2008 [8188];
  int local_c;

  local_c = 0x53ddf0;
  local_2424 = tag_lookup("ui\\shell\\strings\\default_player_profile_names");
  sVar7 = 0;
  sVar8 = 0;
  if (local_2424 != 0xffffffff) {
    do {
      piVar2 = *(int **)((local_2424 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      _Source = &PTR_DAT_00671fac;
      if ((-1 < sVar7) && ((int)sVar7 < *piVar2)) {
        puVar1 = (uint *)(piVar2[1] + sVar7 * 0x14);
        uVar3 = *puVar1;
        if (0 < (int)uVar3) {
          _Source = (undefined **)puVar1[3];
          *(undefined2 *)((int)_Source + ((uVar3 & 0xfffffffe) - 2)) = 0;
        }
      }
      __snprintf(local_2108,0xff,"%s\\%02d.sav",&DAT_00721849,(int)sVar7);
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
          cVar4 = *pcVar5;
          pcVar5 = pcVar5 + 1;
        } while (cVar4 != '\0');
        _strncpy(_Dest,local_2108,0xff - ((int)pcVar5 - (int)(local_2418 + 1)));
        local_2319 = 0;
      }
      local_241c = local_241c | 1;
      cVar4 = file_reference_exists();
      if (cVar4 != '\0') {
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
        local_2110 = 0;
        local_210c = 1;
        cVar4 = file_reference_open(1);
        if (cVar4 != '\0') {
          cVar4 = file_reference_read();
          if (cVar4 != '\0') {
            local_2428 = -1;
            crc32_update(&local_2428,local_2008,0x1ffc);
            if (local_2428 == local_c) {
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
        cVar4 = file_reference_write();
        if (cVar4 == '\0') {
          return sVar7;
        }
      }
      sVar7 = sVar7 + 1;
      sVar8 = sVar7;
    } while (sVar7 < 2);
  }
  return sVar8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
