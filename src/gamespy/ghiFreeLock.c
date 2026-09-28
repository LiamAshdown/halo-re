// ghiFreeLock  (GameSpy SDK in halo.exe; no C existed)
// address 0x621d00, size 40 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621d00..0x621d27: deletes and frees the critical section.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern CRITICAL_SECTION *ghiLockHandle; // 0x006a328c

void ghiFreeLock(void)
{
    CRITICAL_SECTION *lock = ghiLockHandle;

    if (lock == 0) {
        return;
    }
    DeleteCriticalSection(lock);
    free(lock);
    ghiLockHandle = 0;
}
