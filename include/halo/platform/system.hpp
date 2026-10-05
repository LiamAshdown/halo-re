/**
 * @file include/halo/platform/system.hpp
 * Process, dialogs, dynamic libraries, paths, clock, OS version, clipboard and text conversion, independent of the
 * operating system. The calls mirror the Win32 functions the engine used (same flag values, results and buffers);
 * src/platform/system_win32.cpp implements them with exactly those functions. Windows-only features stay Win32: the
 * hardware probe (src/shell/hardware.cpp) and the crash reporter.
 */
#pragma once

#include <cstdint>

struct system_time;          // types/cache.h: SYSTEMTIME layout
struct os_version_info_a;    // types/shell.h: OSVERSIONINFOA layout, size field set by the caller

namespace halo::platform {

[[noreturn]] void process_exit(uint32_t exit_code);

/** Shows a modal message box over window (may be null); flags and the result are the Win32 MB_ / ID values. */
int32_t message_box(void *window, const char *text, const char *caption, uint32_t flags);

/** Sets how the system reports critical errors (SetErrorMode); returns the previous mode. */
uint32_t set_error_mode(uint32_t mode);

/** Copies the one-line text of a system error code into buffer (FormatMessageA from the system table); returns its length. */
uint32_t error_message(uint32_t code, char *buffer, uint32_t size);

/** Loads a dynamic library; null when it is not available. */
void *library_open(const char *file_name);
void *library_symbol(void *library, const char *name);
void library_close(void *library);

/**
 * SHGetFolderPathA for the folders the engine asks for (CSIDL_PERSONAL: the documents folder) where shfolder.dll does
 * not exist; Windows builds keep calling SHGetFolderPathA. Writes the path (backslash separated) to out, 0 on success.
 */
int32_t __stdcall folder_path(void *owner, int32_t csidl, void *token, uint32_t flags, char *out);

/** The path of the running executable (GetModuleFileNameA); returns its length. */
uint32_t executable_path(char *buffer, uint32_t size);
/** The directory for temporary files, ending in a separator (GetTempPathA); returns its length. */
uint32_t temp_directory(char *buffer, uint32_t size);

/** The current local date and time (GetLocalTime). */
void local_time(system_time *time);

/** time (or now when null) as user-locale date / time text (GetDateFormatA / GetTimeFormatA; flags are the DATE_ / TIME_
 * values); returns the length written, 0 on failure. */
uint32_t date_text(const system_time *time, uint32_t flags, char *text, uint32_t size);
uint32_t time_text(const system_time *time, uint32_t flags, char *text, uint32_t size);

/** The fixed version of an executable or driver file (VS_FIXEDFILEINFO dwFileVersionMS / LS); false when it has none. */
bool file_version(const char *path, uint32_t *version_high, uint32_t *version_low);

/** The ANSI code page number (GetACP). */
uint32_t ansi_code_page();

/** The longest gap between the clicks of a double click, in milliseconds (GetDoubleClickTime). */
uint32_t double_click_time();

/** The file mapped at address in this process, for error messages (GetMappedFileNameA); false when there is none. */
bool mapped_file_name(void *address, char *buffer, uint32_t size);

/** Fills the caller's OSVERSIONINFOA-layout record (GetVersionExA). */
bool os_version(os_version_info_a *info);

/** Copies the clipboard's text into buffer (at most capacity bytes, as strncpy); false when there is none. */
bool clipboard_text(void *window, char *buffer, uint32_t capacity);

/** A handle for the system's cryptographic services, or null (CryptAcquireContextA, verify-only context). */
void *crypto_context_open();
void crypto_context_close(void *context);
/** The 20-byte SHA-1 digest of size bytes, computed with context (CryptoAPI on Windows); false on failure. */
bool sha1(void *context, const uint8_t *data, uint32_t size, uint8_t *digest);

/** Converts between the ANSI code page and UTF-16 (MultiByteToWideChar / WideCharToMultiByte); counts as there. */
int32_t ansi_to_wide(const char *text, int32_t length, uint16_t *out, int32_t capacity);
int32_t wide_to_ansi(const uint16_t *text, int32_t length, char *out, int32_t capacity);

}  // namespace halo::platform
