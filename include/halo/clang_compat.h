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
#define _isnan isnan

#define HALO_WCHAR16_COMPAT 1
#include "halo/platform/wchar16.h"

#endif
