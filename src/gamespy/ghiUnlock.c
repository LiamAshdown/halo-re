// ghiUnlock  (GameSpy SDK in halo.exe; no C existed)
// address 0x621d50, size 17 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621d50..0x621d60: leaves the critical section when there is one.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern CRITICAL_SECTION *ghiLockHandle; // 0x006a328c

void ghiUnlock(void)
{
    if (ghiLockHandle != 0) {
        LeaveCriticalSection(ghiLockHandle);
    }
}
