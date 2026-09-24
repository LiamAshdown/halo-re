// playlist_profile_create_default_profiles_on_disk  (Ghidra: playlist_profile_create_default_profiles_on_disk, already named)
// address 0x53bc70, size 607 bytes
// name confidence: 0.85   rewrite confidence: 0.7
// evidence: already named by Ghidra/CEA (__cdecl, no parameters). out/phase4/
// saved_games_functions.md summary "Writes out the full set of built-in default multiplayer
// playlist/game-variant files (one blam.lst each) to the default_playlist folder."
// tag_lookup's second (EDI) argument is dropped from the pack.py decompile the same way it is
// everywhere else in this module (see saved_game_allocate_new_slot.c, which independently
// confirmed 'ustr' 0x75737472 via objdump for the sibling ui\\saved_game_file_strings lookup);
// the same group is used here for consistency, the string being another ui\\... ustr tag.
// The string-list read (tag data's own TagReflexive at offset 0: count then element pointer,
// element stride 0x14 = TagDataOffset) matches types/tags.h UnicodeStringList /
// UnicodeStringListString exactly, unlike saved_game_allocate_new_slot.c's ui\\saved_game_file_
// strings lookup (which reads its reflexive from +0x28 of a differently-shaped tag), so it is
// written with those named types here instead of raw offsets.
// The path-buffer append before file_reference_create is byte-identical to file_reference_init
// (zero, signature, location 2, the is-file-bit-gated path_remove_last_component, append,
// set the is-file bit) but the binary does NOT call that function here -- it is inlined, exactly
// as in player_profile_write_default_files.c (0x53a610), including that same function's
// "always false right after the fresh zero" dead branch; reproduced literally per that file's
// precedent rather than replaced with a call.
// 0x00671fac is the L"<missing string>" array itself (mov ebp,0x671fac), used when the
// ustr list has no string for the index.
// Phase 4 review: same inlined path_append_component and L"<missing string>" fixes as
// saved_game_index_register_default_playlists.c.
// register convention: __cdecl, no parameters.
// reconciled: R37 game_variant.unknown_94 -> uint16 variant_flags (bit 0 built-in, high byte default index)

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
extern uint8_t savegame_index_dirty; // 0x00721447
extern char default_playlists_directory[0x100]; // 0x00721a49
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac, the characters of L"<missing string>" (an array, not a pointer: mov reg,0x671fac; src/game uses the same name)
extern game_variant_defaults_proc default_game_variant_procs[k_default_game_variant_count]; // 0x0069e838

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void directory_ensure_empty(const char *directory_path); // 0x555520, this module
extern void path_remove_last_component(char *path); // 0x555f80, this module
extern uint8_t file_reference_create(file_reference_record *ref); // 0x5555b0, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // 0x5558f0, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module

extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern int32_t __snprintf(char *dest, uint32_t count, const char *format, ...); // CRT
extern void _strncpy(char *dest, const char *source, uint32_t count); // CRT
extern void _strncat(char *dest, const char *source, uint32_t count); // CRT
extern void _wcsncpy(uint16_t *dest, const uint16_t *source, uint32_t count); // CRT

