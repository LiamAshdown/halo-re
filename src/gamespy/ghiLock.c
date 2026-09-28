// ghiLock  (GameSpy SDK in halo.exe; no C existed)
// address 0x621d30, size 17 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621d30..0x621d40: enters the critical section when there is one.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern CRITICAL_SECTION *ghiLockHandle; // 0x006a328c

void ghiLock(void)
{
    if (ghiLockHandle != 0) {
        EnterCriticalSection(ghiLockHandle);
    }
}
