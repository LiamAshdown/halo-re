#include "halo/game/gamerest_savegame.hpp"
#include "halo/text/api.hpp"
#include <wchar.h>
#include <stdint.h>
#include "halo/cache/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &user_save_path_default = halo::link::ref<char *>(halo::game::vars().user_save_path_default);
static auto &saved_game_root_path = halo::link::ref<char []>(halo::game::vars().saved_game_root_path);
static auto &savegame_index_file = halo::link::ref<file_reference>(halo::game::vars().savegame_index_file);
static auto &savegame_index_mutex = halo::link::ref<network_mutex_record *>(halo::game::vars().savegame_index_mutex);
static auto &user_save_path_keys = halo::link::ref<uint32_t [k_maximum_user_save_paths]>(halo::game::vars().user_save_path_keys);
static auto &user_save_paths = halo::link::ref<char [k_maximum_user_save_paths][k_user_save_path_slot_stride]>(halo::game::vars().user_save_paths);
static auto &missing_string_text = halo::link::ref<wchar_t []>(halo::ui::vars().missing_string_text);
static auto &unicode_string_list_scratch_buffer = halo::link::ref<wchar_t>(halo::game::vars().unicode_string_list_scratch_buffer);
static auto &global_sound_effect_object = halo::link::ref<void *>(halo::game::vars().global_sound_effect_object);
static auto &player_profile_cache = halo::link::ref<player_profile [16]>(halo::game::vars().player_profile_cache);
static auto &player_profile_cache_count = halo::link::ref<int32_t>(halo::game::vars().player_profile_cache_count);
static auto &player_profile_cache_initialized = halo::link::ref<uint8_t>(halo::game::vars().player_profile_cache_initialized);

