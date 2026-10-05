/**
 * @file src/platform/system_posix.cpp
 * halo::platform system services on POSIX (Emscripten, Linux). Message boxes print and take the default answer,
 * dynamic libraries exist only off the browser, the OS reports itself as Windows XP (what the engine's version checks
 * expect), dates and times are formatted as en-US, the ANSI code page is Windows-1252, and SHA-1 is computed here.
 */

#include "halo/platform/system.hpp"
#include "halo/platform/file.hpp"
#include "posix_internal.hpp"
#include "system_portable.hpp"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if !defined(__EMSCRIPTEN__)
#include <dlfcn.h>
#endif

struct system_time {
    uint16_t year;
    uint16_t month;
    uint16_t day_of_week;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t milliseconds;
};

struct os_version_info_a {
    uint32_t size;
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t build_number;
    uint32_t platform_id;
    char service_pack[0x80];
};

namespace halo::platform {

namespace {

/** Copies text into buffer as the engine expects Windows paths (backslashes); returns its length. */
uint32_t engine_path(const char *text, char *buffer, uint32_t size)
{
    uint32_t length = static_cast<uint32_t>(strlen(text));

    if (size == 0) {
        return 0;
    }
    if (length >= size) {
        length = size - 1;
    }
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = text[i] == '/' ? '\\' : text[i];
    }
    buffer[length] = '\0';
    return length;
}

void now(system_time *out)
{
    struct timespec time_now;
    struct tm local;

    clock_gettime(CLOCK_REALTIME, &time_now);
    localtime_r(&time_now.tv_sec, &local);
    out->year = static_cast<uint16_t>(local.tm_year + 1900);
    out->month = static_cast<uint16_t>(local.tm_mon + 1);
    out->day_of_week = static_cast<uint16_t>(local.tm_wday);
    out->day = static_cast<uint16_t>(local.tm_mday);
    out->hour = static_cast<uint16_t>(local.tm_hour);
    out->minute = static_cast<uint16_t>(local.tm_min);
    out->second = static_cast<uint16_t>(local.tm_sec);
    out->milliseconds = static_cast<uint16_t>(time_now.tv_nsec / 1000000);
}

/** snprintf's result as GetDateFormatA / GetTimeFormatA count it: characters including the terminator, 0 if it did not fit. */
uint32_t format_result(int written, uint32_t size)
{
    return written >= 0 && static_cast<uint32_t>(written) < size ? static_cast<uint32_t>(written) + 1 : 0;
}

}  // namespace

void process_exit(uint32_t exit_code)
{
    exit(static_cast<int>(exit_code));
}

int32_t message_box(void *window, const char *text, const char *caption, uint32_t flags)
{
    (void)window;
    fprintf(stderr, "[%s] %s\n", caption != nullptr ? caption : "", text != nullptr ? text : "");
    switch (flags & 0xf) {
    case 2: return 3;           // MB_ABORTRETRYIGNORE: IDABORT
    case 3: case 4: return 6;   // MB_YESNOCANCEL, MB_YESNO: IDYES
    case 5: return 2;           // MB_RETRYCANCEL: IDCANCEL
    default: return 1;          // MB_OK, MB_OKCANCEL: IDOK
    }
}

uint32_t set_error_mode(uint32_t mode)
{
    (void)mode;
    return 0;
}

uint32_t error_message(uint32_t code, char *buffer, uint32_t size)
{
    const char *text;
    int written;

    switch (code) {
    case posix::k_error_file_not_found: text = "The system cannot find the file specified."; break;
    case posix::k_error_path_not_found: text = "The system cannot find the path specified."; break;
    case posix::k_error_access_denied: text = "Access is denied."; break;
    case posix::k_error_not_enough_memory: text = "Not enough storage is available to process this command."; break;
    case posix::k_error_disk_full: text = "There is not enough space on the disk."; break;
    case posix::k_error_already_exists: text = "Cannot create a file when that file already exists."; break;
    default: text = nullptr; break;
    }
    written = text != nullptr ? snprintf(buffer, size, "%s", text) : snprintf(buffer, size, "Error %u.", code);
    return written > 0 && static_cast<uint32_t>(written) < size ? static_cast<uint32_t>(written) : 0;
}

void *library_open(const char *file_name)
{
#if defined(__EMSCRIPTEN__)
    (void)file_name;
    return nullptr;  // no dynamic libraries in the browser; callers fall back as when a DLL is missing
#else
    return dlopen(file_name, RTLD_NOW);
#endif
}

void *library_symbol(void *library, const char *name)
{
#if defined(__EMSCRIPTEN__)
    (void)library;
    (void)name;
    return nullptr;
#else
    return library != nullptr ? dlsym(library, name) : nullptr;
#endif
}

