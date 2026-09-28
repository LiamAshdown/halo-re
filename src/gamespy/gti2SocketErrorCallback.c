// gti2SocketErrorCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d6c0, size 69 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d6c0..0x61d704: calls the socket error callback (when set) under a callback
//   level; 0 when the socket was marked to close and is freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SocketErrorCallback(GTI2Socket *socket)
{
    if (socket == 0 || socket->socketErrorCallback == 0) {
        return 1;
    }
    socket->callbackLevel++;
    socket->socketErrorCallback(socket);
    socket->callbackLevel--;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
