/**
 * @file src/platform/system_win32.cpp
 * halo::platform system services on Windows: the same Win32 calls the engine made directly.
 */

#include "halo/platform/system.hpp"

#include "win32.h"
#include "cache.h"

#include <string.h>

namespace halo::platform {

void process_exit(uint32_t exit_code)
{
    ExitProcess(exit_code);
}

int32_t message_box(void *window, const char *text, const char *caption, uint32_t flags)
{
    return MessageBoxA(static_cast<HWND>(window), text, caption, flags);
}

uint32_t set_error_mode(uint32_t mode)
{
    return SetErrorMode(mode);
}

uint32_t error_message(uint32_t code, char *buffer, uint32_t size)
{
    return FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr, code, 0, buffer, size, nullptr);
}

void *library_open(const char *file_name)
{
    return LoadLibraryA(file_name);
}

void *library_symbol(void *library, const char *name)
{
    return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(library), name));
}

void library_close(void *library)
{
    FreeLibrary(static_cast<HMODULE>(library));
}

uint32_t executable_path(char *buffer, uint32_t size)
{
    return GetModuleFileNameA(nullptr, buffer, size);
}

uint32_t temp_directory(char *buffer, uint32_t size)
{
    return GetTempPathA(size, buffer);
}

void local_time(system_time *time)
{
    static_assert(sizeof(system_time) == sizeof(SYSTEMTIME));
    GetLocalTime(reinterpret_cast<SYSTEMTIME *>(time));
}

bool os_version(os_version_info_a *info)
{
    return GetVersionExA(reinterpret_cast<OSVERSIONINFOA *>(info)) != 0;
}

bool clipboard_text(void *window, char *buffer, uint32_t capacity)
{
    HANDLE clipboard_handle;
    char *locked_text;

    if (IsClipboardFormatAvailable(CF_TEXT) != 0) {
        OpenClipboard(static_cast<HWND>(window));
    }
    clipboard_handle = GetClipboardData(CF_TEXT);
    if (clipboard_handle == nullptr) {
        GetLastError();
    } else {
        locked_text = static_cast<char *>(GlobalLock(clipboard_handle));
        if (locked_text != nullptr) {
            strncpy(buffer, locked_text, capacity);
            GlobalUnlock(clipboard_handle);
            CloseClipboard();
            return true;
        }
    }
    CloseClipboard();
    return false;
}

void *crypto_context_open()
{
    HCRYPTPROV provider;

    if (CryptAcquireContextA(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) == 0) {
        return nullptr;
    }
    return reinterpret_cast<void *>(provider);
}

void crypto_context_close(void *context)
{
    CryptReleaseContext(reinterpret_cast<HCRYPTPROV>(context), 0);
}

bool sha1(void *context, const uint8_t *data, uint32_t size, uint8_t *digest)
{
    HCRYPTHASH hash = 0;
    DWORD digest_length = 20;
    bool ok = false;

    if (CryptCreateHash(reinterpret_cast<HCRYPTPROV>(context), CALG_SHA1, 0, 0, &hash) != 0) {
        if (CryptHashData(hash, data, size, 0) != 0) {
            ok = CryptGetHashParam(hash, HP_HASHVAL, digest, &digest_length, 0) != 0;
        }
    }
    if (hash != 0) {
        CryptDestroyHash(hash);
    }
    return ok;
}

int32_t ansi_to_wide(const char *text, int32_t length, uint16_t *out, int32_t capacity)
{
    return MultiByteToWideChar(CP_ACP, 0, text, length, reinterpret_cast<wchar_t *>(out), capacity);
}

int32_t wide_to_ansi(const uint16_t *text, int32_t length, char *out, int32_t capacity)
{
    return WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t *>(text), length, out, capacity, nullptr, nullptr);
}

}  // namespace halo::platform
