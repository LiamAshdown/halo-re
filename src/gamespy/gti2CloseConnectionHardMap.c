// gti2CloseConnectionHardMap  (GameSpy SDK in halo.exe; no C existed)
// address 0x614760, size 23 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614760..0x614776: TableMapSafe callback: gti2CloseConnection(*elem, 1).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"
#include "fn_gamespy.h"

void gti2CloseConnectionHardMap(void *elem, void *client_data)
{
    (void)client_data;
    gti2CloseConnection(*(GTI2Connection **)elem, 1);
}
