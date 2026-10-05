/* Forced include (-include) for builds of src/ with clang off Windows (the Emscripten build, cmake/web.cmake); the MSVC
   build's counterpart is msvc_compat.h. Not part of the reconstruction.

   - The calling-convention keywords mean nothing on WebAssembly: they compile away, so declarations that name them
     (function pointer types of the original ABI) keep building.
   - The MSVC C library names the engine calls map to their POSIX equivalents.
   - wchar_t is a 16-bit UTF-16 unit as on Windows (-fshort-wchar); the wide string functions then come from
     halo/platform/wchar16.h, since the C library's assume 32-bit units. */
#ifndef HALO_CLANG_COMPAT_H
#define HALO_CLANG_COMPAT_H

#define __stdcall
#define __cdecl
#define __fastcall
#define __thiscall

#include <stdio.h>
#include <string.h>
#include <strings.h>

#define _stricmp strcasecmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _strdup strdup

#include <stdint.h>
#include <time.h>
typedef int32_t __time32_t;  // the original's 32-bit time_t
static inline __time32_t _time32(__time32_t *out)
{
    __time32_t now = (__time32_t)time(0);

    if (out != 0) *out = now;
    return now;
}
static inline struct tm *_localtime32(const __time32_t *value)
{
    time_t wide = *value;

    return localtime(&wide);
}

#define HALO_WCHAR16_COMPAT 1
#include "halo/platform/wchar16.h"

#endif