namespace halo::game {

/**
 * blam-cc: EAX -> save_game_name, stack -> root_path, mode, out_path, out_path_size
 * FIXED (step 1, objdump -d 0x551710..0x55199d): EAX is the save game's Unicode name -- the source of the ASCII name
 * every path is built from (0x551762: ESI = ascii name, EDI = name, push 0x80); the draft called it an unused token
 * and converted nothing. The "Name=" line is converted into a scratch buffer (EAX dst, EBX src, EDI 0x80).
 * Builds "<root_path>\<name>\" (slot_path) and "<root_path>\<name>" (its trailing-slash-free
 * twin, reused as the file to create), and "<root_path>\<name>" 's info line "Name=<name>\n";
 * ensures root_path exists (creating it if needed); for mode 3 (open existing), just touches
 * the file; for mode 1/4, creates the slot directory (plus a "checkpoints\" subdirectory the
 * first time the slot itself is created) and writes the slot file's own path string into it (0x551920: the
 * "Name=" line is built and converted but never written). On
 * success, copies slot_path into *out_path. Returns 0 on success, or a Win32/HRESULT-style
 * error code.
 *
 * @address 0x551710
 */
uint32_t SaveGameFiles::create(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path, uint32_t out_path_size)
{
    uint32_t bytes_written = 0;
    char name[128];
    char slot_path[264];
    char slot_dir[264];
    char checkpoint_dir[264];
    char slot_file_no_slash[264];
    char info_line[1024];
    uint32_t root_attrs;
    uint32_t slot_attrs;
    uint32_t have_slot;
    void *file;
    int32_t i;

    if (root_path == 0 || save_game_name == 0 || out_path == 0 || out_path_size == 0) {
        return 0x57;
    }

    halo::text::string_convert_unicode_to_ascii((uint8_t *)name, (uint16_t *)save_game_name, 0x80);
    sprintf(slot_dir, "%s\\%s\\", root_path, name);
    sprintf(slot_path, "%s%s", slot_dir, name);
    sprintf(info_line, "Name=%s\n", name);
    halo::text::string_convert_ascii_to_unicode((uint16_t *)checkpoint_dir, 0x80, info_line);

    sprintf(slot_file_no_slash, "%s\\%s", root_path, name);

    root_attrs = GetFileAttributesA(root_path);
    if (root_attrs == 0xffffffff && CreateDirectoryA(root_path, 0) == 0) {
        return 0x80004005;
    }

    slot_attrs = GetFileAttributesA(slot_file_no_slash);
    have_slot = 1;
    if (slot_attrs == 0xffffffff) {
        have_slot = bytes_written;
    }

    if (mode != 1) {
        if (mode == 3) {
            if (have_slot == 0) {
                return 0x80004005;
            }
            file = CreateFileA(slot_path, 0, 0, 0, 2, 0x80, 0);
            if (file == (void *)0xffffffff) {
                return 0x80004005;
            }
            CloseHandle(file);
            return 0;
        }
        if (mode != 4) {
            return 0x57;
        }
    }

    if (have_slot == 0) {
        if (CreateDirectoryA(slot_dir, 0) == 0) {
            return 0x80004005;
        }

        i = 0;
        do {
            checkpoint_dir[i] = slot_dir[i];
            i = i + 1;
        } while (slot_dir[i - 1] != '\0');
        {
            int32_t end = 0;
            while (checkpoint_dir[end] != '\0') {
                end = end + 1;
            }
            strncpy(checkpoint_dir + end, "checkpoints\\", 0xd);
        }

        if (CreateDirectoryA(checkpoint_dir, 0) == 0) {
            return 0x80004005;
        }
    }

    file = CreateFileA(slot_path, 0xc0000000, 0, 0, 2, 0x80, 0);
    if (file != (void *)0xffffffff) {
        int32_t length = 0;
        while (slot_path[length] != '\0') {
            length = length + 1;
        }
        WriteFile(file, slot_path, length, (LPDWORD)(&bytes_written), 0);
        CloseHandle(file);
        if ((uint32_t)length == bytes_written) {
            strncpy(out_path, slot_dir, out_path_size);
            return 0;
        }
    }
    return 0x80004005;
}

/**
 * blam-cc: EAX -> save_game_name, ECX -> root_path
 * Deletes every file directly under "<root_path>\<name>\" (skipping dotfiles and an entry
 * literally named "checkpoints"), then every file under its "checkpoints\" subdirectory,
 * removes that subdirectory, and finally removes "<root_path>\<name>" itself (trailing slash
 * trimmed). Returns 0 on success (the final RemoveDirectoryA returned 1), 1 otherwise.
 * FIXED (objdump 0x5519a7..0x5519cd): the first argument (EAX, kept in EDI) is the save's wide-character name -- it is
 * the source of the name conversion below -- not a validity token.
 *
 * @address 0x5519a0
 */
uint32_t SaveGameFiles::remove_files(const uint16_t *save_game_name, const char *root_path)
{
    char name[128];
    char root_with_slash[266];
    char pattern[264];
    char delete_path[127];
    win32_find_dataa find_data;
    void *find_handle;
    uint32_t last_delete_ok;
    uint32_t removed_checkpoints_dir;
    int32_t result;

    if (root_path == 0 || save_game_name == 0) {
        return 0x57;
    }

    halo::text::string_convert_unicode_to_ascii((uint8_t *)name, (uint16_t *)save_game_name, 0x80);
    sprintf(root_with_slash, "%s\\%s\\", root_path, name);

    sprintf(pattern, "%s*.*", root_with_slash);
    last_delete_ok = 0;
    find_handle = FindFirstFileA(pattern, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)0xffffffff) {
        do {
            if (find_data.cFileName[0] != '.') {
                {
                    int32_t i = 0xc;
                    uint8_t matches_checkpoints = 1;
                    const char *a = find_data.cFileName;
                    const char *b = "checkpoints";
                    while (i != 0 && matches_checkpoints) {
                        i = i - 1;
                        matches_checkpoints = (*a == *b);
                        a = a + 1;
                        b = b + 1;
                    }
                    if (!matches_checkpoints) {
                        sprintf(delete_path, "%s%s", root_with_slash, find_data.cFileName);
                        last_delete_ok = DeleteFileA(delete_path);
                        if (last_delete_ok == 0) {
                            break;
                        }
                    }
                }
            }
        } while (FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data) != 0);
        FindClose(find_handle);
    }

    sprintf(pattern, "%scheckpoints\\*.*", root_with_slash);
    result = 0;
    if (last_delete_ok != 0) {
        find_handle = FindFirstFileA(pattern, (LPWIN32_FIND_DATAA)&find_data);
        if (find_handle != (void *)0xffffffff) {
            uint32_t has_more;
            do {
                if (find_data.cFileName[0] != '.') {
                    int32_t i = 0xc;
                    uint8_t matches_checkpoints = 1;
                    const char *a = find_data.cFileName;
                    const char *b = "checkpoints";
                    while (i != 0 && matches_checkpoints) {
                        i = i - 1;
                        matches_checkpoints = (*a == *b);
                        a = a + 1;
                        b = b + 1;
                    }
                    if (!matches_checkpoints) {
                        sprintf(delete_path, "%scheckpoints\\%s", root_with_slash, find_data.cFileName);
                        DeleteFileA(delete_path);
                    }
                }
                has_more = FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data);
            } while (has_more != 0);
            FindClose(find_handle);
            sprintf(pattern, "%scheckpoints", root_with_slash);
            removed_checkpoints_dir = RemoveDirectoryA(pattern);
        } else {

            removed_checkpoints_dir = last_delete_ok;
        }

        result = 0;
        if (removed_checkpoints_dir != 0) {
            int32_t end = 0;
            while (root_with_slash[end] != '\0') {
                end = end + 1;
            }
            root_with_slash[end - 1] = '\0';
            result = (int32_t)RemoveDirectoryA(root_with_slash);
        }
    }
    return result != 1;
}

