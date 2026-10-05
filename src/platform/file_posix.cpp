/**
 * @file src/platform/file_posix.cpp
 * halo::platform files on POSIX (Emscripten, Linux) with the Win32 behaviour the engine expects: Win32 open modes and
 * dispositions, error codes, FILETIME times, FindFirstFile patterns and records, and ReadFileEx completions delivered
 * on the reading thread's next alertable wait. Engine paths may use backslashes and any letter case.
 */

#include "halo/platform/file.hpp"
#include "posix_internal.hpp"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct file_time {
    uint32_t low_date_time;
    uint32_t high_date_time;
};

struct win32_find_dataa {
    uint32_t dwFileAttributes;
    uint32_t ftCreationTime[2];
    uint32_t ftLastAccessTime[2];
    uint32_t ftLastWriteTime[2];
    uint32_t nFileSizeHigh;
    uint32_t nFileSizeLow;
    uint32_t dwReserved0;
    uint32_t dwReserved1;
    char cFileName[260];
    char cAlternateFileName[14];
    uint8_t pad[2];
};
static_assert(sizeof(win32_find_dataa) == 0x140, "WIN32_FIND_DATAA layout");

namespace halo::platform {

namespace posix {

namespace {

thread_local uint32_t t_last_error;

constexpr uint32_t k_generic_read = 0x80000000;
constexpr uint32_t k_generic_write = 0x40000000;
constexpr uint32_t k_attribute_readonly = 0x01;
constexpr uint32_t k_attribute_directory = 0x10;
constexpr uint32_t k_attribute_archive = 0x20;
constexpr uint64_t k_filetime_unix_epoch = 116444736000000000ull;  // 1970-01-01 in 100 ns units since 1601

struct file_object {
    handle_header header;
    int fd;
};

struct find_object {
    handle_header header;
    DIR *directory;
    char directory_path[1024];
    char pattern[260];
};

file_object *as_file(file_handle file)
{
    file_object *object = static_cast<file_object *>(file);

    return object != nullptr && file != invalid_handle() && object->header.kind == kind_file ? object : nullptr;
}

uint64_t to_filetime(const timespec &time)
{
    return k_filetime_unix_epoch + static_cast<uint64_t>(time.tv_sec) * 10000000ull + static_cast<uint64_t>(time.tv_nsec) / 100;
}

void store_filetime(uint64_t value, uint32_t *out)
{
    out[0] = static_cast<uint32_t>(value);
    out[1] = static_cast<uint32_t>(value >> 32);
}

/** Win32 wildcard match ('*', '?'), ignoring case; "*.*" matches every name as on Windows. */
bool wildcard_match(const char *pattern, const char *name)
{
    if (strcmp(pattern, "*.*") == 0) {
        pattern = "*";
    }
    while (*pattern != '\0') {
        if (*pattern == '*') {
            pattern++;
            if (*pattern == '\0') return true;
            for (; *name != '\0'; name++) {
                if (wildcard_match(pattern, name)) return true;
            }
            return false;
        }
        if (*name == '\0' || (*pattern != '?' && tolower(static_cast<unsigned char>(*pattern)) != tolower(static_cast<unsigned char>(*name)))) {
            return false;
        }
        pattern++;
        name++;
    }
    return *name == '\0';
}

/** Fills a WIN32_FIND_DATAA record for the entry name of directory. */
void fill_find_data(const char *directory, const char *name, win32_find_dataa *data)
{
    char path[1300];
    struct stat info;

    memset(data, 0, sizeof(*data));
    snprintf(path, sizeof(path), "%s/%s", directory, name);
    if (stat(path, &info) == 0) {
        data->dwFileAttributes = S_ISDIR(info.st_mode) ? k_attribute_directory : k_attribute_archive;
        if ((info.st_mode & S_IWUSR) == 0) data->dwFileAttributes |= k_attribute_readonly;
        store_filetime(to_filetime(info.st_mtim), data->ftCreationTime);
        store_filetime(to_filetime(info.st_atim), data->ftLastAccessTime);
        store_filetime(to_filetime(info.st_mtim), data->ftLastWriteTime);
        data->nFileSizeHigh = static_cast<uint32_t>(static_cast<uint64_t>(info.st_size) >> 32);
        data->nFileSizeLow = static_cast<uint32_t>(info.st_size);
    }
    strncpy(data->cFileName, name, sizeof(data->cFileName) - 1);
}

/** The next entry of an open listing that matches its pattern, or false. */
bool next_match(find_object *find, win32_find_dataa *data)
{
    struct dirent *entry;

    while ((entry = readdir(find->directory)) != nullptr) {
        if (wildcard_match(find->pattern, entry->d_name)) {
            fill_find_data(find->directory_path, entry->d_name, data);
            return true;
        }
    }
    return false;
}

}  // namespace

void set_last_error_from_errno()
{
    switch (errno) {
    case ENOENT: t_last_error = k_error_file_not_found; break;
    case ENOTDIR: t_last_error = k_error_path_not_found; break;
    case EACCES: case EPERM: case EROFS: case EISDIR: t_last_error = k_error_access_denied; break;
    case EEXIST: t_last_error = k_error_file_exists; break;
    case ENOSPC: t_last_error = k_error_disk_full; break;
    case ENOMEM: t_last_error = k_error_not_enough_memory; break;
    case EBADF: t_last_error = k_error_invalid_handle; break;
    case ENOTEMPTY: t_last_error = k_error_dir_not_empty; break;
    default: t_last_error = k_error_invalid_parameter; break;
    }
}

void native_path(const char *path, char *out, uint32_t size)
{
    char *component;
    char *save = nullptr;
    char resolved[1024];
    size_t resolved_length = 0;

    snprintf(out, size, "%s", path);
    for (char *c = out; *c != '\0'; c++) {
        if (*c == '\\') *c = '/';
    }
    if (out[0] == '\0' || access(out, F_OK) == 0) {
        return;
    }
    // match each component case-insensitively against what exists
    resolved[0] = '\0';
    if (out[0] == '/') {
        resolved[resolved_length++] = '/';
        resolved[resolved_length] = '\0';
    }
    component = strtok_r(out, "/", &save);
    while (component != nullptr) {
        char *next = strtok_r(nullptr, "/", &save);
        const char *chosen = component;
        DIR *directory = opendir(resolved_length != 0 ? resolved : ".");
        struct dirent *entry;

        if (directory != nullptr) {
            while ((entry = readdir(directory)) != nullptr) {
                if (strcasecmp(entry->d_name, component) == 0) {
                    chosen = entry->d_name;
                    break;
                }
            }
        }
        resolved_length += static_cast<size_t>(snprintf(resolved + resolved_length, sizeof(resolved) - resolved_length, "%s%s",
            resolved_length != 0 && resolved[resolved_length - 1] != '/' ? "/" : "", chosen));
        if (directory != nullptr) {
            closedir(directory);
        }
        component = next;
        if (resolved_length >= sizeof(resolved) - 1) break;
    }
    snprintf(out, size, "%s", resolved);
}

}  // namespace posix

using namespace posix;

file_handle file_open(const char *path, uint32_t access_mode, uint32_t share, uint32_t disposition, uint32_t flags)
{
    char native[1024];
    int open_flags;
    int fd;
    bool existed;
    struct stat info;
    file_object *object;

    (void)share;
    (void)flags;
    native_path(path, native, sizeof(native));
    existed = stat(native, &info) == 0;
    if (existed && S_ISDIR(info.st_mode)) {
        t_last_error = k_error_access_denied;
        return invalid_handle();
    }
    open_flags = (access_mode & k_generic_write) != 0 ? ((access_mode & k_generic_read) != 0 ? O_RDWR : O_WRONLY) : O_RDONLY;
    switch (disposition) {
    case 1: open_flags |= O_CREAT | O_EXCL; break;   // CREATE_NEW
    case 2: open_flags |= O_CREAT | O_TRUNC; break;  // CREATE_ALWAYS
    case 4: open_flags |= O_CREAT; break;            // OPEN_ALWAYS
    case 5: open_flags |= O_TRUNC; break;            // TRUNCATE_EXISTING
    default: break;                                  // OPEN_EXISTING
    }
    fd = open(native, open_flags, 0644);
    if (fd < 0) {
        set_last_error_from_errno();
        return invalid_handle();
    }
    object = static_cast<file_object *>(malloc(sizeof(file_object)));
    object->header.kind = kind_file;
    object->fd = fd;
    // CREATE_ALWAYS / OPEN_ALWAYS report an existing file through the last error, as CreateFileA does
    t_last_error = existed && (disposition == 2 || disposition == 4) ? k_error_already_exists : k_error_success;
    return object;
}

bool file_read(file_handle file, void *buffer, uint32_t size, uint32_t *bytes_read)
{
    file_object *object = as_file(file);
    uint32_t total = 0;

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    while (total < size) {
        ssize_t count = read(object->fd, static_cast<char *>(buffer) + total, size - total);

        if (count < 0) {
            if (errno == EINTR) continue;
            set_last_error_from_errno();
            if (bytes_read != nullptr) *bytes_read = total;
            return false;
        }
        if (count == 0) break;
        total += static_cast<uint32_t>(count);
    }
    if (bytes_read != nullptr) *bytes_read = total;
    return true;
}

bool file_write(file_handle file, const void *buffer, uint32_t size, uint32_t *bytes_written)
{
    file_object *object = as_file(file);
    uint32_t total = 0;

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    while (total < size) {
        ssize_t count = write(object->fd, static_cast<const char *>(buffer) + total, size - total);

        if (count < 0) {
            if (errno == EINTR) continue;
            set_last_error_from_errno();
            if (bytes_written != nullptr) *bytes_written = total;
            return false;
        }
        total += static_cast<uint32_t>(count);
    }
    if (bytes_written != nullptr) *bytes_written = total;
    return true;
}

uint32_t file_seek(file_handle file, int32_t distance, int32_t *distance_high, uint32_t method)
{
    file_object *object = as_file(file);
    int64_t offset = distance_high != nullptr ? static_cast<int64_t>((static_cast<uint64_t>(static_cast<uint32_t>(*distance_high)) << 32) | static_cast<uint32_t>(distance))
                                              : distance;
    off_t result;

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return 0xffffffff;
    }
    result = lseek(object->fd, static_cast<off_t>(offset), method == 1 ? SEEK_CUR : method == 2 ? SEEK_END : SEEK_SET);
    if (result < 0) {
        set_last_error_from_errno();
        return 0xffffffff;
    }
    if (distance_high != nullptr) {
        *distance_high = static_cast<int32_t>(static_cast<uint64_t>(result) >> 32);
    }
    t_last_error = k_error_success;
    return static_cast<uint32_t>(result);
}

