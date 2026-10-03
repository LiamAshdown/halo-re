#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "halo/saved_games/saved_games.hpp"
#include "halo/sound/api.hpp"

extern "C" {
extern char savegames_directory[0x100];
extern uint16_t missing_string_text[];
extern tag_instance *tag_instances;
extern datum_index tag_lookup(tag_group group, char *path);
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...);
extern uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path,
    uint32_t out_path_size);
extern int32_t savegame_find_first(const char *root, void *out_find_data);
extern uint8_t savegame_find_next(void *out_find_data, int32_t handle);
extern uint8_t user_save_path_remove(int32_t handle);
extern game_variant *game_engine_variant_defaults_classic_slayer(game_variant *out);
extern void game_variant_sanitize_options(game_variant *variant);
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size);
extern uint8_t savegame_index_dirty;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern int32_t savegame_index_get_slot_count(void);
extern uint32_t savegame_slot_handle_pack(uint32_t slot_index, uint32_t type_nibble, uint8_t flag_bit_30,
    uint8_t flag_bit_31);
extern uint8_t savegame_index_append_slot(const saved_game_index_entry *entry, int32_t *out_slot);
extern uint32_t XDeleteSaveGame(const uint16_t *save_game_name, const char *root_path);
extern saved_player_profile default_profile_data;
extern void input_apply_named_device_default_profile(const uint16_t *name);
extern int32_t cached_saved_game_something;
extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry);
extern uint8_t savegame_index_remove_slot(int32_t slot_index);
extern int32_t saved_player_profile_slots_handle;
extern network_mutex_record *saved_game_files_mutex;
extern network_mutex_record *savegame_index_mutex;
extern file_reference_record savegame_index_file;
extern uint8_t savegame_index_file_exists(void);
extern uint8_t saved_game_files_initialized;
extern char profile_directory[0x105];
extern char saved_game_root_directory[0x100];
extern char saved_game_root_path[0x100];
extern char saved_directory[0x100];
extern char player_profiles_directory[0x100];
extern char default_player_profiles_directory[0x100];
extern char playlists_directory[0x100];
extern char default_playlists_directory[0x100];
extern char last_profile_path[0x100];
extern char last_game_variant_path[0x100];
extern char last_multiplayer_map_path[0x100];
extern uint8_t default_player_profile_initialized;
extern variant_write_request variant_write_request_state;
extern int16_t default_game_variant_count;
extern uint8_t unknown_0072132a;
extern int32_t mutex_create(network_mutex_record **out_handle);
extern char directory_create_recursive(char *path);
extern uint16_t saved_game_display_name_buffer[0x80];
extern network_thread_record *variant_write_thread;
extern int16_t savegame_index_write_count;
extern uint8_t saved_game_index_file_open;
extern uint32_t game_state_crc;
extern int16_t local_player_count;
extern uint32_t cache_file_current_header_crc32;
extern datum_index global_scenario_index;
extern char *rasterizer_shader_file_name;
extern int32_t strcmp(const char *a, const char *b);
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal);
}