/**
 * blam-cc: EAX -> find_data, stack -> root_path
 * Starts a directory enumeration under `root_path`, registers the resulting handle against
 * `root_path` in the user_save_path table, and skips forward to the first subdirectory entry
 * (attribute bit 0x10) that isn't a dotfile, building "root_path\<subdir>\" into the scratch
 * area 0x140 bytes past `find_data`. Returns the find handle on success, NULL if the arguments
 * are invalid, or (HANDLE)-1 if the search comes up empty or registration fails.
 * FIXED: returns the find handle as an integer (callers compare it with -1), and the parameters are ordered as both
 * callers pass them (root, find data) -- registers are bound by name: EAX find_data, stack root_path (0x53d769..0x53d775)
 *
 * @address 0x551bc0
 */
int32_t SaveGameFiles::find_first(char *root_path, win32_find_dataa *find_data)
{
    char pattern[264];
    void *handle;

    if (root_path == 0 || find_data == 0) {
        return 0;
    }

    sprintf(pattern, "%s\\*.*", root_path);
    handle = FindFirstFileA(pattern, (LPWIN32_FIND_DATAA)find_data);
    if (handle == (void *)0xffffffff) {
        return (int32_t)handle;
    }

    {
        uint8_t exhausted = 0;
        int32_t slot = UserSavePaths::user_save_path_register((uint32_t)handle, root_path);
        if (slot == -1) {
            FindClose(handle);
            return -1;
        }

        while ((find_data->dwFileAttributes & 0x10) != 0) {
            if (find_data->cFileName[0] != '.') {
                char *scratch = (char *)find_data + 0x140;
                int32_t end;

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                }

                halo::text::string_convert_ascii_to_unicode((uint16_t *)((uint8_t *)find_data + 0x244), 0x80, find_data->cFileName);

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = root_path[i];
                        i = i + 1;
                    } while (root_path[i - 1] != '\0');
                }
                end = 0;
                while (scratch[end] != '\0') {
                    end = end + 1;
                }
                scratch[end] = '\\';
                end = end + 1;
                {
                    int32_t i = 0;
                    do {
                        scratch[end + i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                    end = end + i - 1;
                }
                scratch[end] = '\\';
                scratch[end + 1] = '\0';

                return (int32_t)handle;
            }
            if (exhausted) {
                break;
            }
            if (FindNextFileA(handle, (LPWIN32_FIND_DATAA)find_data) == 0) {
                exhausted = 1;
            }
        }
    }

    FindClose(handle);
    return -1;
}

/**
 * blam-cc: EAX -> find_data, ECX -> handle
 * If `handle` is registered (its root_path is not the default sentinel) and FindNextFileA
 * yields another entry, returns 1; if that entry is a non-dotfile subdirectory, also rebuilds
 * "root_path\<subdir>\" in the scratch area past `find_data`, exactly like savegame_find_first.
 * Returns 0 if `handle`/`find_data` are invalid, the handle isn't registered, or the
 * enumeration is exhausted.
 *
 * @address 0x551d30
 */
uint32_t SaveGameFiles::find_next(win32_find_dataa *find_data, uint32_t handle)
{
    uint32_t result = 0;

    if (handle != 0 && find_data != 0) {
        char *root_path = UserSavePaths::lookup(handle);
        if (root_path != user_save_path_default &&
            FindNextFileA((void *)handle, (LPWIN32_FIND_DATAA)find_data) != 0) {
            result = 1;
            if (find_data->cFileName[0] != '.' && (find_data->dwFileAttributes & 0x10) != 0) {
                char *scratch = (char *)find_data + 0x140;
                int32_t end;

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                }

                halo::text::string_convert_ascii_to_unicode((uint16_t *)((uint8_t *)find_data + 0x244), 0x80, find_data->cFileName);

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = root_path[i];
                        i = i + 1;
                    } while (root_path[i - 1] != '\0');
                }
                end = 0;
                while (scratch[end] != '\0') {
                    end = end + 1;
                }
                scratch[end] = '\\';
                end = end + 1;
                {
                    int32_t i = 0;
                    do {
                        scratch[end + i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                    end = end + i - 1;
                }
                scratch[end] = '\\';
                scratch[end + 1] = '\0';
            }
        }
    }
    return result;
}