uint32_t file_size(file_handle file, uint32_t *size_high)
{
    file_object *object = as_file(file);
    struct stat info;

    if (object == nullptr || fstat(object->fd, &info) != 0) {
        t_last_error = k_error_invalid_handle;
        return 0xffffffff;
    }
    if (size_high != nullptr) {
        *size_high = static_cast<uint32_t>(static_cast<uint64_t>(info.st_size) >> 32);
    }
    return static_cast<uint32_t>(info.st_size);
}

bool file_truncate(file_handle file)
{
    file_object *object = as_file(file);

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    if (ftruncate(object->fd, lseek(object->fd, 0, SEEK_CUR)) != 0) {
        set_last_error_from_errno();
        return false;
    }
    return true;
}

bool file_close(file_handle file)
{
    file_object *object = as_file(file);

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    close(object->fd);
    object->header.kind = static_cast<handle_kind>(0);
    free(object);
    return true;
}

bool file_get_creation_time(file_handle file, file_time *time)
{
    file_object *object = as_file(file);
    struct stat info;
    uint64_t value;

    if (object == nullptr || fstat(object->fd, &info) != 0) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    value = to_filetime(info.st_mtim);  // POSIX keeps no creation time: the modification time stands in
    time->low_date_time = static_cast<uint32_t>(value);
    time->high_date_time = static_cast<uint32_t>(value >> 32);
    return true;
}

