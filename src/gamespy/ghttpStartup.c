// ghttpStartup  (GameSpy SDK in halo.exe; no C existed)
// address 0x61bd00, size 52 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61bd00..0x61bd33: counts a startup (under the lock, which does not exist yet
//   the first time); the first creates the lock and sets the throttle to 125 bytes / 250 ms -- and leaves without
//   unlocking; later ones unlock.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern int ghiThrottleBufferSize;         // 0x00683dd4
extern unsigned long ghiThrottleTimeDelay; // 0x00683dd8

extern int ghiReferenceCount;             // 0x006a2e6c

void ghttpStartup(void)
{
    ghiLock();
    ghiReferenceCount++;
    if (ghiReferenceCount == 1) {
        ghiCreateLock();
        ghiThrottleBufferSize = 0x7d;
        ghiThrottleTimeDelay = 0xfa;
        return;
    }
    ghiUnlock();
}
