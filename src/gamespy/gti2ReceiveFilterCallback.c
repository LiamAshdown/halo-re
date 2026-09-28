// gti2ReceiveFilterCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d9f0, size 138 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d9f0..0x61da79: filter n of the connection  receiveFilters (none: 1) with
//   (connection, n, message, len, reliable; no message unless both set) under callback levels; 0 when the socket gets
//   freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ReceiveFilterCallback(GTI2Connection *connection, int filterID, const unsigned char *message, int len, int reliable)
{
    gt2FilterCallback *filter;
    GTI2Socket *socket;

    if (connection == 0) {
        return 1;
    }
    filter = (gt2FilterCallback *)ArrayNth(connection->receiveFilters, filterID);
    if (filter == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    (*filter)(connection, filterID, message, len, reliable);
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    socket = connection->socket;
    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
}
