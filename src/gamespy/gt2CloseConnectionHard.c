// gt2CloseConnectionHard  (GameSpy SDK in halo.exe; no C existed)
// address 0x614710, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614710..0x61471f: gti2CloseConnection(connection, 1).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gt2CloseConnectionHard(GTI2Connection *connection)
{
    gti2CloseConnection(connection, 1);
}