namespace halo::saved_games::saved_game {

/**
 * Looks up the ui\\saved_game_file_strings ustr tag and, for each candidate slot number 1..999,
 * formats "<string> <number>" (bounded to 0x7f characters) into out_name and asks
 * XCreateSaveGame (mode 3, an existence query) whether that name is free. Stops at the first
 * free number; clears out_name if all 999 are taken (or the tag lookup failed).
 *
 * @address 0x0053ca80
 */
void allocate_new_slot(uint16_t *out_name)
{
    datum_index tag_index;
    UnicodeStringList *string_list;
    UnicodeStringListString *string_entry;
    uint16_t *format_string;
    uint32_t string_size;
    int32_t number;
    int32_t next_number;
    uint32_t create_result;
    char scratch_path[0x100];

    out_name[0] = 0;
    tag_index = tag_lookup('ustr', (char *)"ui\\saved_game_file_strings");
    if (tag_index != -1) {
        memset(scratch_path, 0, sizeof(scratch_path));
        number = 0;
        do {
            string_list = (UnicodeStringList *)tag_instances[tag_index & 0xffff].data;
            format_string = missing_string_text;
            if (2 < (int32_t)string_list->strings.count) {
                string_entry = (UnicodeStringListString *)string_list->strings.pointer + 2;
                string_size = string_entry->string.size;
                if (0 < (int32_t)string_size) {
                    format_string = (uint16_t *)string_entry->string.pointer;
                    *(uint16_t *)((uint8_t *)format_string + (string_size >> 1) * 2 - 2) = 0;
                }
            }
            next_number = number + 1;
            string_format_wide_va_bounded(0x7f, out_name, format_string, next_number);
            out_name[0x7f] = 0;
            create_result = XCreateSaveGame(out_name, savegames_directory, 3, scratch_path, 0x100);
            if (create_result != 0) {
                break;
            }
            number = next_number;
        } while (number < 999);
        if (number == 999) {
            out_name[0] = 0;
        }
    }
    return;
}

/**
 * Returns _saved_game_storage_ok if there is at least 0x2800000 bytes of free disk space.
 * Otherwise counts the entries under savegames_directory (up to 999); if there are already at
 * least 999, returns _saved_game_storage_too_many_saves, else _saved_game_storage_ok (0 by way
 * of the always-zero EBX return, matching the binary).
 *
 * @address 0x0053d120
 */
int32_t check_storage_availability(void)
{
    uint64_t free_bytes_available;
    uint64_t total_bytes;
    uint64_t total_free_bytes;
    int32_t ok;
    int32_t handle;
    int32_t count;
    uint8_t find_data[0x344];
    uint8_t found;
    uint8_t removed;

    ok = GetDiskFreeSpaceExA(savegames_directory, (PULARGE_INTEGER)&free_bytes_available, (PULARGE_INTEGER)&total_bytes, (PULARGE_INTEGER)&total_free_bytes);
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

/**
 * (target_name)
 * If "<source_directory><source_name>.sav" exists, copies both the .bin and .sav files of
 * source_name onto target_name (both within source_directory). Used for autosave rotation
 * (copy the previous newest autosave to autosave1, then copy the just-written save onto the
 * newest autosave slot) and for checkpoint loading.
 *
 * @address 0x005387e0
 */
uint8_t copy_files_to_target(char *source_directory, char *source_name, char *target_name)
{
    char check_path[256];
    win32_find_dataa find_data;
    void *find_handle;
    char source_path[256];
    char target_path[256];

    sprintf(check_path, "%s%s.sav", source_directory, source_name);
    find_handle = FindFirstFileA(check_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle == (void *)0xffffffff) {
        return 0;
    }
    FindClose(find_handle);

    sprintf(target_path, "%s%s.bin", source_directory, target_name);
    sprintf(source_path, "%s%s.bin", source_directory, source_name);
    CopyFileA(source_path, target_path, 0);

    sprintf(source_path, "%s%s.sav", source_directory, source_name);
    sprintf(target_path, "%s%s.sav", source_directory, target_name);
    CopyFileA(source_path, target_path, 0);
    return 1;
}

/**
 * Creates a new game-variant slot named name, opens its file, fills it with the classic-slayer
 * default options (variant_flags's built-in bit cleared, since this is a user-created variant),
 * sanitizes it, stamps name over its name field, crcs and writes it, and rolls back (deletes the
 * slot) if the seek or write fails. Returns the new slot's packed handle, or 0xffffffff on any
 * failure.
 *
 * @address 0x0053bb50
 */
uint32_t create_custom_variant(uint32_t unused, uint16_t *name)
{
    uint32_t handle;
    file_reference_record ref;
    game_variant_file file;
    game_variant *defaults_ptr;
    uint8_t opened;
    uint8_t seeked;
    uint8_t written;

    (void)unused;

    handle = saved_game_create_slot(1, name);
    if (handle == 0xffffffff) {
        return 0xffffffff;
    }

    opened = saved_game_open_file_by_handle((int32_t)handle, &ref);
    if (opened != 0) {
        memset(&file, 0, sizeof(file));
        defaults_ptr = game_engine_variant_defaults_classic_slayer((game_variant *)&file.variant);
        memcpy(&file.variant, defaults_ptr, sizeof(file.variant));
        file.variant.variant_flags = (int16_t)((uint16_t)file.variant.variant_flags & 0xfffe);
        game_variant_sanitize_options(&file.variant);
        wcsncpy((wchar_t *)file.variant.name, (const wchar_t *)name, 0x17);
        file.variant.name[0x17] = 0;
        file.checksum = 0xffffffff;
        crc32_update(&file.checksum, &file.variant, sizeof(file.variant));

        seeked = file_reference_seek(0, &ref);
        if (seeked == 0 || (written = file_reference_write(&ref, &file, sizeof(file)), written == 0)) {
            saved_game_delete_by_handle((int32_t)handle);
            handle = 0xffffffff;
        }
        file_reference_close(&ref);
        return handle;
    }
    saved_game_delete_by_handle((int32_t)handle);
    return 0xffffffff;
}

/**
 * Creates a new player-profile save slot named `name`, writes a freshly initialized default
 * profile into it, and returns its handle. Rolls the slot back (deleting it) if the file
 * couldn't be opened or written.
 *
 * @address 0x00539ab0
 */
uint32_t create_default_profile(uint16_t *name)
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
    ((void (*)(uint32_t *crc, uint8_t *data, int32_t length))crc32_update)(&file.checksum, (uint8_t *)&file.profile, k_saved_player_profile_size);

    if (file_reference_seek(0, &ref) == 0 || file_reference_write(&ref, &file, sizeof(file)) == 0) {
        saved_game_delete_by_handle(handle);
        handle = 0xffffffff;
    }
    file_reference_close(&ref);
    return handle;
}

/**
 * Registers a new saved-game (type 0: blam.sav) or playlist (type 1: blam.lst) slot named
 * name. Checks storage availability and the 999-slot limit (reporting a UI error the first time
 * either trips), asks the Xbox save API for a fresh directory, builds and writes the empty
 * body+checksum file there (creating savegame.bin too for a profile), then registers the entry
 * via savegame_index_append_slot and packs its handle via savegame_slot_handle_pack. Rolls
 * back (XDeleteSaveGame) on any
 * failure after the directory was created. Returns the packed handle, or 0xffffffff on failure.
 *
 * @address 0x0053c660
 */
uint32_t create_slot(uint16_t type, uint16_t *name)
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
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x21;
            quit_confirm_error_unknown_ae = -1;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
    } else if (storage_status == 2 && quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x22;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    if (storage_status != 0) {
        return 0xffffffff;
    }

    entry_count = savegame_index_get_slot_count();
    if (0x3e6 < entry_count) {
        if (quit_confirm_error_string_index != -1) {
            return 0xffffffff;
        }
        quit_confirm_error_string_index = 0x24;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
        return 0xffffffff;
    }

    memset(directory, 0, sizeof(directory));
    create_result = XCreateSaveGame(name, savegames_directory, 1, directory, 0x100);
    if (create_result != 0) {
        return 0xffffffff;
    }

    memset(&entry, 0, sizeof(entry));
    wcsncpy((wchar_t *)entry.display_name, (const wchar_t *)name, 0x7f);
    entry.display_name[0x7f] = 0;
    entry.type = (int16_t)type;
    entry.index = (int16_t)entry_count;
    entry.builtin = 0;
    entry.checksum_valid = 0;

    saved_type = (int32_t)type;

    if (type == 0) {
        _snprintf(entry.path, 0xff, "%s%s", directory, "blam.sav");
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
        _snprintf(entry.path, 0xff, "%s%s", directory, "blam.lst");
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
                ok = file_reference_write(&ref, body, sizeof(body));
                if (ok) {
                    entry.checksum_valid = 1;
                }
                file_reference_close(&ref);
            }
            registered = savegame_index_append_slot(&entry, &appended_slot);
            if (registered != 0) {
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

/**
 * Converts the caller's narrow search name to a bounded (0x1ff character) wide string, then
 * enumerates up to 100 player-profile saved games. For each real profile whose flags have bit
 * 0x2 set and whose name matches the search string (case-insensitively), deletes that saved
 * game and applies the named default device profile. If nothing matches, still applies the
 * named default device profile before returning.
 *
 * @address 0x0053b9b0
 */
void delete_by_display_name(const char *name)
{
    int32_t handles[100];
    uint16_t capacity_and_count;
    uint16_t name_wide[0x200];
    int32_t length;
    int32_t i;
    saved_player_profile profile;
    uint8_t ok;
    int32_t handle;

    length = (int32_t)strlen(name);
    if ((uint32_t)(length * 2 + 2) > 0x400) {
        length = 0x1ff;
    }
    if ((uint32_t)(length * 2 + 2) < 0x401) {
        name_wide[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            name_wide[i] = (uint16_t)(uint8_t)name[i];
        }
    }

    capacity_and_count = 100;
    saved_game_enumerate_by_type(0, handles, 0, &capacity_and_count);
    for (i = 0; i < (int32_t)capacity_and_count; i++) {
        handle = handles[i];
        if (handle == -1) {
            profile = default_profile_data;
        } else {
            ok = player_profile_get(handle, &profile);
            if (ok != 0 && (profile.flags & 0x2) != 0 && _wcsicmp((const wchar_t *)name_wide, (const wchar_t *)profile.name) == 0) {
                if (handle != -1) {
                    saved_game_delete_by_handle(handle);
                }
                input_apply_named_device_default_profile(name_wide);
                return;
            }
        }
    }
    input_apply_named_device_default_profile(name_wide);
    return;
}

/**
 * Deletes the saved-game entry identified by handle, both from disk (via XDeleteSaveGame,
 * unless the handle's builtin bit is set) and from the in-memory index. Returns 1 on success, 0
 * if the index is dirty, the handle is out of range, the entry can't be found, the disk delete
 * fails, or the in-memory removal fails.
 *
 * @address 0x0053c960
 */
uint8_t delete_by_handle(int32_t handle)
{
    uint8_t result;
    uint32_t slot_index;
    saved_game_index_entry entry;
    uint8_t found;
    int32_t delete_result;
    uint8_t removed;

    result = 0;
    if (savegame_index_dirty == 0) {
        slot_index = ((uint32_t)handle >> 16) & 0xfff;
        result = 0;
        if ((handle & 0xf) < 2 && slot_index < 999) {
            found = savegame_index_read_slot((int32_t)slot_index, &entry);
            if (found != 0) {
                if ((handle & 0x40000000) == 0) {
                    result = 1;
                    delete_result = XDeleteSaveGame(entry.display_name, savegames_directory);
                    if (delete_result != 0) {
                        result = 0;
                    }
                }
                removed = savegame_index_remove_slot((int32_t)slot_index);
                if (removed == 0) {
                    result = 0;
                }
            }
        }
    }
    cached_saved_game_something = -1;
    return result;
}

/**
 * Deletes "<current profile directory><name>.bin" and "<name>.sav", but only if the .sav
 * exists. Returns whether either file was deleted.
 *
 * @address 0x005388c0
 */
uint8_t delete_files(char *name)
{
    char directory[264];
    char check_path[256];
    win32_find_dataa find_data;
    void *find_handle;
    char path[256];
    int32_t deleted_bin;
    int32_t deleted_sav;

    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(check_path, "%s%s.sav", directory, name);
    find_handle = FindFirstFileA(check_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle == (void *)0xffffffff) {
        return 0;
    }
    FindClose(find_handle);

    sprintf(path, "%s%s.bin", directory, name);
    deleted_bin = DeleteFileA(path);
    sprintf(path, "%s%s.sav", directory, name);
    deleted_sav = DeleteFileA(path);
    if (deleted_sav == 0 && deleted_bin == 0) {
        return 0;
    }
    return 1;
}

/**
 * Rebuilds the saved-game index first if it is marked dirty, then scans up to *capacity_and_count
 * index entries (rebuilding the count from savegame_index_get_slot_count (FUN_0053e420) as the scan bound) for ones matching
 * type, writing their packed handles into out_handles until either the scan bound or the
 * caller's capacity is reached. builtin_only selects whether non-builtin entries are skipped.
 * *capacity_and_count is always overwritten with the number of handles actually written (0 if
 * either mutex wait fails).
 * (parameters ordered as the callers declare them; EBX is bound by name)
 *
 * @address 0x0053c4e0
 */
void enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count)
{
    uint32_t wait_result;
    int32_t entry_count;
    int32_t written;
    uint8_t opened;
    int32_t i;
    saved_game_index_entry entry;
    uint8_t read_ok;
    int32_t handle;

    written = 0;
    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80) {
        if (savegame_index_dirty != 0) {
            saved_game_list_rebuild_index();
        }
        entry_count = savegame_index_get_slot_count();
        written = 0;
        wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);
        if (wait_result == 0 || wait_result == 0x80) {
            opened = savegame_index_file_exists();
            if (opened != 0) {
                i = 0;
                if (*capacity_and_count != 0) {
                    do {
                        if (entry_count <= i) {
                            break;
                        }
                        read_ok = file_reference_read(&savegame_index_file, &entry, sizeof(entry));
                        if (read_ok == 0) {
                            break;
                        }
                        if (entry.type == (int16_t)type &&
                            (builtin_only == 1 || entry.builtin == 0)) {
                            handle = savegame_slot_handle_pack(i, entry.type, entry.builtin, entry.checksum_valid);
                            out_handles[written] = handle;
                            written++;
                        }
                        i++;
                    } while (written < (int32_t)(uint32_t)*capacity_and_count);
                }
                file_reference_close(&savegame_index_file);
            }
            ReleaseMutex(savegame_index_mutex->handle);
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    *capacity_and_count = (uint16_t)written;
    return;
}

/**
 * Checks whether "<current profile directory><name>.sav" exists on disk.
 *
 * @address 0x00538770
 */
uint8_t file_exists(char *name)
{
    char directory[264];
    char path[832];
    win32_find_dataa find_data;
    void *find_handle;
    uint8_t found;

    found = 0;
    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(path, "%s%s.sav", directory, name);
    find_handle = FindFirstFileA(path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)0xffffffff) {
        found = 1;
        FindClose(find_handle);
    }
    return found;
}

