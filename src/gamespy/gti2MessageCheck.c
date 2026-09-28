// gti2MessageCheck  (GameSpy SDK in halo.exe; no C existed)
// address 0x614a70, size 54 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614a70..0x614aa5: normalizes a (message, length) pair in place: a NULL message
//   becomes "" (0x0065512c) with length 0, and length -1 becomes strlen + 1.
// blam-cc: cdecl

#include "gamespy.h"

void gti2MessageCheck(const char **message, int *length)
{
    if (*message == 0) {
        *message = "";
        *length = 0;
        return;
    }
    if (*length == -1) {
        *length = (int)strlen(*message) + 1;
    }
}
