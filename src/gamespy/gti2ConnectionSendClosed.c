// gti2ConnectionSendClosed  (GameSpy SDK in halo.exe; no C existed)
// address 0x618ec0, size 27 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618ec0..0x618eda: gti2SendClosed to the connection  address.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ConnectionSendClosed(GTI2Connection *connection)
{
    return gti2SendClosed(connection->socket, connection->ip, connection->port);
}
