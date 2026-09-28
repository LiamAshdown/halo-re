// gti2HandleAck  (GameSpy SDK in halo.exe; no C existed)
// address 0x618420, size 209 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618420..0x6184f0: EAX connection, stack serial: notes the ack time; every
//   outgoing message before the first whose serial is not older (16-bit) is dropped. When that empties the list the
//   outgoing buffer length is reset, otherwise the remaining messages move down by the first one  start and the
//   buffer is shortened to match.
// blam-cc: EAX -> connection, stack -> serialNumber

#include "gamespy.h"

#include "gt2.h"

int gti2HandleAck(GTI2Connection *connection, unsigned short serialNumber)
{
    int count;
    int i;
    int shift;

    connection->lastAck = current_time();
    count = ArrayLength(connection->outgoingBufferMessages);
    if (count == 0) {
        return 1;
    }
    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

        if ((short)(message->serialNumber - serialNumber) >= 0) {
            break;
        }
    }
    if (i == 0) {
        return 1;
    }
    do {
        i--;
        ArrayDeleteAt(connection->outgoingBufferMessages, i);
    } while (i != 0);
    count = ArrayLength(connection->outgoingBufferMessages);
    if (count == 0) {
        connection->outgoingBuffer.len = 0;
        return 1;
    }
    shift = ((GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, 0))->start;
    for (i = 0; i < count; i++) {
        ((GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i))->start -= shift;
    }
    gti2BufferShorten(&connection->outgoingBuffer, 0, shift);
    return 1;
}