/**
 * Closes and clears each of the two module mutexes (if created), waits for and clears the
 * background profile-verification and game-variant-write threads, and clears
 * saved_game_files_initialized.
 *
 * @address 0x0053c480
 */
void files_dispose(void)
{
    network_mutex_record *mutex;

    mutex = saved_game_files_mutex;
    if (saved_game_files_mutex != 0) {
        CloseHandle(mutex->handle);
        mutex->name[0] = 0;
        mutex->handle = 0;
        mutex->in_use = 0;
        saved_game_files_mutex = 0;
    }
    mutex = savegame_index_mutex;
    if (savegame_index_mutex != 0) {
        CloseHandle(mutex->handle);
        mutex->name[0] = 0;
        mutex->handle = 0;
        mutex->in_use = 0;
        savegame_index_mutex = 0;
    }
    player_profile_verify_thread_wait_and_clear();
    control_profile_variant_write_wait_and_clear();
    saved_game_files_initialized = 0;
}

/**
 * One-time module init: zeroes the saved-game-files globals block, copies profile_directory as
 * saved_game_root_directory and builds every path buffer under it, creates the saved/
 * player_profiles/default_profile/playlists/default_playlist directory tree, marks the index
 * dirty, creates the files and index mutexes (leaving saved_game_files_initialized false if
 * either fails), builds and marks-initialized the default (unsaved) player profile, writes the
 * two default profile files to disk, and resets the game-variant write-request block.
 *
 * @address 0x0053c260
 */
