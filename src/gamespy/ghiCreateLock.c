// ghiCreateLock  (GameSpy SDK in halo.exe; no C existed)
// address 0x621cd0, size 39 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621cd0..0x621cf6: a malloc  critical section (NULL when out of memory).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern CRITICAL_SECTION *ghiLockHandle; // 0x006a328c

void ghiCreateLock(void)
{
    CRITICAL_SECTION *lock = (CRITICAL_SECTION *)malloc(0x18);

    if (lock != 0) {
        InitializeCriticalSection(lock);
    }
    ghiLockHandle = lock;
}
