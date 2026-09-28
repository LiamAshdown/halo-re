// gti2SendLastOutgoingMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618980, size 64 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618980..0x6189bf: ESI connection: sends the newest outgoing message from the
//   buffer (dump args 1, 0); the pending ack rode along.
// blam-cc: ESI -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2SendLastOutgoingMessage(GTI2Connection *connection)
{
    GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages,
        ArrayLength(connection->outgoingBufferMessages) - 1);

    if (!gti2ConnectionSendData(connection, connection->outgoingBuffer.buffer + message->start, message->len, 1, 0)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}
