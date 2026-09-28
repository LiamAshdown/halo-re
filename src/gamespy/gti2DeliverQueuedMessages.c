// gti2DeliverQueuedMessages  (GameSpy SDK in halo.exe; no C existed)
// address 0x619840, size 106 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619840..0x6198a9: EAX connection: while a buffered out-of-order message
//   (searched last first) carries the serial now expected, it is delivered and removed; 0 when a delivery failed.
// blam-cc: EAX -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2DeliverQueuedMessages(GTI2Connection *connection)
{
    int i;

restart:
    for (i = ArrayLength(connection->incomingBufferMessages) - 1; i >= 0; i--) {
        GTI2IncomingBufferMessage *message =
            (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);

        if (message->serialNumber == connection->expectedSerialNumber) {
            if (!gti2DeliverReliableMessage(connection, message->type,
                    connection->incomingBuffer.buffer + message->start, message->len)) {
                return 0;
            }
            gti2RemoveIncomingBufferMessage(connection, i, message);
            goto restart;
        }
    }
    return 1;
}
