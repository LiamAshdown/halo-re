/* win32.h -- the Windows API, from the Windows SDK headers.

   Every source file that calls Windows includes this first, instead of declaring the functions it uses itself: the
   prototypes (return type, parameters, __stdcall) then come from the SDK and cannot drift. The standalone link binds
   the calls to the SDK import libraries (kernel32, user32, gdi32, advapi32, ws2_32, winmm, version, ole32, oleaut32,
   shell32). WIN32_LEAN_AND_MEAN keeps the rarely used parts out; the few this code does use are included explicitly;
   NOMINMAX keeps the min / max macros out. winsock2.h has to come before windows.h. */
#ifndef HALO_WIN32_H
#define HALO_WIN32_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <objbase.h>
#include <oleauto.h>

#endif
