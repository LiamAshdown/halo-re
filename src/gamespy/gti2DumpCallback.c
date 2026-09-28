// gti2DumpCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61da80, size 153 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61da80..0x61db18: (socket, connection, ip, port, reset, message, len, send,
//   arg9, arg10): the send dump callback (+0x24) for a send, else the receive one (+0x28); called with (socket,
//   connection, ip, port, reset, message, len, arg9, arg10) -- no message unless both set -- under the socket and
//   (when there is one) connection callback levels; 0 when the socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2DumpCallback(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port, int reset,
    const unsigned char *message, int len, int send, int arg9, int arg10)
{
    gt2DumpCallback callback;

    if (socket == 0) {
        return 1;
    }
    callback = send != 0 ? socket->sendDumpCallback : socket->receiveDumpCallback;
    if (callback == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    socket->callbackLevel++;
    if (connection != 0) {
        connection->callbackLevel++;
    }
    callback(socket, connection, ip, port, reset, message, len, arg9, arg10);
    socket->callbackLevel--;
    if (connection != 0) {
        connection->callbackLevel--;
    }
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