/**
 * Implements the original `savegame_slot_handle_pack`.
 *
 * @address 0x53e630
 */
uint32_t SaveGameFiles::slot_handle_pack(uint32_t slot_index, uint32_t type_nibble, uint8_t flag_bit_30, uint8_t flag_bit_31)
{
    uint32_t result = ((slot_index & 0xfff) << 0x10) | (type_nibble & 0xf);
    if (flag_bit_30 == 1) {
        result = result | 0x40000000;
    }
    if (flag_bit_31 == 1) {
        result = result | 0x80000000;
    }
    return result;
}

/**
 * UNSURE: `unused` is read nowhere in this function's body (see header note).
 * If the index file has fewer than 999 records, seeks to the end and writes a new record
 * (source elided, same as savegame_index_write_slot), reporting the new record's slot number
 * through `*out_slot_count`. Returns 1 on success, 0 otherwise.
 * FIXED 2026-09-27 (static loop) from objdump 0x53e300..0x53e40d: the first stack argument ([esp+0x14] at 0x53e3be)
 * is the 0x206-byte entry appended at record index size / 0x206 (must be < 999); the draft named it "unused" and
 * called seek / write / close with no arguments, so creating a profile never registered it in the index.
 *
 * @address 0x53e300
 */
uint8_t SaveGameIndex::append_slot(const void *entry, uint32_t *out_slot_count)
{
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);

    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }

    {
        uint32_t *raw = (uint32_t *)&savegame_index_file;
        int32_t i;
        uint8_t *flags_byte = (uint8_t *)&savegame_index_file + 4;
        uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_index_file + 6);

        for (i = 0; i < 0x43; i++) {
            raw[i] = 0;
        }
        raw[0] = 0x66696c6f;
        *word_at_6 = 2;
        if ((*flags_byte & 1) != 0) {
            halo::saved_games::path_remove_last_component((char *)((uint8_t *)&savegame_index_file + 8));
        }
        halo::saved_games::path_append_component((char *)&savegame_index_file + 8, saved_game_root_path);
        *flags_byte = *flags_byte | 1;
    }

    if (halo::saved_games::file_reference_open((file_reference_record *)&savegame_index_file, 2) != 0) {
        uint32_t size = halo::saved_games::file_reference_get_size((file_reference_record *)&savegame_index_file);
        if (size / 0x206 < 999) {
            if (halo::saved_games::file_reference_seek((int32_t)(size / 0x206) * 0x206, (file_reference_record *)&savegame_index_file) != 0 &&
                halo::saved_games::file_reference_write((file_reference_record *)&savegame_index_file, entry, 0x206) != 0) {
                result = 1;
                *out_slot_count = size / 0x206;
            }
        }
        if (halo::saved_games::file_reference_close((file_reference_record *)&savegame_index_file) == 0) {
            result = 0;
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

/**
 * Resets the shared save-game-directory file_reference to the "saved games index" path and
 * tries to open it for reading, purely to confirm the file exists (and to warm its cached
 * size); returns 1 on success, 0 otherwise.
 *
 * @address 0x53e060
 */
uint8_t SaveGameIndex::file_exists()
{
    uint32_t *raw = (uint32_t *)&savegame_index_file;
    int32_t i;
    uint8_t *flags_byte = (uint8_t *)&savegame_index_file + 4;
    uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_index_file + 6);

    for (i = 0; i < 0x43; i++) {
        raw[i] = 0;
    }
    raw[0] = 0x66696c6f;
    *word_at_6 = 2;
    if ((*flags_byte & 1) != 0) {
        halo::saved_games::path_remove_last_component((char *)((uint8_t *)&savegame_index_file + 8));
    }
    halo::saved_games::path_append_component((char *)&savegame_index_file + 8, saved_game_root_path);
    *flags_byte = *flags_byte | 1;

    if (halo::saved_games::file_reference_open((file_reference_record *)&savegame_index_file, 1) != 0) {
        halo::saved_games::file_reference_get_size((file_reference_record *)&savegame_index_file);
        return 1;
    }
    return 0;
}

/**
 * Resets the shared file_reference to the index path and queries its size via file_reference_get_size_by_path,
 * returning size / 0x206 (the number of save-slot records), or 0 if the query fails.
 *
 * @address 0x53e420
 */
uint32_t SaveGameIndex::get_slot_count()
{
    uint32_t *raw = (uint32_t *)&savegame_index_file;
    int32_t i;
    uint8_t *flags_byte = (uint8_t *)&savegame_index_file + 4;
    uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_index_file + 6);
    uint32_t size;

    for (i = 0; i < 0x43; i++) {
        raw[i] = 0;
    }
    raw[0] = 0x66696c6f;
    *word_at_6 = 2;
    if ((*flags_byte & 1) != 0) {
        halo::saved_games::path_remove_last_component((char *)((uint8_t *)&savegame_index_file + 8));
    }
    halo::saved_games::path_append_component((char *)&savegame_index_file + 8, saved_game_root_path);
    *flags_byte = *flags_byte | 1;

    if (halo::saved_games::file_reference_get_size_by_path((file_reference_record *)((void *)&savegame_index_file), &size) != 0) {
        return size / 0x206;
    }
    return 0;
}

