// gti2BufferIncomingMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618c30, size 294 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618c30..0x618d55: EAX len, ESI connection, stack type, serial, message,
//   overflow out: an out-of-order reliable message is kept (dropped when the incoming buffer or the sorted record
//   list cannot take it): its record (start, len, type, serial) goes in by serial (static compare 0x618720), its
//   bytes at the buffer end. When it is the only one, a nack asks for expected..serial-1; when it went in last, a gap
//   before it is nacked. *overflow is always 0.
// blam-cc: EAX -> len, ESI -> connection, stack -> type, serialNumber, message, overflow

#include "gamespy.h"

#include "gt2.h"

static int gti2IncomingBufferMessageCompare(const void *elem1, const void *elem2) // 0x618720
{
    return (short)(((const GTI2IncomingBufferMessage *)elem1)->serialNumber -
                   ((const GTI2IncomingBufferMessage *)elem2)->serialNumber);
}

int gti2BufferIncomingMessage(GTI2Connection *connection, int len, int type, unsigned short serialNumber,
    const unsigned char *message, int *overflow)
{
    GTI2IncomingBufferMessage record;
    int count;

    if (gti2GetBufferFreeSpace(&connection->incomingBuffer) < len) {
        *overflow = 0;
        return 1;
    }
    record.start = connection->incomingBuffer.len;
    record.len = len;
    record.type = type;
    record.serialNumber = serialNumber;
    count = ArrayLength(connection->incomingBufferMessages);
    ArrayInsertSorted(connection->incomingBufferMessages, &record, gti2IncomingBufferMessageCompare);
    if (ArrayLength(connection->incomingBufferMessages) != count + 1) {
        *overflow = 0;
        return 1;
    }
    gti2BufferWriteData(&connection->incomingBuffer, message, len);
    if (count == 0) {
        if (!gti2SendNack(connection, connection->expectedSerialNumber, (unsigned short)(serialNumber - 1))) {
            return 0;
        }
    } else if (((GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, count))->serialNumber ==
               serialNumber) {
        unsigned short previous =
            ((GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, count - 1))->serialNumber;

        if ((unsigned short)(serialNumber - previous) > 1 &&
            !gti2SendNack(connection, (unsigned short)(previous + 1), (unsigned short)(serialNumber - 1))) {
            return 0;
        }
    }
    *overflow = 0;
    return 1;
}
