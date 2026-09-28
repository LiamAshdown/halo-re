// gti2UnrecognizedMessageCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61db20, size 118 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61db20..0x61db95: clears *handled; the unrecognized-message callback (socket,
//   ip, port, message, len; no message unless both set) under the socket callback level decides *handled; 0 when the
//   socket gets freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2UnrecognizedMessageCallback(GTI2Socket *socket, unsigned int ip, unsigned short port,
    const unsigned char *message, int len, int *handled)
{
    *handled = 0;
    if (socket == 0 || socket->unrecognizedMessageCallback == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    socket->callbackLevel++;
    *handled = socket->unrecognizedMessageCallback(socket, ip, port, message, len);
    socket->callbackLevel--;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
