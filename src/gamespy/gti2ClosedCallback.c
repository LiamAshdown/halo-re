// gti2ClosedCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d8a0, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d8a0..0x61d8fa: the closed callback (connection, reason) under callback
//   levels; 0 when the socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ClosedCallback(GTI2Connection *connection, int reason)
{
    GTI2Socket *socket;

    if (connection == 0 || connection->callbacks.closed == 0) {
        return 1;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.closed(connection, reason);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    socket = connection->socket;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
