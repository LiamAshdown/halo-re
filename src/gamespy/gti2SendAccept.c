// gti2SendAccept  (GameSpy SDK in halo.exe; no C existed)
// address 0x6193b0, size 90 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6193b0..0x619409: type 4 with our 16-byte public key.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendAccept(GTI2Connection *connection)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, 0x17, GTI2MsgAccept, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    gti2BufferWriteData(&connection->outgoingBuffer, connection->publicKey, 0x10);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
