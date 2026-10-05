/**
 * @file include/halo/platform/wchar16.h
 * Wide strings as the engine knows them: wchar_t is a 16-bit UTF-16 unit (MSVC), and swprintf follows MSVC's format
 * rules (%s and %ls take a wide string, %S and %hs a narrow one, %c a wide character). Builds where the C library's
 * wchar_t is 32-bit compile with -fshort-wchar and force-include this header with HALO_WCHAR16_COMPAT defined: the
 * C library names then map to the halo_ implementations in src/platform/wchar16.cpp, which work on 16-bit units.
 * The MSVC build uses its own C library and only compiles the halo_ functions (tools/wchar16_test.cpp checks them
 * against it).
 */
#ifndef HALO_PLATFORM_WCHAR16_H
#define HALO_PLATFORM_WCHAR16_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t halo_wcslen(const wchar_t *text);
wchar_t *halo_wcscpy(wchar_t *destination, const wchar_t *source);
wchar_t *halo_wcsncpy(wchar_t *destination, const wchar_t *source, size_t count);
wchar_t *halo_wcscat(wchar_t *destination, const wchar_t *source);
wchar_t *halo_wcsncat(wchar_t *destination, const wchar_t *source, size_t count);
int halo_wcscmp(const wchar_t *a, const wchar_t *b);
int halo_wcsncmp(const wchar_t *a, const wchar_t *b, size_t count);
int halo_wcsicmp(const wchar_t *a, const wchar_t *b);
int halo_wcsnicmp(const wchar_t *a, const wchar_t *b, size_t count);
wchar_t *halo_wcschr(const wchar_t *text, wchar_t character);
wchar_t *halo_wcsstr(const wchar_t *text, const wchar_t *pattern);
/** MSVC swprintf / vswprintf: at most count units including the terminator; -1 (and a truncated string) when it does not fit. */
int halo_swprintf(wchar_t *out, size_t count, const wchar_t *format, ...);
int halo_vswprintf(wchar_t *out, size_t count, const wchar_t *format, va_list args);
/** wprintf: the text goes to stdout as UTF-8. */
int halo_wprintf(const wchar_t *format, ...);

#ifdef __cplusplus
}
#endif

#if defined(HALO_WCHAR16_COMPAT)
#define wcslen halo_wcslen
#define wcscpy halo_wcscpy
#define wcsncpy halo_wcsncpy
#define wcscat halo_wcscat
#define wcsncat halo_wcsncat
#define wcscmp halo_wcscmp
#define wcsncmp halo_wcsncmp
#define _wcsicmp halo_wcsicmp
#define _wcsnicmp halo_wcsnicmp
#define wcschr halo_wcschr
#define wcsstr halo_wcsstr
#define swprintf halo_swprintf
#define vswprintf halo_vswprintf
#define wprintf halo_wprintf
#endif

#endif
