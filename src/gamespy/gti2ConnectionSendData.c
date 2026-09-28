// gti2ConnectionSendData  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cf20, size 71 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cf20..0x61cf66: gti2SocketSend to the connection  address; after a send the
//   last-send time is now. 0 when the send failed.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ConnectionSendData(GTI2Connection *connection, const unsigned char *message, int len, int arg4, int arg5)
{
    if (!gti2SocketSend(connection->socket, connection->ip, connection->port, message, len, arg4, arg5)) {
        return 0;
    }
    connection->lastSend = current_time();
    return 1;
}
