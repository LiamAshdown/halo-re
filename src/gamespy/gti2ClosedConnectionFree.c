// gti2ClosedConnectionFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c350, size 14 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c350..0x61c35d: the closed-connections array's element free:
//   gti2FreeConnection of the element.
// blam-cc: cdecl

#include "gamespy.h"

extern void gti2FreeConnection(void *connection);

void gti2ClosedConnectionFree(void *elem)
{
    gti2FreeConnection(*(void **)elem);
}
