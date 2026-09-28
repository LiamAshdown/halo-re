// gti2ConnectAttemptCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d710, size 130 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d710..0x61d791: the socket  connect-attempt callback (socket, connection, ip,
//   port, latency, message, len; no message unless both are set) under socket and connection callback levels; 0 when
//   the socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ConnectAttemptCallback(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port,
    int latency, const unsigned char *message, int len)
{
    if (socket == 0 || connection == 0 || socket->connectAttemptCallback == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    socket->callbackLevel++;
    connection->callbackLevel++;
    socket->connectAttemptCallback(socket, connection, ip, port, latency, message, len);
    socket->callbackLevel--;
    connection->callbackLevel--;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
