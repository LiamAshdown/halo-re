// gti2PingCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d900, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d900..0x61d95a: the ping callback (connection, latency) under callback
//   levels; 0 when the socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2PingCallback(GTI2Connection *connection, int latency)
{
    GTI2Socket *socket;

    if (connection == 0 || connection->callbacks.ping == 0) {
        return 1;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.ping(connection, latency);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    socket = connection->socket;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
