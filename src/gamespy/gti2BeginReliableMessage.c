// gti2BeginReliableMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x6190f0, size 269 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6190f0..0x6191fc: EAX connection, EBX total length, stack type, overflow out:
//   when the outgoing buffer or its record list cannot take the message the peer is told it is closed and the
//   connection fails out of memory (1, 4), *overflow = 1. Otherwise the header goes in: fe fe, type, the next serial
//   and the expected serial (high bytes first), *overflow = 0.
// blam-cc: EAX -> connection, EBX -> len, stack -> type, overflow

#include "gamespy.h"

#include "gt2.h"

int gti2BeginReliableMessage(GTI2Connection *connection, int len, int type, int *overflow)
{
    GTI2Buffer *buffer = &connection->outgoingBuffer;
    unsigned short serial;

    if (gti2GetBufferFreeSpace(buffer) < len || !gti2AddOutgoingBufferMessage(connection, len, connection->serialNumber)) {
        if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
            return 0;
        }
        if (!gti2ConnectionError(connection, 1, 4)) {
            return 0;
        }
        *overflow = 1;
        return 1;
    }
    gti2BufferWriteData(buffer, GTI2Magic, 2);
    gti2BufferWriteByte(buffer, (unsigned char)type);
    serial = connection->serialNumber++;
    gti2BufferWriteUShort(buffer, serial);
    gti2BufferWriteUShort(buffer, connection->expectedSerialNumber);
    *overflow = 0;
    return 1;
}
