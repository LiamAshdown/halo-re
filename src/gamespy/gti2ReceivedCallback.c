// gti2ReceivedCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d830, size 109 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d830..0x61d89c: the received callback (connection, message, len, reliable)
//   under callback levels; 0 when the socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ReceivedCallback(GTI2Connection *connection, const unsigned char *message, int len, int reliable)
{
    GTI2Socket *socket;

    if (connection == 0 || connection->callbacks.received == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.received(connection, message, len, reliable);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    socket = connection->socket;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