// blam-cc: __cdecl, no parameters
// For each of the k_default_game_variant_count built-in variant builders: builds the default
// game_variant, writes it under default_playlists_directory\NN\blam.lst (creating/emptying the
// NN directory first), gives it a localized display name from the ui\\default_multiplayer_
// game_setting_names ustr tag's string list (index i, falling back to default_ustr_fallback_
// string if that index is out of range or empty), stamps the built-in index into the high byte
// of variant_flags, and crcs and writes the file. Counts successful writes into
// default_game_variant_count and marks the save-game index dirty once done. No-ops if the ustr
// tag isn't found.
void playlist_profile_create_default_profiles_on_disk(void)
{
    datum_index tag_id;
    int16_t i;
    game_variant *defaults_result;
    game_variant variant_scratch;
    char path[256];
    UnicodeStringList *name_list;
    UnicodeStringListString *entry;
    uint16_t *source_name;
    uint32_t source_size;
    game_variant_file variant_file;
    file_reference_record ref;
    char *end;
    uint8_t written;

    tag_id = tag_lookup(0x75737472, "ui\\default_multiplayer_game_setting_names"); // 'ustr'
    if (tag_id == k_datum_index_none) {
        return;
    }

    for (i = 0; i < (int16_t)k_default_game_variant_count; i++) {
        // The callback fills variant_scratch and returns the same pointer in EAX (see the
        // game_variant_defaults_proc typedef); the binary still copies through EAX rather than
        // using variant_scratch's own address, so that copy is preserved literally.
        defaults_result = default_game_variant_procs[i](&variant_scratch);
        memcpy(&variant_file.variant, defaults_result, sizeof(variant_file.variant));

        __snprintf(path, 0xff, "%s\\%02d", default_playlists_directory, i);
        directory_ensure_empty(path);
        _strncat(path, "\\blam.lst", 0xff);

        source_name = missing_string_text;
        name_list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
        if (0 <= i && i < (int32_t)name_list->strings.count) {
            entry = (UnicodeStringListString *)name_list->strings.pointer + i;
            source_size = entry->string.size;
            if (0 < (int32_t)source_size) {
                source_name = (uint16_t *)entry->string.pointer;
                // Zeroes the last UTF-16 character of the shared tag string data in place.
                *(uint16_t *)((uint8_t *)source_name + ((source_size & 0xfffffffe) - 2)) = 0;
            }
        }

        _wcsncpy(variant_file.variant.name, source_name, 0x17);
        variant_file.variant.name[0x17] = 0;
        variant_file.variant.variant_flags =
            (int16_t)((uint16_t)variant_file.variant.variant_flags | ((uint16_t)(uint8_t)i << 8));

        variant_file.checksum = 0xffffffff;
        crc32_update(&variant_file.checksum, &variant_file.variant, sizeof(variant_file.variant));

        // Manual file_reference_record construction, kept inlined rather than calling
        // file_reference_init -- see the header comment and player_profile_write_default_files.c.
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

        written = 0;
        if (file_reference_create(&ref) != 0 && file_reference_open(&ref, 2) != 0 &&
            file_reference_seek(0, &ref) != 0) {
            written = file_reference_write(&ref, &variant_file, sizeof(variant_file));
            file_reference_close(&ref);
        }
        if (written != 0) {
            default_game_variant_count = default_game_variant_count + 1;
        }
    }
    savegame_index_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x53bc70):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void __cdecl playlist_profile_create_default_profiles_on_disk(void)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  char cVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined **_Source;
  char *pcVar7;
  int iVar8;
  char *_Dest;
  undefined4 *puVar9;
  wchar_t *pwVar10;
  int local_234c;
  undefined4 uStack_2348;
  byte bStack_2344;
  undefined2 uStack_2342;
  char acStack_2340 [255];
  undefined1 uStack_2241;
  char acStack_2238 [255];
  undefined1 uStack_2139;
  undefined4 auStack_2138 [38];
  undefined1 local_20a0 [152];
  wchar_t awStack_2008 [23];
  undefined2 uStack_1fda;
  ushort uStack_1f74;
  undefined4 auStack_1f70 [2009];
  undefined4 uStack_c;

  uStack_c = 0x53bc80;
  uVar5 = tag_lookup("ui\\default_multiplayer_game_setting_names");
  if (uVar5 != 0xffffffff) {
    local_234c = 0;
    do {
      puVar6 = (undefined4 *)
               (*(code *)(&PTR_game_engine_variant_defaults_classic_slayer_0069e838)[local_234c])
                         (local_20a0);
      puVar9 = auStack_2138;
      for (iVar8 = 0x26; iVar8 != 0; iVar8 = iVar8 + -1) {
        *puVar9 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar9 = puVar9 + 1;
      }
      __snprintf(acStack_2238,0xff,"%s\\%02d",&DAT_00721a49,local_234c);
      uStack_2139 = 0;
      FUN_00555520();
      _strncat(acStack_2238,"\\blam.lst",0xff);
      uStack_2139 = 0;
      piVar2 = *(int **)(DAT_0087bc14 + (uVar5 & 0xffff) * 0x20 + 0x14);
      _Source = &PTR_DAT_00671fac;
      if ((-1 < (short)local_234c) && ((int)(short)local_234c < *piVar2)) {
        puVar1 = (uint *)(piVar2[1] + (short)local_234c * 0x14);
        uVar3 = *puVar1;
        if (0 < (int)uVar3) {
          _Source = (undefined **)puVar1[3];
          *(undefined2 *)((int)_Source + ((uVar3 & 0xfffffffe) - 2)) = 0;
        }
      }
      puVar6 = auStack_2138;
      pwVar10 = awStack_2008;
      for (iVar8 = 0x26; iVar8 != 0; iVar8 = iVar8 + -1) {
        *(undefined4 *)pwVar10 = *puVar6;
        puVar6 = puVar6 + 1;
        pwVar10 = pwVar10 + 2;
      }
      _wcsncpy(awStack_2008,(wchar_t *)_Source,0x17);
      uStack_1fda = 0;
      uStack_1f74 = uStack_1f74 | (ushort)(byte)local_234c << 8;
      auStack_1f70[0] = 0xffffffff;
      crc32_update(auStack_1f70,awStack_2008,0x98);
      puVar6 = &uStack_2348;
      for (iVar8 = 0x43; iVar8 != 0; iVar8 = iVar8 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      uStack_2348 = 0x66696c6f;
      uStack_2342 = 2;
      if ((bStack_2344 & 1) != 0) {
        path_remove_last_component();
      }
      if (acStack_2238[0] != '\0') {
        pcVar7 = acStack_2340;
        do {
          _Dest = pcVar7;
          pcVar7 = _Dest + 1;
        } while (*_Dest != '\0');
        if (_Dest != acStack_2340) {
          *_Dest = '\\';
          *pcVar7 = '\0';
          _Dest = pcVar7;
        }
        pcVar7 = acStack_2340;
        do {
          cVar4 = *pcVar7;
          pcVar7 = pcVar7 + 1;
        } while (cVar4 != '\0');
        _strncpy(_Dest,acStack_2238,0xff - ((int)pcVar7 - (int)(acStack_2340 + 1)));
        uStack_2241 = 0;
      }
      bStack_2344 = bStack_2344 | 1;
      cVar4 = file_reference_create();
      if (((cVar4 != '\0') && (cVar4 = file_reference_open(2), cVar4 != '\0')) &&
         (cVar4 = file_reference_seek(), cVar4 != '\0')) {
        cVar4 = file_reference_write();
        file_reference_close();
        if (cVar4 == '\x01') {
          DAT_00721328._0_2_ = (short)DAT_00721328 + 1;
        }
      }
      local_234c = local_234c + 1;
    } while (local_234c < 0x26);
    DAT_00721447 = 1;
  }
  return;
}
#endif
