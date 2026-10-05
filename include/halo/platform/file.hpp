/**
 * @file include/halo/platform/file.hpp
 * Files and directories, independent of the operating system. The calls mirror the Win32 functions the engine used
 * (same arguments, results and edge cases), with the Windows-only arguments dropped. Open modes, share modes,
 * dispositions, flags, attributes and seek methods keep the Win32 values the engine already uses
 * (halo::win32::k_generic_read, k_open_existing, ...): every platform interprets that encoding. src/platform/file_win32.cpp
 * implements them with exactly those Win32 functions.
 */
#pragma once

#include <cstdint>

struct file_time;         // types/cache.h: FILETIME layout
struct win32_find_dataa;  // types/game.h: WIN32_FIND_DATAA layout

namespace halo::platform {

/** An open file; on Windows the HANDLE itself, so engine records that store one keep their size. */
using file_handle = void *;

/** The handle file_open and find_first return on failure (INVALID_HANDLE_VALUE). */
inline file_handle invalid_handle()
{
    return reinterpret_cast<file_handle>(static_cast<intptr_t>(-1));
}

/** Opens or creates a file (CreateFileA). */
file_handle file_open(const char *path, uint32_t access, uint32_t share, uint32_t disposition, uint32_t flags);
bool file_read(file_handle file, void *buffer, uint32_t size, uint32_t *bytes_read);
bool file_write(file_handle file, const void *buffer, uint32_t size, uint32_t *bytes_written);
/** Moves the file pointer (SetFilePointer): returns the new low 32 bits, or 0xffffffff on failure. */
uint32_t file_seek(file_handle file, int32_t distance, int32_t *distance_high, uint32_t method);
/** The file's size (GetFileSize): low 32 bits, the high ones in *size_high when it is not null. */
uint32_t file_size(file_handle file, uint32_t *size_high);
/** Ends the file at the file pointer (SetEndOfFile). */
bool file_truncate(file_handle file);
bool file_close(file_handle file);

/** A file's creation time, which the map cache uses as a slot's age (GetFileTime / SetFileTime, first argument). */
bool file_get_creation_time(file_handle file, file_time *time);
bool file_set_creation_time(file_handle file, const file_time *time);
/** -1, 0 or 1 as a is earlier than, equal to or later than b (CompareFileTime). */
int32_t file_time_compare(const file_time *a, const file_time *b);
/** The current UTC time as a file time (GetSystemTime + SystemTimeToFileTime). */
void file_time_now(file_time *time);

bool file_delete(const char *path);
bool file_copy(const char *source, const char *destination, bool fail_if_exists);
/** The file's attributes, 0xffffffff when it does not exist (GetFileAttributesA). */
uint32_t file_attributes(const char *path);
bool file_set_attributes(const char *path, uint32_t attributes);
/** The size of the file at path (low 32 bits, GetFileAttributesExA); false (last_error set) when it cannot be read. */
bool file_size_by_path(const char *path, uint32_t *size);
bool directory_create(const char *path);
bool directory_remove(const char *path);
/** Copies the current directory into buffer; returns its length (GetCurrentDirectoryA). */
uint32_t current_directory(uint32_t size, char *buffer);
/** Free and total bytes of the volume holding path; any output may be null (GetDiskFreeSpaceExA). */
bool disk_free_space(const char *path, uint64_t *available_to_caller, uint64_t *total, uint64_t *free);

/** Starts listing the files matching pattern (FindFirstFileA); invalid_handle() when none match. */
file_handle find_first(const char *pattern, win32_find_dataa *data);
bool find_next(file_handle find, win32_find_dataa *data);
bool find_close(file_handle find);

/**
 * Starts reading size bytes at request's offset into buffer (ReadFileEx). request is an OVERLAPPED-layout record the
 * caller keeps alive; completion(error, bytes, request) runs on this thread during its next alertable wait
 * (thread.hpp: wait_alertable, sleep_alertable). Returns nonzero when the read was started. The arguments are untyped
 * so the map streamer can keep its own request record and completion routine.
 */
int32_t file_read_async(void *file, void *buffer, uint32_t size, void *request, void *completion);

/** The calling thread's last error code (GetLastError), and setting it (SetLastError). */
uint32_t last_error();
void set_last_error(uint32_t error);

}  // namespace halo::platform