void library_close(void *library)
{
#if !defined(__EMSCRIPTEN__)
    if (library != nullptr) {
        dlclose(library);
    }
#else
    (void)library;
#endif
}

uint32_t executable_path(char *buffer, uint32_t size)
{
#if defined(__EMSCRIPTEN__)
    return engine_path("/halo/halo.exe", buffer, size);
#else
    char path[1024];
    ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);

    path[length > 0 ? length : 0] = '\0';
    return engine_path(path, buffer, size);
#endif
}

int32_t __stdcall folder_path(void *owner, int32_t csidl, void *token, uint32_t flags, char *out)
{
    const char *home = getenv("HOME");
    char path[512];

    (void)owner;
    (void)csidl;  // the documents folder is the only one the engine asks for
    (void)token;
    (void)flags;
    snprintf(path, sizeof(path), "%s/Documents", home != nullptr ? home : "");
    engine_path(path, out, 260);
    return 0;
}

uint32_t temp_directory(char *buffer, uint32_t size)
{
    return engine_path("/tmp/", buffer, size);
}

void local_time(system_time *time)
{
    now(time);
}

uint32_t date_text(const system_time *time, uint32_t flags, char *text, uint32_t size)
{
    static const char *const days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char *const months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September",
        "October", "November", "December"};
    system_time current;

    if (time == nullptr) {
        now(&current);
        time = &current;
    }
    if ((flags & 2) != 0 && time->month >= 1 && time->month <= 12) {  // DATE_LONGDATE
        return format_result(snprintf(text, size, "%s, %s %u, %u", days[time->day_of_week % 7], months[time->month - 1], time->day, time->year), size);
    }
    return format_result(snprintf(text, size, "%u/%u/%u", time->month, time->day, time->year), size);  // en-US short date
}

uint32_t time_text(const system_time *time, uint32_t flags, char *text, uint32_t size)
{
    system_time current;
    bool no_seconds = (flags & 2) != 0;
    bool twenty_four = (flags & 8) != 0;
    unsigned hour;

    if (time == nullptr) {
        now(&current);
        time = &current;
    }
    if (twenty_four) {
        return no_seconds ? format_result(snprintf(text, size, "%02u:%02u", time->hour, time->minute), size)
                          : format_result(snprintf(text, size, "%02u:%02u:%02u", time->hour, time->minute, time->second), size);
    }
    hour = time->hour % 12 == 0 ? 12u : time->hour % 12u;
    return no_seconds ? format_result(snprintf(text, size, "%u:%02u %s", hour, time->minute, time->hour < 12 ? "AM" : "PM"), size)
                      : format_result(snprintf(text, size, "%u:%02u:%02u %s", hour, time->minute, time->second, time->hour < 12 ? "AM" : "PM"), size);
}

bool file_version(const char *path, uint32_t *version_high, uint32_t *version_low)
{
    (void)path;
    (void)version_high;
    (void)version_low;
    return false;  // no version resources off Windows
}

uint32_t ansi_code_page()
{
    return 1252;
}

uint32_t double_click_time()
{
    return 500;  // the Windows default
}

bool mapped_file_name(void *address, char *buffer, uint32_t size)
{
    (void)address;
    (void)buffer;
    (void)size;
    return false;
}

bool os_version(os_version_info_a *info)
{
    info->major_version = 5;  // Windows XP (NT 5.1, build 2600)
    info->minor_version = 1;
    info->build_number = 2600;
    info->platform_id = 2;
    info->service_pack[0] = '\0';
    return true;
}

bool clipboard_text(void *window, char *buffer, uint32_t capacity)
{
    char *text;

    (void)window;
    if (capacity == 0 || !SDL_HasClipboardText()) {
        return false;
    }
    text = SDL_GetClipboardText();
    if (text == nullptr) {
        return false;
    }
    strncpy(buffer, text, capacity);  // as strncpy: not terminated when the text fills the buffer
    SDL_free(text);
    return true;
}

void *crypto_context_open()
{
    static int context;

    return &context;  // SHA-1 needs no provider here
}

void crypto_context_close(void *context)
{
    (void)context;
}

bool sha1(void *context, const uint8_t *data, uint32_t size, uint8_t *digest)
{
    (void)context;
    portable::sha1(data, size, digest);
    return true;
}

int32_t ansi_to_wide(const char *text, int32_t length, uint16_t *out, int32_t capacity)
{
    return portable::ansi_to_wide(text, length, out, capacity);
}

int32_t wide_to_ansi(const uint16_t *text, int32_t length, char *out, int32_t capacity)
{
    return portable::wide_to_ansi(text, length, out, capacity);
}

}  // namespace halo::platform
