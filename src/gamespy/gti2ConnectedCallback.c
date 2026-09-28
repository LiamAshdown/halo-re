// gti2ConnectedCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d7a0, size 132 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d7a0..0x61d823: records the connect result, then the connected callback
//   (connection, result, message, len; no message unless both set) under the connection and socket callback levels; 0
//   when the socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ConnectedCallback(GTI2Connection *connection, int result, const unsigned char *message, int len)
{
    GTI2Socket *socket;

    if (connection == 0) {
        return 1;
    }
    connection->connectionResult = result;
    if (connection->callbacks.connected == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    connection->callbacks.connected(connection, result, message, len);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    socket = connection->socket;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
