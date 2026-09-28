// gti2SendFilterCallback  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d960, size 138 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d960..0x61d9e9: filter n of the connection  sendFilters (none: 1) with
//   (connection, n, message, len, reliable; no message unless both set) under callback levels; 0 when the socket gets
//   freed afterwards.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendFilterCallback(GTI2Connection *connection, int filterID, const unsigned char *message, int len, int reliable)
{
    gt2FilterCallback *filter;
    GTI2Socket *socket;

    if (connection == 0) {
        return 1;
    }
    filter = (gt2FilterCallback *)ArrayNth(connection->sendFilters, filterID);
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
