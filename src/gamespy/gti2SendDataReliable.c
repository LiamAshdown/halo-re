// gti2SendDataReliable  (GameSpy SDK in halo.exe; no C existed)
// address 0x619200, size 89 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619200..0x619258: a reliable application message (type 0, 7 header bytes)
//   through the outgoing buffer.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendDataReliable(GTI2Connection *connection, const unsigned char *message, int len)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, len + 7, GTI2MsgAppReliable, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    gti2BufferWriteData(&connection->outgoingBuffer, message, len);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
