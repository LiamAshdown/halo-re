#include "crt.h"
#include "halo/text/api.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "halo/saved_games/saved_games.hpp"
#include "halo/saved_games/layout.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/text/text.hpp"
#include "halo/core/link.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/platform/file.hpp"

static auto &file_enumeration_pos = halo::link::ref<file_enumeration_position>(halo::saved_games::vars().file_enumeration_pos);
static auto &file_enumeration_flags_value = halo::link::ref<uint32_t>(halo::saved_games::vars().file_enumeration_flags_value);
static auto &file_enumeration_handles = halo::link::ref<void *[8]>(halo::saved_games::vars().file_enumeration_handles);
static auto &file_enumeration_path = halo::link::ref<char [0x100]>(halo::saved_games::vars().file_enumeration_path);
static auto &file_enumeration_find_data = halo::link::ref<win32_find_dataa>(halo::saved_games::vars().file_enumeration_find_data);
static auto &file_root_template = halo::link::ref<char [4]>(halo::saved_games::vars().file_root_template);

/**
 * Closes ref's open handle and clears it. Returns 1 on success, 0 on failure (after reporting
 * the Win32 error, leaving the stale handle in place).
 *
 * @address 0x00555890
 */
uint8_t halo::saved_games::FileReference::close()
{
    file_reference_record *ref = self;
    int32_t ok;

    ok = halo::platform::file_close(ref->handle);
    if (ok != 0) {
        ref->handle = 0;
        return 1;
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

/**
 * Compares the file names of two file references case-insensitively. Each reference is built
 * into its full path, split into components and reduced to its final name before the comparison;
 * the result has the sign of _stricmp. Used as a sort comparator by the hs source rebuild.
 *
 * @address 0x00483d20
 */
int32_t halo::saved_games::FileReference::compare_full_path(const file_reference_record *b) const
{
    const file_reference_record *a = self;
    char scratch[0x100];
    char *dir_start, *ext_fallback, *name_end, *ext_start;
    char name_a[0x104];
    char name_b[0x104];

    scratch[0] = 0;
    {
        int32_t i;
        for (i = 0; i < 0x100; i++) scratch[i] = 0;
    }
    halo::saved_games::path_build_full((char *)a->path, scratch, a->location);
    halo::saved_games::path_split_components(&dir_start, scratch, &ext_fallback, &name_end, &ext_start, a->flags & 1);
    name_a[0] = 0;
    halo::saved_games::path_append_component(name_a, scratch);

    {
        int32_t i;
        for (i = 0; i < 0x100; i++) scratch[i] = 0;
    }
    halo::saved_games::path_build_full((char *)b->path, scratch, b->location);
    halo::saved_games::path_split_components(&dir_start, scratch, &ext_fallback, &name_end, &ext_start, b->flags & 1);
    name_b[0] = 0;
    halo::saved_games::path_append_component(name_b, scratch);

    return _stricmp(name_a, name_b);
}

/**
 * Creates the directory or file described by ref (directory when the is-file bit of flags is
 * clear, file otherwise). Tolerates ERROR_ALREADY_EXISTS (0xb7) for a directory. Returns 1 on
 * success, 0 on failure (after reporting the Win32 error).
 *
 * @address 0x005555b0
 */
uint8_t halo::saved_games::FileReference::create()
{
    file_reference_record *ref = self;
    char full_path[0x800];
    void *handle;
    int32_t created;
    uint32_t error;

    halo::saved_games::path_build_full(ref->path, full_path, ref->location);
    if ((ref->flags & _file_reference_is_file_bit) == 0) {
        created = halo::platform::directory_create(ref->path);
        if (created == 0) {
            error = halo::platform::last_error();
            if (error != win32::k_error_already_exists) {
                halo::saved_games::saved_games_report_last_error();
                return 0;
            }
        }
    } else {
        handle = halo::platform::file_open(full_path, win32::k_generic_write, win32::k_file_share_none, win32::k_create_always, win32::k_file_attribute_normal);
        if (handle == win32::invalid_handle()) {
            halo::saved_games::saved_games_report_last_error();
            return 0;
        }
        halo::platform::file_close(handle);
    }
    return 1;
}

/**
 * Deletes the directory or file described by ref (built to its full path). Returns 1 on
 * success, 0 on failure (after reporting the Win32 error).
 *
 * @address 0x00555670
 */
uint8_t halo::saved_games::FileReference::remove()
{
    file_reference_record *ref = self;
    char full_path[0x800];
    int32_t ok;

    halo::saved_games::path_build_full(ref->path, full_path, ref->location);
    if ((ref->flags & _file_reference_is_file_bit) == 0) {
        ok = halo::platform::directory_remove(full_path);
        if (ok != 0) {
            return 1;
        }
    } else {
        ok = halo::platform::file_set_attributes(full_path, win32::k_file_attribute_normal);
        if (ok != 0) {
            ok = halo::platform::file_delete(full_path);
            if (ok != 0) {
                return 1;
            }
        }
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

/**
 * Returns 1 if ref's full path currently exists on disk, 0 otherwise. Reports the Win32 error
 * unless it is ERROR_FILE_NOT_FOUND (2) or ERROR_PATH_NOT_FOUND (3).
 *
 * @address 0x00555720
 */
uint8_t halo::saved_games::FileReference::exists()
{
    file_reference_record *ref = self;
    char full_path[0x100];
    uint32_t attributes;
    uint32_t error;

    halo::saved_games::path_build_full(ref->path, full_path, ref->location);
    attributes = halo::platform::file_attributes(full_path);
    if (attributes != win32::k_invalid_file_attributes) {
        return 1;
    }
    error = halo::platform::last_error();
    if (error != 2) {
        error = halo::platform::last_error();
        if (error != 3) {
            halo::saved_games::saved_games_report_last_error();
        }
    }
    return 0;
}

/**
 * Returns the size in bytes of ref's open handle, or 0xffffffff on failure (after reporting the
 * Win32 error).
 *
 * @address 0x00555950
 */
uint32_t halo::saved_games::FileReference::get_size()
{
    file_reference_record *ref = self;
    uint32_t size;

    size = halo::platform::file_size(ref->handle, nullptr);
    if (size == win32::k_invalid_file_size) {
        halo::saved_games::saved_games_report_last_error();
    }
    return size;
}

/**
 * Retrieves ref's full-path file size (low dword only) without opening it. Returns 1 on
 * success (storing the size through out_size), 0 on failure (after reporting the Win32 error).
 *
 * @address 0x00555b00
 */
uint8_t halo::saved_games::FileReference::get_size_by_path(uint32_t *out_size)
{
    file_reference_record *ref = self;
    char full_path[0x800];
    win32_file_attribute_data attributes;
    int32_t ok;

    halo::saved_games::path_build_full(ref->path, full_path, ref->location);
    ok = GetFileAttributesExA(full_path, (GET_FILEEX_INFO_LEVELS)(0 ), &attributes);
    if (ok != 0) {
        *out_size = attributes.file_size_low;
        return 1;
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

/**
 * Opens ref's full path with CreateFileA (share mode FILE_SHARE_READ, OPEN_ALWAYS,
 * FILE_ATTRIBUTE_NORMAL), storing the handle in ref->handle. If _file_open_append is set, seeks
 * to the end of the file; on failure to do so, closes and clears the handle. Returns 1 on
 * success, 0 on failure (after reporting the Win32 error).
 *
 * @address 0x005557a0
 */
uint8_t halo::saved_games::FileReference::open(uint8_t mode)
{
    file_reference_record *ref = self;
    char full_path[0x800];
    void *handle;
    uint32_t desired_access;
    uint32_t seek_result;

    halo::saved_games::path_build_full(ref->path, full_path, ref->location);
    desired_access = 0;
    if ((mode & _file_open_read) != 0) {
        desired_access = win32::k_generic_read;
    }
    if ((mode & _file_open_write) != 0) {
        desired_access = desired_access | win32::k_generic_write;
    }
    handle = halo::platform::file_open(full_path, desired_access, win32::k_file_share_read, win32::k_open_existing, win32::k_file_attribute_normal);
    if (handle != win32::invalid_handle()) {
        ref->handle = handle;
        if ((mode & _file_open_append) == 0) {
            return 1;
        }
        seek_result = halo::platform::file_seek(handle, 0, nullptr, win32::k_file_end);
        if (seek_result != win32::k_invalid_set_file_pointer) {
            return 1;
        }
        halo::platform::file_close(ref->handle);
        ref->handle = 0;
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

/**
 * Reads exactly size bytes from ref's open handle into buffer. Fails (ERROR_HANDLE_EOF, 0x26)
 * if fewer bytes were read than requested. Returns 1 on success, 0 on failure (after reporting
 * the Win32 error).
 *
 * @address 0x00555a20
 */
uint8_t halo::saved_games::FileReference::read(void *buffer, uint32_t size)
{
    file_reference_record *ref = self;
    int32_t ok;
    uint32_t bytes_read;

    ok = halo::platform::file_read(ref->handle, buffer, size, &bytes_read);
    if (ok != 0) {
        if (bytes_read == size) {
            return 1;
        }
        halo::platform::set_last_error(win32::k_error_handle_eof);
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

/**
 * Seeks ref's open handle to an absolute byte offset. Returns 1 on success, 0 on failure (after
 * reporting the Win32 error).
 *
 * @address 0x005558f0
 */
uint8_t halo::saved_games::FileReference::seek(int32_t offset)
{
    file_reference_record *ref = self;
    uint32_t result;

    result = halo::platform::file_seek(ref->handle, offset, nullptr, win32::k_file_begin);
    if (result == win32::k_invalid_set_file_pointer) {
        halo::saved_games::saved_games_report_last_error();
    }
    return result != win32::k_invalid_set_file_pointer;
}

/**
 * Seeks ref's open handle to offset, then truncates or extends the file to that length.
 * Returns 1 on success, 0 on failure (after reporting the Win32 error).
 *
 * @address 0x005559b0
 */
uint8_t halo::saved_games::FileReference::set_length(int32_t offset)
{
    file_reference_record *ref = self;
    uint8_t seeked;
    int32_t ok;

    seeked = halo::saved_games::file_reference_seek(offset, ref);
    if (seeked != 0) {
        ok = halo::platform::file_truncate(ref->handle);
        if (ok != 0) {
            return 1;
        }
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

/**
 * Writes exactly size bytes from buffer to ref's open handle. Returns 1 on success, 0 on
 * failure (after reporting the Win32 error).
 *
 * @address 0x00555a90
 */
uint8_t halo::saved_games::FileReference::write(const void *buffer, uint32_t size)
{
    file_reference_record *ref = self;
    int32_t ok;
    uint32_t bytes_written;

    ok = halo::platform::file_write(ref->handle, buffer, size, &bytes_written);
    if (ok != 0 && bytes_written == size) {
        return 1;
    }
    halo::saved_games::saved_games_report_last_error();
    return 0;
}

namespace halo::saved_games::directory {

/**
 * Builds a fresh, relative (_file_location_relative) file_reference_record naming
 * directory_path. If it doesn't exist yet, creates it. If it does, enumerates every entry
 * directly inside it and deletes each one (the directory itself is left in place, now empty).
 *
 * @address 0x00555520
 */
void ensure_empty(const char *directory_path)
{
    file_reference_record dir_ref;
    file_reference_record entry;
    uint8_t exists;
    uint8_t found;

    memset(&dir_ref, 0, sizeof(dir_ref));
    dir_ref.signature = k_file_reference_signature;
    dir_ref.location = _file_location_relative;
    halo::saved_games::path_append_component(dir_ref.path, directory_path);

    exists = halo::saved_games::file_reference_exists(&dir_ref);
    if (!exists) {
        halo::saved_games::file_reference_create(&dir_ref);
    } else {
        halo::saved_games::file_enumerate_start(0, &dir_ref);
        found = halo::saved_games::file_enumerate_find_next(&entry, 0);
        if (found != 0) {
            do {
                halo::saved_games::file_reference_delete(&entry);
                found = halo::saved_games::file_enumerate_find_next(&entry, 0);
            } while (found != 0);
            return;
        }
    }
    return;
}

}  // namespace halo::saved_games::directory

namespace halo::saved_games::file_enumerate {

/**
 * Advances the (possibly recursive) enumeration started by file_enumerate_start and returns the
 * next matching entry through out_entry (built as a full file_reference_record naming it) and,
 * if out_write_time is non-null, its FILETIME. Recurses into subdirectories when the recursive
 * flag is set, skips "." and "..", and honors the directories-only flag. Returns 1 while there
 * is a match, 0 once the (possibly nested) enumeration is exhausted.
 *
 * @address 0x00555c10
 */
uint8_t find_next(file_reference_record *out_entry, uint32_t *out_write_time)
{
    char search_path[0x100];
    int32_t depth;
    char *end;
    char *dest;
    uint32_t remaining;
    void *handle;
    int32_t found;
    int16_t location;
    int32_t is_dot;

    depth = file_enumeration_pos.depth;
    if (file_enumeration_pos.depth < 0) {
        return 0;
    }

    for (;;) {
        bool level_exhausted = false;

        if (file_enumeration_handles[depth] == (void *)-1) {
            halo::saved_games::path_build_full(file_enumeration_path, search_path, file_enumeration_pos.location);
            end = search_path;
            while (*end != '\0') {
                end++;
            }
            if (end != search_path) {
                *end = '\\';
                end++;
                *end = '\0';
            }
            dest = search_path;
            while (*dest != '\0') {
                dest++;
            }
            remaining = (uint32_t)(0xff - ((int32_t)dest - (int32_t)search_path));
            strncpy(dest, "*.*", remaining);
            search_path[0xff] = '\0';
            handle = halo::platform::find_first(search_path, &file_enumeration_find_data);
            file_enumeration_handles[depth] = handle;
            level_exhausted = handle == win32::invalid_handle();
        } else {
            found = halo::platform::find_next(file_enumeration_handles[depth], &file_enumeration_find_data);
            if (found == 0) {
                halo::platform::find_close(file_enumeration_handles[depth]);
                file_enumeration_handles[depth] = (void *)-1;
                level_exhausted = true;
            }
        }

        if (level_exhausted) {
            halo::saved_games::path_remove_last_component(file_enumeration_path);
            depth--;
            if (depth < 0) {
                file_enumeration_pos.depth = (int16_t)depth;
                return 0;
            }
            continue;
        }

        location = file_enumeration_pos.location;
        if ((file_enumeration_find_data.dwFileAttributes & win32::k_file_attribute_directory) == 0) {
            if ((file_enumeration_flags_value & 2) == 0) {
                memset(out_entry, 0, sizeof(*out_entry));
                out_entry->signature = k_file_reference_signature;
                out_entry->location = location;
                halo::saved_games::path_append_component(out_entry->path, file_enumeration_path);
                if ((out_entry->flags & _file_reference_is_file_bit) != 0) {
                    halo::saved_games::path_remove_last_component(out_entry->path);
                }
                halo::saved_games::path_append_component(out_entry->path, file_enumeration_find_data.cFileName);
                out_entry->flags = out_entry->flags | _file_reference_is_file_bit;
                if (out_write_time != 0) {
                    out_write_time[0] = file_enumeration_find_data.ftLastWriteTime[0];
                    out_write_time[1] = file_enumeration_find_data.ftLastWriteTime[1];
                }
                file_enumeration_pos.depth = (int16_t)depth;
                return 1;
            }
        } else {
            is_dot = (file_enumeration_find_data.cFileName[0] == '.' &&
                      file_enumeration_find_data.cFileName[1] == '\0');
            if (!is_dot) {
                is_dot = (file_enumeration_find_data.cFileName[0] == '.' &&
                          file_enumeration_find_data.cFileName[1] == '.' &&
                          file_enumeration_find_data.cFileName[2] == '\0');
            }
            if (!is_dot) {
                if ((file_enumeration_flags_value & 2) != 0) {
                    memset(out_entry, 0, sizeof(*out_entry));
                    out_entry->signature = k_file_reference_signature;
                    out_entry->location = location;
                    halo::saved_games::path_append_component(out_entry->path, file_enumeration_path);
                    halo::saved_games::path_append_component(out_entry->path, file_enumeration_find_data.cFileName);
                }
                if ((file_enumeration_flags_value & 1) != 0) {
                    if ((file_enumeration_flags_value & 2) == 0) {
                        halo::saved_games::path_append_component(file_enumeration_path, file_enumeration_find_data.cFileName);
                    }
                    depth++;
                }
                if ((file_enumeration_flags_value & 2) != 0) {
                    if (out_write_time != 0) {
                        out_write_time[0] = file_enumeration_find_data.ftLastWriteTime[0];
                        out_write_time[1] = file_enumeration_find_data.ftLastWriteTime[1];
                    }
                    file_enumeration_pos.depth = (int16_t)depth;
                    return 1;
                }
            }
        }
        if (depth < 0) {
            file_enumeration_pos.depth = (int16_t)depth;
            return 0;
        }
    }
}

/**
 * Closes any find handles still open from a previous (possibly recursive) enumeration up to and
 * including the current depth, then starts a fresh one: stores flags, resets the depth to 0 and
 * the location to ref->location, and copies ref->path into the shared enumeration path buffer.
 *
 * @address 0x00555b90
 */
void start(uint32_t flags, file_reference_record *ref)
{
    int32_t depth;
    int32_t handle_count;
    char *dest;
    const char *src;

    if (file_enumeration_pos.depth >= 0) {
        depth = file_enumeration_pos.depth;
        handle_count = (uint16_t)(file_enumeration_pos.depth + 1);
        do {
            if (file_enumeration_handles[depth] != (void *)-1) {
                halo::platform::find_close(file_enumeration_handles[depth]);
                file_enumeration_handles[depth] = (void *)-1;
            }
            depth--;
            handle_count--;
        } while (handle_count != 0);
    }
    file_enumeration_flags_value = flags;
    file_enumeration_pos.depth = 0;
    file_enumeration_pos.location = ref->location;
    dest = file_enumeration_path;
    src = ref->path;
    do {
        *dest = *src;
        dest++;
    } while (*src++ != '\0');
    return;
}

}  // namespace halo::saved_games::file_enumerate

namespace halo::saved_games::file_reference {

/**
 * Zeroes the record, sets its signature and location (always _file_location_absolute), then
 * either appends component as the whole path (is_file != 0: the caller is naming a single
 * file, no prior path to trim) or, only if flags already has the is-file bit set from a prior
 * call, removes the previous trailing component first, then appends component and sets the
 * is-file bit. Returns ref.
 *
 * @address 0x005554c0
 */
file_reference_record *init(file_reference_record *ref, const char *component, uint8_t is_file)
{
    uint8_t *zero_cursor;
    int32_t i;

    zero_cursor = (uint8_t *)ref;
    for (i = 0x43; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    ref->signature = k_file_reference_signature;
    ref->location = _file_location_absolute;
    if (is_file != 0) {
        halo::saved_games::path_append_component(ref->path, component);
        return ref;
    }
    if ((ref->flags & _file_reference_is_file_bit) != 0) {
        halo::saved_games::path_remove_last_component(ref->path);
    }
    halo::saved_games::path_append_component(ref->path, component);
    ref->flags = ref->flags | _file_reference_is_file_bit;
    return ref;
}

}  // namespace halo::saved_games::file_reference

namespace halo::saved_games::path {

/**
 * Appends component to destination after a backslash separator (omitted when destination is
 * empty). At most 0xff characters of the result are kept and the buffer is always terminated.
 *
 * @address 0x00555ec0
 */
void append_component(char *destination, const char *component)
{
    char *end;

    if (*component != '\0') {
        end = destination;
        while (*end != '\0') {
            end++;
        }
        if (end != destination) {
            *end = '\\';
            end++;
            *end = '\0';
        }
        strncpy(end, component, 0xff - (uint32_t)(end - destination));
        destination[0xff] = '\0';
    }
    return;
}

/**
 * Appends suffix to destination after a '.' separator, e.g. to add a file extension. Same
 * bounds as path_append_component: at most 0xff characters are kept and the result is terminated.
 *
 * @address 0x00555f20
 */
void append_extension(char *destination, const char *suffix)
{
    char *end;

    if (*suffix != '\0') {
        end = destination;
        while (*end != '\0') {
            end++;
        }
        if (end != destination) {
            *end = '.';
            end++;
            *end = '\0';
        }
        strncpy(end, suffix, 0xff - (uint32_t)(end - destination));
        destination[0xff] = '\0';
    }
    return;
}

/**
 * Builds the full path of a file reference path for the given location. Location 2 copies the
 * path unchanged, a positive location prefixes the root template, and otherwise a path that is not
 * rooted gets a leading ".\".
 *
 * @address 0x005560d0
 */
void build_full(char *source, char *destination, int16_t location)
{
    destination[0] = 0;
    if (location == 2) {
        strcpy(destination, source);
        return;
    }
    if (location > 0) {
        strcpy(destination, file_root_template);
        strncat(destination, source, 0xfb);
        return;
    }
    if (source[1] != '\\' && source[1] != ':') {
        memcpy(destination, ".\\", 3);
    }
    strcat(destination, source);
}

/**
 * Truncates path in place by removing its trailing path component: scans backward from the end
 * (respecting double-byte characters) for the rightmost '\\', then NULs the string at (or just
 * after, if no backslash was found) that position.
 *
 * @address 0x00555f80
 */
void remove_last_component(char *path)
{
    char *end;
    int16_t length;
    int16_t remaining;
    int16_t ch;
    int16_t last_backslash_pos;
    char *at;
    uint8_t is_double_byte;
    uint16_t final_char;
    int16_t final_width;

    end = path;
    while (*end != '\0') {
        end++;
    }
    length = (int16_t)(end - path);

    remaining = length;
    do {
        last_backslash_pos = 0;
        if (remaining == 0) {
            break;
        }
        ch = halo::text::dbcs_text::find_character_boundary(reinterpret_cast<uint8_t *>(path), &remaining);
        last_backslash_pos = remaining;
    } while (ch != '\\');

    at = path + last_backslash_pos;
    is_double_byte = halo::text::dbcs_text::char_is_double_byte(reinterpret_cast<uint8_t *>(at));
    if (!is_double_byte) {
        final_char = (uint16_t)(uint8_t)*at;
        final_width = 1;
    } else {
        final_width = 2;
        final_char = (uint16_t)(((uint8_t)at[0] << 8) | (uint8_t)at[1]);
    }
    if (final_char == '\\') {
        path[last_backslash_pos + final_width - 1] = '\0';
        return;
    }
    path[last_backslash_pos + final_width] = '\0';
    return;
}

/**
 * Splits a file path in place, scanning backwards in multi-byte aware steps. The extension and
 * the last backslash are cut with NUL bytes, and the out pointers receive the directory start, the
 * name end and the extension start.
 *
 * @address 0x00556000
 */
void split_components(char **dir_start_out, char *path, char **ext_fallback_out, char **name_end_out,
    char **ext_start_out, uint8_t split_extension)
{
    int16_t length = (int16_t)strlen(path);
    char *end = path + length;

    *name_end_out = end;
    *dir_start_out = end;
    *ext_fallback_out = end;
    *ext_start_out = end;
    while (length != 0) {
        uint16_t character = halo::text::dbcs_text::find_character_boundary((uint8_t *)path, &length);

        if (character == '.') {
            if (split_extension && **ext_fallback_out == 0 && **ext_start_out == 0) {
                path[length] = 0;
                *ext_start_out = path + length + 1;
            }
        } else if (character == '\\') {
            if (split_extension && **ext_fallback_out == 0) {
                path[length] = 0;
                *ext_fallback_out = path + length + 1;
            } else if (**dir_start_out == 0) {
                *dir_start_out = path + length + 1;
            }
        }
    }
    if (split_extension && **ext_fallback_out == 0) {
        *ext_fallback_out = path;
        return;
    }
    if (*ext_fallback_out != path) {
        *name_end_out = path;
    }
}

}  // namespace halo::saved_games::path

namespace halo::saved_games::saved_game {

/**
 * Formats the current Win32 last-error code into a discarded 0x800-byte scratch buffer (the
 * message text is never read back -- only the FormatMessageA call itself matters, presumably
 * for its side effect of validating/consuming the error), then clears the last-error code.
 *
 * @address 0x00556170
 */
void report_last_error(void)
{
    uint32_t message_id;
    char scratch[0x800];

    message_id = halo::platform::last_error();
    FormatMessageA(win32::k_format_message_system_message, 0, message_id, 0, (LPSTR)scratch, sizeof(scratch), 0);
    halo::platform::set_last_error(0);
    return;
}

}  // namespace halo::saved_games::saved_game
