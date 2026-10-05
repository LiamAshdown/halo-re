/**
 * @file src/platform/file_win32.cpp
 * halo::platform files on Windows: the same Win32 calls the engine made directly.
 */

#include "halo/platform/file.hpp"

#include "win32.h"
#include "cache.h"

static_assert(sizeof(file_time) == sizeof(FILETIME));

namespace halo::platform {

file_handle file_open(const char *path, uint32_t access, uint32_t share, uint32_t disposition, uint32_t flags)
{
    return CreateFileA(path, access, share, nullptr, disposition, flags, nullptr);
}

bool file_read(file_handle file, void *buffer, uint32_t size, uint32_t *bytes_read)
{
    return ReadFile(file, buffer, size, reinterpret_cast<DWORD *>(bytes_read), nullptr) != 0;
}

bool file_write(file_handle file, const void *buffer, uint32_t size, uint32_t *bytes_written)
{
    return WriteFile(file, buffer, size, reinterpret_cast<DWORD *>(bytes_written), nullptr) != 0;
}

uint32_t file_seek(file_handle file, int32_t distance, int32_t *distance_high, uint32_t method)
{
    return SetFilePointer(file, distance, reinterpret_cast<LONG *>(distance_high), method);
}

uint32_t file_size(file_handle file, uint32_t *size_high)
{
    return GetFileSize(file, reinterpret_cast<DWORD *>(size_high));
}

bool file_truncate(file_handle file)
{
    return SetEndOfFile(file) != 0;
}

bool file_close(file_handle file)
{
    return CloseHandle(file) != 0;
}

bool file_get_creation_time(file_handle file, file_time *time)
{
    return GetFileTime(file, reinterpret_cast<FILETIME *>(time), nullptr, nullptr) != 0;
}

bool file_set_creation_time(file_handle file, const file_time *time)
{
    return SetFileTime(file, reinterpret_cast<const FILETIME *>(time), nullptr, nullptr) != 0;
}

int32_t file_time_compare(const file_time *a, const file_time *b)
{
    return CompareFileTime(reinterpret_cast<const FILETIME *>(a), reinterpret_cast<const FILETIME *>(b));
}

void file_time_now(file_time *time)
{
    SYSTEMTIME now;

    GetSystemTime(&now);
    SystemTimeToFileTime(&now, reinterpret_cast<FILETIME *>(time));
}

bool file_delete(const char *path)
{
    return DeleteFileA(path) != 0;
}

bool file_copy(const char *source, const char *destination, bool fail_if_exists)
{
    return CopyFileA(source, destination, fail_if_exists ? TRUE : FALSE) != 0;
}

uint32_t file_attributes(const char *path)
{
    return GetFileAttributesA(path);
}

bool file_size_by_path(const char *path, uint32_t *size)
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        return false;
    }
    *size = data.nFileSizeLow;
    return true;
}

bool file_set_attributes(const char *path, uint32_t attributes)
{
    return SetFileAttributesA(path, attributes) != 0;
}

bool directory_create(const char *path)
{
    return CreateDirectoryA(path, nullptr) != 0;
}

bool directory_remove(const char *path)
{
    return RemoveDirectoryA(path) != 0;
}

uint32_t current_directory(uint32_t size, char *buffer)
{
    return GetCurrentDirectoryA(size, buffer);
}

bool disk_free_space(const char *path, uint64_t *available_to_caller, uint64_t *total, uint64_t *free)
{
    return GetDiskFreeSpaceExA(path, reinterpret_cast<ULARGE_INTEGER *>(available_to_caller), reinterpret_cast<ULARGE_INTEGER *>(total),
               reinterpret_cast<ULARGE_INTEGER *>(free)) != 0;
}

file_handle find_first(const char *pattern, win32_find_dataa *data)
{
    return FindFirstFileA(pattern, reinterpret_cast<WIN32_FIND_DATAA *>(data));
}

bool find_next(file_handle find, win32_find_dataa *data)
{
    return FindNextFileA(find, reinterpret_cast<WIN32_FIND_DATAA *>(data)) != 0;
}

bool find_close(file_handle find)
{
    return FindClose(find) != 0;
}

int32_t file_read_async(void *file, void *buffer, uint32_t size, void *request, void *completion)
{
    return ReadFileEx(file, buffer, size, static_cast<OVERLAPPED *>(request), reinterpret_cast<LPOVERLAPPED_COMPLETION_ROUTINE>(completion));
}

uint32_t last_error()
{
    return GetLastError();
}

void set_last_error(uint32_t error)
{
    SetLastError(error);
}

}  // namespace halo::platform