bool file_set_creation_time(file_handle file, const file_time *time)
{
    file_object *object = as_file(file);
    uint64_t value = (static_cast<uint64_t>(time->high_date_time) << 32) | time->low_date_time;
    uint64_t since_epoch = value > k_filetime_unix_epoch ? value - k_filetime_unix_epoch : 0;
    struct timespec times[2];

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    times[0].tv_sec = static_cast<time_t>(since_epoch / 10000000);
    times[0].tv_nsec = static_cast<long>(since_epoch % 10000000) * 100;
    times[1] = times[0];
    if (futimens(object->fd, times) != 0) {
        set_last_error_from_errno();
        return false;
    }
    return true;
}

int32_t file_time_compare(const file_time *a, const file_time *b)
{
    uint64_t x = (static_cast<uint64_t>(a->high_date_time) << 32) | a->low_date_time;
    uint64_t y = (static_cast<uint64_t>(b->high_date_time) << 32) | b->low_date_time;

    return x < y ? -1 : x > y ? 1 : 0;
}

void file_time_now(file_time *time)
{
    struct timespec now;
    uint64_t value;

    clock_gettime(CLOCK_REALTIME, &now);
    value = to_filetime(now);
    time->low_date_time = static_cast<uint32_t>(value);
    time->high_date_time = static_cast<uint32_t>(value >> 32);
}

