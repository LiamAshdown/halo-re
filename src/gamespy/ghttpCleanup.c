// ghttpCleanup  (GameSpy SDK in halo.exe; no C existed)
// address 0x61bd40, size 61 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61bd40..0x61bd7c: the last cleanup frees every connection and the proxy string,
//   unlocks and frees the lock; earlier ones just unlock.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern char *ghiProxyAddress;             // 0x007231e0
extern unsigned short ghiProxyPort;       // 0x007231dc

extern int ghiReferenceCount;             // 0x006a2e6c

void ghttpCleanup(void)
{
    ghiLock();
    ghiReferenceCount--;
    if (ghiReferenceCount != 0) {
        ghiUnlock();
        return;
    }
    ghiCleanupConnections();
    if (ghiProxyAddress != 0) {
        free(ghiProxyAddress);
        ghiProxyAddress = 0;
    }
    ghiUnlock();
    ghiFreeLock();
}
