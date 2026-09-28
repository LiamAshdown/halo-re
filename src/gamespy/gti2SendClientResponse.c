// gti2SendClientResponse  (GameSpy SDK in halo.exe; no C existed)
// address 0x619330, size 119 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619330..0x6193a6: type 3 with the 32-byte response, our 16-byte public key and
//   the initial message.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendClientResponse(GTI2Connection *connection, const unsigned char *response, const unsigned char *message,
    int len)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, len + 0x37, GTI2MsgClientResponse, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    gti2BufferWriteData(&connection->outgoingBuffer, response, 0x20);
    gti2BufferWriteData(&connection->outgoingBuffer, connection->publicKey, 0x10);
    gti2BufferWriteData(&connection->outgoingBuffer, message, len);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
