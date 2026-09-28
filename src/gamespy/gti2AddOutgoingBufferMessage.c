// gti2AddOutgoingBufferMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618900, size 113 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618900..0x618970: ESI connection, EDX len, stack serial: appends a record
//   (start at the outgoing buffer end, len, no resends, serial, sent now); 1 when it was added.
// blam-cc: ESI -> connection, EDX -> len, stack -> serialNumber

#include "gamespy.h"

#include "gt2.h"

int gti2AddOutgoingBufferMessage(GTI2Connection *connection, int len, unsigned short serialNumber)
{
    GTI2OutgoingBufferMessage message;
    int count;

    memset(&message, 0, sizeof(message));
    message.start = connection->outgoingBuffer.len;
    message.len = len;
    message.serialNumber = serialNumber;
    message.lastSend = current_time();
    message.resends = 0;
    count = ArrayLength(connection->outgoingBufferMessages);
    ArrayAppend(connection->outgoingBufferMessages, &message);
    return ArrayLength(connection->outgoingBufferMessages) == count + 1;
}