bool file_delete(const char *path)
{
    char native[1024];

    native_path(path, native, sizeof(native));
    if (unlink(native) != 0) {
        set_last_error_from_errno();
        return false;
    }
    return true;
}

bool file_copy(const char *source, const char *destination, bool fail_if_exists)
{
    char from[1024];
    char to[1024];
    char buffer[65536];
    int in;
    int out;
    ssize_t count;
    bool ok = true;

    native_path(source, from, sizeof(from));
    native_path(destination, to, sizeof(to));
    in = open(from, O_RDONLY);
    if (in < 0) {
        set_last_error_from_errno();
        return false;
    }
    out = open(to, O_WRONLY | O_CREAT | O_TRUNC | (fail_if_exists ? O_EXCL : 0), 0644);
    if (out < 0) {
        set_last_error_from_errno();
        close(in);
        return false;
    }
    while ((count = read(in, buffer, sizeof(buffer))) > 0) {
        if (write(out, buffer, static_cast<size_t>(count)) != count) {
            set_last_error_from_errno();
            ok = false;
            break;
        }
    }
    close(in);
    close(out);
    return ok && count >= 0;
}

uint32_t file_attributes(const char *path)
{
    char native[1024];
    struct stat info;
    uint32_t attributes;

    native_path(path, native, sizeof(native));
    if (stat(native, &info) != 0) {
        set_last_error_from_errno();
        return 0xffffffff;
    }
    attributes = S_ISDIR(info.st_mode) ? k_attribute_directory : k_attribute_archive;
    return (info.st_mode & S_IWUSR) == 0 ? attributes | k_attribute_readonly : attributes;
}

bool file_set_attributes(const char *path, uint32_t attributes)
{
    char native[1024];
    struct stat info;

    native_path(path, native, sizeof(native));
    if (stat(native, &info) != 0) {
        set_last_error_from_errno();
        return false;
    }
    chmod(native, (attributes & k_attribute_readonly) != 0 ? (info.st_mode & ~0222u) : (info.st_mode | S_IWUSR));
    return true;
}

bool file_size_by_path(const char *path, uint32_t *size)
{
    char native[1024];
    struct stat info;

    native_path(path, native, sizeof(native));
    if (stat(native, &info) != 0) {
        set_last_error_from_errno();
        return false;
    }
    *size = static_cast<uint32_t>(info.st_size);
    return true;
}

bool directory_create(const char *path)
{
    char native[1024];

    native_path(path, native, sizeof(native));
    if (mkdir(native, 0755) != 0) {
        set_last_error_from_errno();
        if (errno == EEXIST) t_last_error = k_error_already_exists;
        return false;
    }
    return true;
}

bool directory_remove(const char *path)
{
    char native[1024];

    native_path(path, native, sizeof(native));
    if (rmdir(native) != 0) {
        set_last_error_from_errno();
        return false;
    }
    return true;
}

uint32_t current_directory(uint32_t size, char *buffer)
{
    char path[1024];
    uint32_t length;

    if (getcwd(path, sizeof(path)) == nullptr) {
        set_last_error_from_errno();
        return 0;
    }
    length = static_cast<uint32_t>(strlen(path));
    if (length + 1 > size) {
        return length + 1;  // the size needed, as GetCurrentDirectoryA
    }
    for (uint32_t i = 0; i <= length; i++) {
        buffer[i] = path[i] == '/' ? '\\' : path[i];  // the engine splits paths at backslashes
    }
    return length;
}