void files_initialize(void)
{
    uint8_t *zero_cursor;
    int32_t i;
    int32_t mutex1_ok;
    int32_t mutex2_ok;

    zero_cursor = (uint8_t *)&savegame_index_file;
    for (i = 0x2c7; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }

    strncpy(saved_game_root_directory, profile_directory, 0xff);
    _snprintf(saved_game_root_path, 0xff, "%s\\%s\\%s", saved_game_root_directory, "saved", "hdmu.map");
    _snprintf(savegames_directory, 0xff, "%s\\%s", saved_game_root_directory, "savegames");
    _snprintf(saved_directory, 0xff, "%s\\%s", saved_game_root_directory, "saved");
    directory_create_recursive(saved_directory);
    _snprintf(player_profiles_directory, 0xff, "%s\\%s", saved_game_root_directory, "saved\\player_profiles");
    directory_create_recursive(player_profiles_directory);
    _snprintf(default_player_profiles_directory, 0xff, "%s\\%s", saved_game_root_directory,
        "saved\\player_profiles\\default_profile");
    directory_create_recursive(default_player_profiles_directory);
    _snprintf(playlists_directory, 0xff, "%s\\%s", saved_game_root_directory, "saved\\playlists");
    directory_create_recursive(playlists_directory);
    _snprintf(default_playlists_directory, 0xff, "%s\\%s", saved_game_root_directory,
        "saved\\playlists\\default_playlist");
    directory_create_recursive(default_playlists_directory);
    _snprintf(last_profile_path, 0xff, "%s\\%s", saved_game_root_directory, "lastprof.txt");
    _snprintf(last_game_variant_path, 0xff, "%s\\%s", saved_game_root_directory, "lastmpvr.txt");
    _snprintf(last_multiplayer_map_path, 0xff, "%s\\%s", saved_game_root_directory, "lastmpmp.txt");

    savegame_index_dirty = 1;
    saved_game_files_mutex = 0;
    savegame_index_mutex = 0;
    mutex1_ok = mutex_create(&saved_game_files_mutex);
    if (mutex1_ok != 0) {
        mutex2_ok = mutex_create(&savegame_index_mutex);
        saved_game_files_initialized = 1;
        if (mutex2_ok != 0) {
            goto default_profile;
        }
    }
    saved_game_files_initialized = 0;

default_profile:
    zero_cursor = (uint8_t *)&default_profile_data;
    for (i = 0x1001; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    player_profile_initialize(&default_profile_data, 0, 0);
    default_player_profile_initialized = 1;
    player_profile_write_default_files();

    zero_cursor = (uint8_t *)&variant_write_request_state;
    for (i = 0x29; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    unknown_0072132a = 1;
}

/**
 * Under both module mutexes, opens the save-game index file and scans every entry for one whose
 * type matches and whose path matches name (case-insensitive, up to strlen(name) characters),
 * returning its packed handle. Returns -1 if not found or if either mutex wait, the entry count,
 * or the index open fails.
 *
 * @address 0x0053d4a0
 */
int32_t find_by_name(char *name, int16_t type)
{
    uint32_t name_length;
    uint32_t wait_result;
    int32_t entry_count;
    uint8_t opened;
    saved_game_index_entry entry;
    uint8_t read_ok;
    int32_t i;
    int32_t handle;
    int32_t result;

    result = -1;
    handle = -1;
    name_length = strlen(name);

    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80) {
        wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);
        if (wait_result == 0 || wait_result == 0x80) {
            entry_count = savegame_index_get_slot_count();
            opened = savegame_index_file_exists();
            if (opened != 0) {
                for (i = 0; i < entry_count; i++) {
                    read_ok = file_reference_read(&savegame_index_file, &entry, sizeof(entry));
                    if (read_ok == 0) {
                        break;
                    }
                    if (entry.type == type &&
                        _strnicmp(name, entry.path, name_length) == 0) {
                        handle = savegame_slot_handle_pack(i, entry.type, entry.builtin, entry.checksum_valid);
                        break;
                    }
                }
                file_reference_close(&savegame_index_file);
                result = handle;
            }
            ReleaseMutex(savegame_index_mutex->handle);
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    return result;
}

/**
 * Resolves handle to its index-file slot and, if found and its type is a player profile or game
 * variant, copies its path into out_directory and truncates it at the start of its "blam.sav" /
 * "blam.lst" file name, leaving just the containing directory. Clears out_directory and returns 0
 * on any failure (invalid handle, out-of-range slot, unresolved entry, unrecognised type, or the
 * file name not found in the resolved path).
 *
 * @address 0x0053d080
 */
uint8_t get_directory_by_handle(int32_t handle, char *out_directory)
{
    uint32_t slot_index;
    saved_game_index_entry entry;
    uint8_t found;
    const char *needle;
    char *name_start;

    out_directory[0] = '\0';
    if (handle == -1) {
        return 0;
    }
    slot_index = ((uint32_t)handle >> 16) & 0xfff;
    if (slot_index >= 999) {
        return 0;
    }
    found = savegame_index_read_slot((int32_t)slot_index, &entry);
    if (found == 0) {
        return 0;
    }
    if (entry.type != _saved_game_type_player_profile && entry.type != _saved_game_type_game_variant) {
        return 0;
    }
    strncpy(out_directory, entry.path, 0xff);
    out_directory[0xff] = '\0';
    needle = (entry.type == _saved_game_type_player_profile) ? "blam.sav" : "blam.lst";
    name_start = strstr(out_directory, needle);
    if (name_start != 0) {
        *name_start = '\0';
        return 1;
    }
    out_directory[0] = '\0';
    return 0;
}

/**
 * Resolves handle to its index-file slot (bits 16..27) and, if it is in range and the index
 * entry is found, copies its display name into the shared saved_game_display_name_buffer.
 * Always returns a pointer to that shared buffer (stale contents if the lookup failed).
 *
 * @address 0x0053c600
 */
uint16_t *get_display_name(int32_t handle)
{
    saved_game_index_entry entry;
    uint32_t slot_index;
    uint8_t found;

    slot_index = ((uint32_t)handle >> 16) & 0xfff;
    saved_game_display_name_buffer[0] = 0;
    if (slot_index < 999) {
        found = savegame_index_read_slot(slot_index, &entry);
        if (found != 0) {
            wcsncpy((wchar_t *)saved_game_display_name_buffer, (const wchar_t *)entry.display_name, 0x7f);
            saved_game_display_name_buffer[0x7f] = 0;
        }
    }
    return saved_game_display_name_buffer;
}

/**
 * Fills *out with a full game_variant for handle: if handle is non-negative (checksum bit clear,
 * not a confirmed index entry), or if the disk read/crc check below fails, *out gets the
 * classic-slayer defaults with its name field overwritten by saved_game_get_display_name(handle);
 * otherwise (handle negative, i.e. checksum-confirmed) it opens the handle's blam.lst, reads and
 * crc-validates its body, and returns that real data on a match. Always first waits for and
 * clears any in-flight asynchronous game-variant write. Returns 1 on success, 0 if the mutex
 * wait/handle-open/read failed on the disk-read path.
 *
 * @address 0x0053bee0
 */
uint8_t get_variant(int32_t handle, game_variant *out)
{
    uint32_t exit_code;
    uint32_t wait_result;
    file_reference_record ref;
    game_variant_file file;
    uint32_t checksum;
    game_variant defaults;
    game_variant *defaults_ptr;
    uint16_t *display_name;
    uint16_t dead_word;
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

/**
 * Builds savegame_index_file as a fresh, absolute file_reference_record naming saved_game_root_path,
 * creates and opens it for writing. On success, resets savegame_index_write_count to 0, marks
 * the index open, and returns 1. On failure, sets savegame_index_write_count to -1 and returns
 * whatever saved_game_index_file_open already was.
 *
 * @address 0x0053daa0
 */
uint8_t index_open_for_write(void)
{
    uint8_t created;
    uint8_t opened;

    memset(&savegame_index_file, 0, sizeof(savegame_index_file));
    savegame_index_file.signature = k_file_reference_signature;
    savegame_index_file.location = _file_location_absolute;
    if ((savegame_index_file.flags & _file_reference_is_file_bit) != 0) {
        path_remove_last_component(savegame_index_file.path);
    }
    path_append_component(savegame_index_file.path, saved_game_root_path);
    savegame_index_file.flags |= _file_reference_is_file_bit;

    created = file_reference_create(&savegame_index_file);
    if (created != 0) {
        opened = file_reference_open(&savegame_index_file, 2);
        if (opened != 0) {
            savegame_index_write_count = 0;
            saved_game_index_file_open = 1;
            return 1;
        }
    }
    savegame_index_write_count = -1;
    return saved_game_index_file_open;
}

/**
 * For each built-in playlist index 0..default_game_variant_count-1: builds
 * default_playlists_directory\NN\blam.lst's path, and if that file exists, registers a
 * saved_game_index_entry for it (path, a localized display name from the ui\\default_
 * multiplayer_game_setting_names ustr tag with the same fallback/strip-last-char handling as
 * playlist_profile_create_default_profiles_on_disk.c, type = game_variant, builtin = 1,
 * checksum_valid set if the file's own body crc matches its stored checksum), consuming an
 * index-file slot. Returns the loop index reached (early, on an index-count overflow or a write
 * failure) or the count of indices iterated (on normal completion).
 *
 * @address 0x0053db40
 */
int16_t index_register_default_playlists(void)
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
    tag_id = tag_lookup(0x75737472, (char *)"ui\\default_multiplayer_game_setting_names");
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

            _snprintf(path, 0xff, "%s\\%02d\\blam.lst", default_playlists_directory, (int32_t)i);

            memset(&ref, 0, sizeof(ref));
            ref.signature = k_file_reference_signature;
            ref.location = _file_location_absolute;
            if ((ref.flags & _file_reference_is_file_bit) != 0) {
                path_remove_last_component(ref.path);
            }
            if (path[0] != '\0') {
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
                entry.type = _saved_game_type_game_variant;
                entry.builtin = 1;

                opened = file_reference_open(&ref, 1);
                if (opened != 0) {
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

/**
 * For each built-in default profile index 0..k_default_player_profile_count-1: builds
 * default_player_profiles_directory\NN.sav's path, and if that file exists, registers a
 * saved_game_index_entry for it (path, a localized display name from the ui\\shell\\strings\\
 * default_player_profile_names ustr tag with the same fallback/strip-last-char handling as
 * saved_game_index_register_default_playlists.c, type = player_profile, builtin = 1,
 * checksum_valid set if the file's own body crc matches its stored checksum), consuming an
 * index-file slot. Returns the loop index reached (early, on an index-count overflow or a write
 * failure) or the count of indices iterated (on normal completion).
 *
 * @address 0x0053dde0
 */
int16_t index_register_default_profiles(void)
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

    tag_id = tag_lookup(0x75737472, (char *)"ui\\shell\\strings\\default_player_profile_names");
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
                path_remove_last_component(ref.path);
            }
            if (path[0] != '\0') {
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

/**
 * Creates (or truncates) lastmpmp.txt and writes 0x100 bytes from data into it.
 *
 * @address 0x0053d5e0
 */
void last_mp_map_clear(const void *data)
{
    file_reference_record ref;
    uint8_t ok;

    file_reference_init(&ref, last_multiplayer_map_path, 0);
    ok = file_reference_create(&ref);
    if (ok != 0) {
        ok = file_reference_open(&ref, 2);
        if (ok != 0) {
            file_reference_write(&ref, data, 0x100);
            file_reference_close(&ref);
        }
    }
    return;
}

/**
 * Opens lastmpmp.txt for read and reads 0x100 bytes into out_data. Always clears
 * out_data[0xff] before returning. Returns the read result (1 on success), or 0 if the file
 * couldn't be opened.
 *
 * @address 0x0053d670
 */
uint8_t last_mp_map_read(uint8_t *out_data)
{
    file_reference_record ref;
    uint8_t ok;
    uint8_t read_ok;

    file_reference_init(&ref, last_multiplayer_map_path, 0);
    ok = file_reference_open(&ref, 1);
    if (ok != 0) {
        read_ok = file_reference_read(&ref, out_data, 0x100);
        file_reference_close(&ref);
        out_data[0xff] = 0;
        return read_ok;
    }
    out_data[0xff] = 0;
    return 0;
}

/**
 * Creates (or truncates) lastmpvr.txt and writes 0x100 bytes from data into it.
 *
 * @address 0x0053d360
 */
void last_mp_variant_clear(const void *data)
{
    file_reference_record ref;
    uint8_t ok;

    file_reference_init(&ref, last_game_variant_path, 0);
    ok = file_reference_create(&ref);
    if (ok != 0) {
        ok = file_reference_open(&ref, 2);
        if (ok != 0) {
            file_reference_write(&ref, data, 0x100);
            file_reference_close(&ref);
        }
    }
    return;
}

/**
 * Opens lastmpvr.txt for read and reads 0x100 bytes into out_data. Always clears
 * out_data[0xff] before returning. Returns the read result (1 on success), or 0 if the file
 * couldn't be opened.
 *
 * @address 0x0053d3f0
 */
uint8_t last_mp_variant_read(uint8_t *out_data)
{
    file_reference_record ref;
    uint8_t ok;
    uint8_t read_ok;

    file_reference_init(&ref, last_game_variant_path, 0);
    ok = file_reference_open(&ref, 1);
    if (ok != 0) {
        read_ok = file_reference_read(&ref, out_data, 0x100);
        file_reference_close(&ref);
        out_data[0xff] = 0;
        return read_ok;
    }
    out_data[0xff] = 0;
    return 0;
}

/**
 * Creates (or truncates) lastprof.txt and writes 0x100 bytes from data into it.
 *
 * @address 0x0053d220
 */
void last_profile_clear(const void *data)
{
    file_reference_record ref;
    uint8_t ok;

    file_reference_init(&ref, last_profile_path, 0);
    ok = file_reference_create(&ref);
    if (ok != 0) {
        ok = file_reference_open(&ref, 2);
        if (ok != 0) {
            file_reference_write(&ref, data, 0x100);
            file_reference_close(&ref);
        }
    }
    return;
}

/**
 * Opens lastprof.txt for read and reads 0x100 bytes into out_data. Always clears
 * out_data[0xff] before returning (whether or not the read succeeded). Returns the read result
 * (1 on success), or 0 if the file couldn't be opened.
 *
 * @address 0x0053d2b0
 */
uint8_t last_profile_read(uint8_t *out_data)
{
    file_reference_record ref;
    uint8_t ok;
    uint8_t read_ok;

    file_reference_init(&ref, last_profile_path, 0);
    ok = file_reference_open(&ref, 1);
    if (ok != 0) {
        read_ok = file_reference_read(&ref, out_data, 0x100);
        file_reference_close(&ref);
        out_data[0xff] = 0;
        return read_ok;
    }
    out_data[0xff] = 0;
    return 0;
}

/**
 * Under saved_game_files_mutex, (re)opens the index file for writing and enumerates every entry
 * under savegames_directory. For each found directory, tries "<dir>blam.sav" then, if that
 * doesn't exist, "<dir>blam.lst"; whichever exists becomes a saved_game_index_entry (path, the
 * found display name, its type, checksum_valid set if the body's crc matches), written to the
 * index (subject to both the per-scan 999-entry cap and the persistent index-write-count cap).
 * A directory matching neither is logged and skipped. After the scan (successful or not), always
 * re-registers the built-in default playlists and profiles, closes the index file, resets
 * savegame_index_write_count to -1 and saved_game_index_file_open to 0, and finally clears
 * savegame_index_dirty.
 *
 * @address 0x0053d720
 */
void list_rebuild_index(void)
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

                    path_len = _snprintf(entry.path, 0xff, "%s%s", find_data.save_game_directory, "blam.sav");
                    entry_type = -1;
                    if (path_len < 1) {
                        goto try_variant;
                    }

                    memset(&ref, 0, sizeof(ref));
                    ref.signature = k_file_reference_signature;
                    ref.location = _file_location_absolute;
                    if ((ref.flags & _file_reference_is_file_bit) != 0) {
                        path_remove_last_component(ref.path);
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
                    path_len = _snprintf(entry.path, 0xff, "%s%s", find_data.save_game_directory, "blam.lst");
                    if (0 < path_len) {
                        memset(&ref, 0, sizeof(ref));
                        ref.signature = k_file_reference_signature;
                        ref.location = _file_location_absolute;
                        if ((ref.flags & _file_reference_is_file_bit) != 0) {
                            path_remove_last_component(ref.path);
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
                    wcsncpy((wchar_t *)entry.display_name, (const wchar_t *)find_data.save_game_name, 0x7f);
                    entry.type = entry_type;

                    opened = file_reference_open(&ref, 1);
                    if (opened != 0) {
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

/**
 * Returns 1 if name is non-null, non-empty and XCreateSaveGame's open-existing query (mode 3)
 * against savegames_directory fails, i.e. no saved game of that name exists yet; 0 otherwise.
 *
 * @address 0x0053d1e0
 */
uint8_t name_is_available(const uint16_t *name)
{
    char scratch[0x100];
    uint32_t result;

    if (name != 0 && *name != 0) {
        result = XCreateSaveGame(name, savegames_directory, 3, scratch, 0x100);
        if (result != 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Resolves handle to its index-file slot and, if found, builds a file-reference in out_ref
 * naming its path (absolute, is-file) and opens it for reading and writing. Returns 1 on
 * success, 0 if the slot couldn't be resolved or the open failed.
 *
 * @address 0x0053c9f0
 */
uint8_t open_file_by_handle(int32_t handle, file_reference_record *out_ref)
{
    saved_game_index_entry entry;
    uint8_t found;
    uint8_t opened;

    found = savegame_index_read_slot(((uint32_t)handle >> 16) & 0xfff, &entry);
    if (found != 0) {
        memset(out_ref, 0, sizeof(*out_ref));
        out_ref->signature = k_file_reference_signature;
        out_ref->location = _file_location_absolute;
        if ((out_ref->flags & _file_reference_is_file_bit) != 0) {
            path_remove_last_component(out_ref->path);
        }
        path_append_component(out_ref->path, entry.path);
        out_ref->flags = out_ref->flags | _file_reference_is_file_bit;
        opened = file_reference_open(out_ref, 3);
        if (opened != 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * stack parameters (expected_crc, corrupt_flag)
 * Opens savegame.bin, reads header_size bytes into header_buffer, then crcs the whole
 * total_size-byte image (header included) in up to 0x20000-byte chunks and compares the result
 * against *expected_crc (clearing *expected_crc as it goes). If the header read itself fails,
 * deletes the current profile's savegame.bin. Returns 1 only on a matching crc.
 *
 * @address 0x00539570
 */
uint8_t validate_crc(int32_t total_size, int32_t header_size, uint8_t *header_buffer, uint32_t *expected_crc,
    uint8_t *corrupt_flag)
{
    void *file;
    uint32_t bytes_read;
    int32_t remaining;
    int32_t chunk;
    uint32_t running_crc;
    uint32_t previous_crc;
    char profile_directory[512];

    file = game_state_open_persistent_storage(0);
    if (corrupt_flag != 0) {
        *corrupt_flag = 0;
    }
    if (file == (void *)0xffffffff) {
        return 0;
    }

    if (SetFilePointer(file, 0, 0, 0) == 0xffffffff ||
        ReadFile(file, header_buffer, header_size, (LPDWORD)&bytes_read, 0) == 0 ||
        bytes_read != (uint32_t)header_size) {
        if (saved_game_get_directory_by_handle(saved_player_profile_slots_handle, profile_directory) != 0) {
            DeleteFileA(profile_directory);
        }
    } else {
        previous_crc = *expected_crc;
        running_crc = 0xffffffff;
        *expected_crc = 0;
        ((void (*)(uint32_t *crc, uint8_t *data, int32_t length))crc32_update)(&running_crc, header_buffer, header_size);

        remaining = total_size - header_size;
        while (0 < remaining) {
            uint8_t chunk_buffer[0x20000];

            chunk = remaining;
            if (0x1ffff < remaining) {
                chunk = 0x20000;
            }
            if (ReadFile(file, chunk_buffer, chunk, (LPDWORD)&bytes_read, 0) != 0 && bytes_read == (uint32_t)chunk) {
                ((void (*)(uint32_t *crc, uint8_t *data, int32_t length))crc32_update)(&running_crc, chunk_buffer, chunk);
            }
            halo::sound::sound_idle_update();
            remaining = remaining - chunk;
        }

        if (running_crc == previous_crc) {
            CloseHandle(file);
            return 1;
        }
        if (corrupt_flag != 0 && previous_crc != 0) {
            *corrupt_flag = 1;
            CloseHandle(file);
            return 0;
        }
    }
    CloseHandle(file);
    return 0;
}

/**
 * Verifies a loaded game_state_header against the running build: its build_version string must
 * match the current build or one of eight older accepted builds, and its scenario_name,
 * allocation_checksum, local_player_count and map_checksum must all match the running state.
 * On any mismatch, if report_error is set, points rasterizer_shader_file_name at the header's
 * scenario_name and raises a fatal error dialog (string 0x89, title 0x7e).
 *
 * @address 0x00538430
 */
uint8_t verify_version_and_checksum(game_state_header *header, uint8_t report_error)
{
    static const char *k_accepted_build_versions[] = {
        "01.00.03.0606", "01.00.04.0607", "01.00.05.0610", "01.00.06.0612",
        "01.00.07.0613", "01.00.08.0616", "01.00.09.0619", "01.00.10.0620"
    };
    uint8_t version_ok;
    int32_t i;
    char *tag_path;

    version_ok = (strcmp(header->build_version, "01.00.10.0621") == 0) ? 1 : 0;
    if (version_ok == 0) {
        for (i = 0; i < 8; i = i + 1) {
            if (strcmp(header->build_version, k_accepted_build_versions[i]) == 0) {
                version_ok = 1;
                break;
            }
        }
    }

    if (version_ok == 0) {
        if (report_error == 0) {
            return 0;
        }
        rasterizer_shader_file_name = header->scenario_name;
        shell_display_fatal_error_dialog(0x89, 0x7e, 1);
        return 0;
    }

    tag_path = tag_instances[(int16_t)global_scenario_index].path;
    if (strcmp(header->scenario_name, tag_path) == 0 &&
        header->allocation_checksum == game_state_crc &&
        header->local_player_count == local_player_count &&
        header->map_checksum == cache_file_current_header_crc32) {
        return 1;
    }

    if (report_error != 0) {
        rasterizer_shader_file_name = header->scenario_name;
        shell_display_fatal_error_dialog(0x89, 0x7e, 1);
    }
    return 0;
}

}  // namespace halo::saved_games::saved_game
