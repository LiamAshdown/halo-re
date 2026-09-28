// gti2RemoveIncomingBufferMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618740, size 125 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618740..0x6187bc: EAX message, ECX connection, EDX index: deletes the entry
//   (its start and length read first), moves every later message down by the length and cuts those bytes out of the
//   incoming buffer.
// blam-cc: ECX -> connection, EDX -> index, EAX -> message

#include "gamespy.h"

#include "gt2.h"

void gti2RemoveIncomingBufferMessage(GTI2Connection *connection, int index, const GTI2IncomingBufferMessage *message)
{
    int start = message->start;
    int len = message->len;
    int count;
    int i;

    ArrayDeleteAt(connection->incomingBufferMessages, index);
    count = ArrayLength(connection->incomingBufferMessages);
    for (i = 0; i < count; i++) {
        GTI2IncomingBufferMessage *other = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);

        if (other->start > start) {
            other->start -= len;
        }
    }
    gti2BufferShorten(&connection->incomingBuffer, start, len);
}