/**
 * Locks the save-game directory mutex, resets the shared file_reference to the index path,
 * opens it for reading, and -- if it is large enough to contain `slot` -- seeks to that slot's
 * record and reads it. Returns 1 on full success, 0 otherwise; always releases the mutex before
 * returning (unless the initial wait itself failed/timed out).
 * FIXED (objdump 0x53e16e..0x53e1c2): two stack parameters, the slot and the 0x206-byte destination record
 * ([esp+0x14], passed to file_reference_read in ECX); both callers push (slot, &entry).
 *
 * @address 0x53e0e0
 */
uint8_t SaveGameIndex::read_slot(uint16_t slot, void *out_entry)
{
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);

    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }

    {
        uint32_t *raw = (uint32_t *)&savegame_index_file;
        int32_t i;
        uint8_t *flags_byte = (uint8_t *)&savegame_index_file + 4;
        uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_index_file + 6);

        for (i = 0; i < 0x43; i++) {
            raw[i] = 0;
        }
        raw[0] = 0x66696c6f;
        *word_at_6 = 2;
        if ((*flags_byte & 1) != 0) {
            halo::saved_games::path_remove_last_component((char *)((uint8_t *)&savegame_index_file + 8));
        }
        halo::saved_games::path_append_component((char *)&savegame_index_file + 8, saved_game_root_path);
        *flags_byte = *flags_byte | 1;
    }

    if (halo::saved_games::file_reference_open((file_reference_record *)&savegame_index_file, 1) != 0) {
        uint32_t size = halo::saved_games::file_reference_get_size((file_reference_record *)&savegame_index_file);
        if ((uint32_t)slot * 0x206 + 0x206 <= size) {
            if (halo::saved_games::file_reference_seek((int32_t)slot * 0x206, (file_reference_record *)((file_reference *)&savegame_index_file)) != 0) {
                result = 1;
                if (halo::saved_games::file_reference_read((file_reference_record *)((file_reference *)&savegame_index_file), out_entry, 0x206) == 0) {
                    result = 0;
                }
            }
        }
        if (halo::saved_games::file_reference_close((file_reference_record *)((file_reference *)&savegame_index_file)) == 0) {
            result = 0;
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

/**
 * As savegame_index_read_slot, but opens the index file for writing and writes `slot`'s record
 * instead of reading it.
 * FIXED 2026-09-27 (static loop) from objdump 0x53e1f0..0x53e2fa: the second stack argument ([esp+0x14] at
 * 0x53e2ae) is the 0x206-byte index entry written at slot * 0x206; the draft had no entry parameter and called
 * seek / write / close with no arguments, so a profile rename never reached the index file.
 *
 * @address 0x53e1f0
 */
uint8_t SaveGameIndex::write_slot(uint16_t slot, const void *entry)
{
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);

    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }

    {
        uint32_t *raw = (uint32_t *)&savegame_index_file;
        int32_t i;
        uint8_t *flags_byte = (uint8_t *)&savegame_index_file + 4;
        uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_index_file + 6);

        for (i = 0; i < 0x43; i++) {
            raw[i] = 0;
        }
        raw[0] = 0x66696c6f;
        *word_at_6 = 2;
        if ((*flags_byte & 1) != 0) {
            halo::saved_games::path_remove_last_component((char *)((uint8_t *)&savegame_index_file + 8));
        }
        halo::saved_games::path_append_component((char *)&savegame_index_file + 8, saved_game_root_path);
        *flags_byte = *flags_byte | 1;
    }

    if (halo::saved_games::file_reference_open((file_reference_record *)&savegame_index_file, 2) != 0) {
        uint32_t size = halo::saved_games::file_reference_get_size((file_reference_record *)&savegame_index_file);
        if ((uint32_t)slot * 0x206 + 0x206 <= size) {
            if (halo::saved_games::file_reference_seek((int32_t)slot * 0x206, (file_reference_record *)&savegame_index_file) != 0) {
                result = 1;
                if (halo::saved_games::file_reference_write((file_reference_record *)&savegame_index_file, entry, 0x206) == 0) {
                    result = 0;
                }
            }
        }
        if (halo::saved_games::file_reference_close((file_reference_record *)&savegame_index_file) == 0) {
            result = 0;
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

/**
 * blam-cc: ECX -> user_id
 * Returns the registered path for `user_id`, or the default path if no slot matches.
 *
 * @address 0x5516a0
 */
char * UserSavePaths::lookup(uint32_t user_id)
{
    int32_t i = 0;
    do {
        if (user_save_path_keys[i] == user_id) {
            if (i == -1) {
                return user_save_path_default;
            }
            return user_save_paths[i];
        }
        i = i + 1;
    } while (i < k_maximum_user_save_paths);
    return user_save_path_default;
}

/**
 * Finds the first free (user id 0) slot, copies `path` into it (up to 0x104 chars, per
 * strncpy's own count), stores `user_id`, and returns the slot index; returns -1 if all 8
 * slots are taken.
 *
 * @address 0x551650
 */
int32_t UserSavePaths::user_save_path_register(uint32_t user_id, char *path)
{
    int32_t i = 0;
    do {
        if (user_save_path_keys[i] == 0) {
            strncpy(user_save_paths[i], path, 0x104);
            user_save_path_keys[i] = user_id;
            return i;
        }
        i = i + 1;
    } while (i < k_maximum_user_save_paths);
    return -1;
}

/**
 * blam-cc: EAX -> user_id
 * Finds the slot registered for `user_id`, clears its user id and zeroes its path buffer, and
 * returns 1. Returns 0 if no slot matches.
 *
 * @address 0x5516d0
 */
uint8_t UserSavePaths::user_save_path_remove(uint32_t user_id)
{
    int32_t i = 0;
    do {
        if (user_save_path_keys[i] == user_id) {
            if (i != -1) {
                uint32_t *raw;
                int32_t j;

                user_save_path_keys[i] = 0;
                raw = (uint32_t *)user_save_paths[i];
                for (j = 0; j < 0x41; j++) {
                    raw[j] = 0;
                }
                ((uint8_t *)raw)[0x104] = 0;
                return 1;
            }
            break;
        }
        i = i + 1;
    } while (i < k_maximum_user_save_paths);
    return 0;
}

/**
 * blam-cc: path in EAX, index in ECX (low 16 bits only)
 * Looks up the unicode_string_list tag named `path` and returns its `index`'th string, copied
 * into the shared scratch buffer. Falls back to the shared L"<missing string>" placeholder when the
 * tag cannot be found or `index` is out of range. When a string is found, its last wide character (the tag
 * data's own list-entry terminator/padding character) is overwritten with a NUL before the
 * copy -- this trim is unconditional whenever the raw entry size is positive (0x4b8d75..0x4b8d7e).
 *
 * @address 0x4b8d30
 */
wchar_t * UnicodeStringLists::get_string(char *path, int16_t index)
{
    datum_index tag_id;
    wchar_t *source;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    int32_t char_count;

    tag_id = halo::cache::tag_lookup(0x75737472, path);
    source = missing_string_text;

    if (tag_id != k_datum_index_none && index >= 0) {
        list = (UnicodeStringList *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
        if (index < (int32_t)list->strings.count) {
            entry = (UnicodeStringListString *)list->strings.pointer + index;
            char_count = (int32_t)entry->string.size;
            if (char_count > 0) {
                source = (wchar_t *)entry->string.pointer;
                source[(char_count / 2) - 1] = 0;
            }
        }
    }

    wcscpy(&unicode_string_list_scratch_buffer, source);
    return &unicode_string_list_scratch_buffer;
}

/**
 * Returns 1 if global_sound_effect_object is non-NULL and its dword at +4 is 0, 1, or 2.
 *
 * @address 0x551620
 */
uint32_t UserProfile::signin_state_is_valid()
{
    if (global_sound_effect_object != 0) {
        int32_t state = *(int32_t *)((uint8_t *)global_sound_effect_object + 4);
        if (state == 0 || state == 1 || state == 2) {
            return 1;
        }
    }
    return 0;
}

/**
 * One-time initializer: zeroes the whole 16-slot player-profile cache table (and, redundantly,
 * each slot's `in_use` flag a second time) and resets the live count. A no-op after the first
 * call.
 *
 * @address 0x466c20
 */
void UserProfile::profile_cache_initialize()
{
    uint8_t *bytes;
    uint32_t i;
    int32_t slot;

    if (player_profile_cache_initialized != 0) {
        return;
    }

    bytes = (uint8_t *)player_profile_cache;
    for (i = 0; i < sizeof(player_profile_cache); i = i + 1) {
        bytes[i] = 0;
    }

    for (slot = 0; slot < 16; slot = slot + 1) {
        player_profile_cache[slot].in_use = 0;
    }

    player_profile_cache_count = 0;
    player_profile_cache_initialized = 1;
}

/**
 * Implements the original `player_customization_slot_set`.
 *
 * @address 0x4705f0
 */
uint8_t UserProfile::customization_slot_set(uint8_t *base, uint8_t new_value, uint32_t key)
{
    uint8_t *entry = base + 0x1a2;
    int32_t i;

    for (i = 0; i < 16; i++) {

        if ((int32_t)(int8_t)entry[0x1f] == (int32_t)key) {

            uint8_t changed = (int32_t)(int8_t)entry[0x1e] != (int32_t)new_value;
            entry[0x1e] = new_value;
            return changed;
        }
        entry = entry + 0x20;
    }
    return 0;
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::SaveGameFiles::create; forwards to the C++ implementation.
 * register convention: a value gating the whole call (Ghidra's `in_EAX`, checked only for
 * non-zero, never otherwise read) in EAX; `root_path`, `mode`, `out_path` and `out_path_size`
 * are this function's own four recognized stack parameters.
 * // blam-cc: EAX -> save_game_name, stack -> root_path, mode, out_path, out_path_size
 * blam-cc: ESI dest, EDI source, stack capacity
 * blam-cc: EAX dst, EDI capacity_bytes, EBX source
 * blam-cc: EAX -> save_game_name, stack -> root_path, mode, out_path, out_path_size
 *
 * @address 0x551710
 */
uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path, uint32_t out_path_size)
{
    return halo::game::SaveGameFiles::create(save_game_name, root_path, mode, out_path, out_path_size);
}

/**
 * C entry point for halo::game::SaveGameFiles::remove_files; forwards to the C++ implementation.
 * register convention: a validity token in EAX (in_EAX) and the save-game root path in ECX
 * (in_ECX); no stack parameters.
 * // blam-cc: EAX -> save_game_name, ECX -> root_path
 * blam-cc: EAX -> save_game_name, ECX -> root_path
 *
 * @address 0x5519a0
 */
uint32_t XDeleteSaveGame(const uint16_t *save_game_name, const char *root_path)
{
    return halo::game::SaveGameFiles::remove_files(save_game_name, root_path);
}

/**
 * C entry point for halo::game::SaveGameFiles::find_first; forwards to the C++ implementation.
 * register convention: the caller-supplied WIN32_FIND_DATAA (plus 0x140-byte trailing scratch)
 * in EAX (in_EAX); `root_path` is this function's own recognized stack parameter.
 * // blam-cc: EAX -> find_data, stack -> root_path
 * blam-cc: EAX -> find_data, stack -> root_path
 *
 * @address 0x551bc0
 */
int32_t savegame_find_first(char *root_path, win32_find_dataa *find_data)
{
    return halo::game::SaveGameFiles::find_first(root_path, find_data);
}

/**
 * C entry point for halo::game::SaveGameFiles::find_next; forwards to the C++ implementation.
 * register convention: the caller-supplied WIN32_FIND_DATAA (plus scratch, see
 * savegame_find_first.c) in EAX (in_EAX), the find handle in ECX (in_ECX).
 * // blam-cc: EAX -> find_data, ECX -> handle
 * blam-cc: EAX -> find_data, ECX -> handle
 *
 * @address 0x551d30
 */
uint32_t savegame_find_next(win32_find_dataa *find_data, uint32_t handle)
{
    return halo::game::SaveGameFiles::find_next(find_data, handle);
}

/**
 * C entry point for halo::game::SaveGameFiles::slot_handle_pack; forwards to the C++ implementation.
 * register convention: a slot index in EAX (in_EAX, masked to 12 bits), a 4-bit field in ECX
 * (in_ECX), a boolean in DL (in_DL); `flag_bit_31` is this function's own recognized stack
 * parameter.
 * // blam-cc: EAX -> slot_index, ECX -> type_nibble, DL -> flag_bit_30, stack -> flag_bit_31
 *
 * @address 0x53e630
 */
uint32_t savegame_slot_handle_pack(uint32_t slot_index, uint32_t type_nibble, uint8_t flag_bit_30, uint8_t flag_bit_31)
{
    return halo::game::SaveGameFiles::slot_handle_pack(slot_index, type_nibble, flag_bit_30, flag_bit_31);
}

/**
 * C entry point for halo::game::SaveGameIndex::append_slot; forwards to the C++ implementation.
 * register convention: `unused` (Ghidra's `param_1`, never read in the body) and `out_slot_count`
 * are both genuine stack parameters.
 *
 * @address 0x53e300
 */
uint8_t savegame_index_append_slot(const void *entry, uint32_t *out_slot_count)
{
    return halo::game::SaveGameIndex::append_slot(entry, out_slot_count);
}

/**
 * C entry point for halo::game::SaveGameIndex::file_exists; forwards to the C++ implementation.
 * register convention: none -- no stack parameters, no register arguments recovered.
 * blam-cc: ESI -> reference, stack -> mode
 * blam-cc: EAX -> reference
 * blam-cc: EBX -> component, ESI -> reference
 * blam-cc: EBX -> path
 *
 * @address 0x53e060
 */
uint8_t savegame_index_file_exists(void)
{
    return halo::game::SaveGameIndex::file_exists();
}

/**
 * C entry point for halo::game::SaveGameIndex::get_slot_count; forwards to the C++ implementation.
 * register convention: none -- no stack parameters, no register arguments recovered.
 *
 * @address 0x53e420
 */
uint32_t savegame_index_get_slot_count(void)
{
    return halo::game::SaveGameIndex::get_slot_count();
}

/**
 * C entry point for halo::game::SaveGameIndex::read_slot; forwards to the C++ implementation.
 * register convention: a slot index in the low 16 bits of the incoming value (Ghidra's own
 * `ushort param_1`, a genuine stack parameter).
 *
 * @address 0x53e0e0
 */
uint8_t savegame_index_read_slot(uint16_t slot, void *out_entry)
{
    return halo::game::SaveGameIndex::read_slot(slot, out_entry);
}

/**
 * C entry point for halo::game::SaveGameIndex::write_slot; forwards to the C++ implementation.
 * register convention: a slot index in the low 16 bits of the incoming value (Ghidra's own
 * `ushort param_1`, a genuine stack parameter).
 *
 * @address 0x53e1f0
 */
uint8_t savegame_index_write_slot(uint16_t slot, const void *entry)
{
    return halo::game::SaveGameIndex::write_slot(slot, entry);
}

/**
 * C entry point for halo::game::UserSavePaths::lookup; forwards to the C++ implementation.
 * register convention: a user id in ECX (in_ECX); no stack parameters.
 * // blam-cc: ECX -> user_id
 * blam-cc: ECX -> user_id
 *
 * @address 0x5516a0
 */


/**
 * C entry point for halo::game::UserSavePaths::user_save_path_register; forwards to the C++ implementation.
 * register convention: none -- both are genuine stack parameters (Ghidra's own param_1/param_2).
 *
 * @address 0x551650
 */


/**
 * C entry point for halo::game::UserSavePaths::user_save_path_remove; forwards to the C++ implementation.
 * register convention: a user id in EAX (in_EAX); no stack parameters.
 * // blam-cc: EAX -> user_id
 * blam-cc: EAX -> user_id
 *
 * @address 0x5516d0
 */
uint8_t user_save_path_remove(uint32_t user_id)
{
    return halo::game::UserSavePaths::user_save_path_remove(user_id);
}

/**
 * C entry point for halo::game::UnicodeStringLists::get_string; forwards to the C++ implementation.
 * register convention: EAX = path (char *), pushed as tag_lookup's stack argument before EDI is
 * blam-cc: EAX. ECX = index (int16_t; only the low word, aliased through ESI, is ever
 * blam-cc: ECX. The tag group passed to tag_lookup is hardcoded to 'ustr'
 * blam-cc: path in EAX, index in ECX (low 16 bits only)
 *
 * @address 0x4b8d30
 */
wchar_t * unicode_string_list_get_string(char *path, int16_t index)
{
    return halo::game::UnicodeStringLists::get_string(path, index);
}

/**
 * C entry point for halo::game::UserProfile::signin_state_is_valid; forwards to the C++ implementation.
 * register convention: none -- no stack parameters, no register arguments recovered.
 *
 * @address 0x551620
 */
uint32_t user_profile_signin_state_is_valid(void)
{
    return halo::game::UserProfile::signin_state_is_valid();
}

/**
 * C entry point for halo::game::UserProfile::profile_cache_initialize; forwards to the C++ implementation.
 * register convention: no parameters.
 *
 * @address 0x466c20
 */
void player_profile_cache_initialize(void)
{
    halo::game::UserProfile::profile_cache_initialize();
}

/**
 * C entry point for halo::game::UserProfile::customization_slot_set; forwards to the C++ implementation.
 * register convention: fully reconstructed against
 * objdump -d -M intel --start-address=0x4705f0 --stop-address=0x470624 bin/halo.exe
 * blam-cc: ECX -> base, EBX -> new_value, ESI -> key
 *
 * @address 0x4705f0
 */
uint8_t player_customization_slot_set(uint8_t *base, uint8_t new_value, uint32_t key)
{
    return halo::game::UserProfile::customization_slot_set(base, new_value, key);
}

}