bool disk_free_space(const char *path, uint64_t *available_to_caller, uint64_t *total, uint64_t *free_bytes)
{
    char native[1024];
    struct statvfs info;

    native_path(path != nullptr ? path : ".", native, sizeof(native));
    if (statvfs(native, &info) != 0) {
        set_last_error_from_errno();
        return false;
    }
    if (available_to_caller != nullptr) *available_to_caller = static_cast<uint64_t>(info.f_bavail) * info.f_frsize;
    if (total != nullptr) *total = static_cast<uint64_t>(info.f_blocks) * info.f_frsize;
    if (free_bytes != nullptr) *free_bytes = static_cast<uint64_t>(info.f_bfree) * info.f_frsize;
    return true;
}

file_handle find_first(const char *pattern, win32_find_dataa *data)
{
    char native[1024];
    char *slash;
    find_object *find = static_cast<find_object *>(calloc(1, sizeof(find_object)));

    // split "dir\\*.sav" into the directory and the wildcard
    snprintf(native, sizeof(native), "%s", pattern);
    for (char *c = native; *c != '\0'; c++) {
        if (*c == '\\') *c = '/';
    }
    slash = strrchr(native, '/');
    if (slash != nullptr) {
        snprintf(find->pattern, sizeof(find->pattern), "%s", slash + 1);
        *slash = '\0';
        native_path(native[0] != '\0' ? native : "/", find->directory_path, sizeof(find->directory_path));
    } else {
        snprintf(find->pattern, sizeof(find->pattern), "%s", native);
        snprintf(find->directory_path, sizeof(find->directory_path), ".");
    }
    find->header.kind = kind_find;
    find->directory = opendir(find->directory_path);
    if (find->directory == nullptr) {
        set_last_error_from_errno();
        if (t_last_error == k_error_file_not_found) t_last_error = k_error_path_not_found;
        free(find);
        return invalid_handle();
    }
    if (!next_match(find, data)) {
        closedir(find->directory);
        free(find);
        t_last_error = k_error_file_not_found;
        return invalid_handle();
    }
    return find;
}

bool find_next(file_handle find, win32_find_dataa *data)
{
    find_object *object = static_cast<find_object *>(find);

    if (object == nullptr || find == invalid_handle() || object->header.kind != kind_find) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    if (!next_match(object, data)) {
        t_last_error = k_error_no_more_files;
        return false;
    }
    return true;
}

bool find_close(file_handle find)
{
    find_object *object = static_cast<find_object *>(find);

    if (object == nullptr || find == invalid_handle() || object->header.kind != kind_find) {
        t_last_error = k_error_invalid_handle;
        return false;
    }
    closedir(object->directory);
    object->header.kind = static_cast<handle_kind>(0);
    free(object);
    return true;
}

int32_t file_read_async(void *file, void *buffer, uint32_t size, void *request, void *completion)
{
    file_object *object = as_file(file);
    uint32_t *overlapped = static_cast<uint32_t *>(request);  // OVERLAPPED: Internal, InternalHigh, Offset, OffsetHigh, hEvent
    off_t offset;
    uint32_t total = 0;
    uint32_t error = k_error_success;

    if (object == nullptr) {
        t_last_error = k_error_invalid_handle;
        return 0;
    }
    offset = static_cast<off_t>((static_cast<uint64_t>(overlapped[3]) << 32) | overlapped[2]);
    // the read happens now; its completion waits for the thread's next alertable wait, as with ReadFileEx
    while (total < size) {
        ssize_t count = pread(object->fd, static_cast<char *>(buffer) + total, size - total, offset + static_cast<off_t>(total));

        if (count < 0) {
            if (errno == EINTR) continue;
            set_last_error_from_errno();
            error = t_last_error;
            break;
        }
        if (count == 0) {
            if (total == 0) error = k_error_handle_eof;
            break;
        }
        total += static_cast<uint32_t>(count);
    }
    overlapped[0] = error;
    overlapped[1] = total;
    queue_completion(reinterpret_cast<completion_routine>(completion), error, total, request);
    return 1;
}

uint32_t last_error()
{
    return t_last_error;
}

void set_last_error(uint32_t error)
{
    t_last_error = error;
}

}  // namespace halo::platform
