// os_platform_identify  (Ghidra: os_platform_identify, already named)
// address 0x5427e0, size 89 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: GetVersionExA dwPlatformId mapped exactly to the os_platform enum in shell.h
// (VER_PLATFORM_WIN32_WINDOWS=1 -> windows_9x, VER_PLATFORM_WIN32_NT=2 -> windows_nt, else other).
// register convention: __cdecl, no arguments, no return value.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t os_platform_value; // 0x00721ef0, UNSURE: named os_platform_value not os_platform -- shell.h's os_platform enum typedef occupies that identifier in the same C namespace, so the documented global name does not compile as written

// Queries GetVersionExA and caches a small platform-family code (Win9x vs WinNT vs other) into
// os_platform for later feature gating.
void os_platform_identify(void)
{
    os_version_info_a info;

    info.size = 0x94;
    if (GetVersionExA((LPOSVERSIONINFOA)&info) != 0) {
        if (info.platform_id != 1) {
            if (info.platform_id != 2) {
                os_platform_value = k_os_platform_other;
                return;
            }
            os_platform_value = k_os_platform_windows_nt;
            return;
        }
        os_platform_value = k_os_platform_windows_9x;
    }
}

#if 0
Original Ghidra decompilation (0x5427e0):


void __cdecl os_platform_identify(void)

{
  BOOL BVar1;
  _OSVERSIONINFOA local_94;
  
  local_94.dwOSVersionInfoSize = 0x94;
  BVar1 = GetVersionExA(&local_94);
  if (BVar1 != 0) {
    if (local_94.dwPlatformId != 1) {
      if (local_94.dwPlatformId != 2) {
        DAT_00721ef0 = 1;
        return;
      }
      DAT_00721ef0 = 3;
      return;
    }
    DAT_00721ef0 = 2;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
