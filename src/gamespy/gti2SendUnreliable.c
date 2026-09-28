// gti2SendUnreliable  (GameSpy SDK in halo.exe; no C existed)
// address 0x6189c0, size 165 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6189c0..0x618a64: a message that itself starts with the 0xfe 0xfe magic is sent
//   escaped behind another magic, built at the end of the outgoing buffer (not sent at all when it does not fit) and
//   cut off again; anything else is sent as it is.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendUnreliable(GTI2Connection *connection, const unsigned char *message, int len)
{
    if (len >= 2 && *(const unsigned short *)message == *(const unsigned short *)GTI2Magic) {
        GTI2Buffer *buffer = &connection->outgoingBuffer;
        int total = len + 2;

        if (gti2GetBufferFreeSpace(buffer) >= total) {
            unsigned char *start = buffer->buffer + buffer->len;

            gti2BufferWriteData(buffer, GTI2Magic, 2);
            gti2BufferWriteData(buffer, message, len);
            if (!gti2ConnectionSendData(connection, start, total, 0, 0)) {
                return 0;
            }
            gti2BufferShorten(buffer, -1, total);
        }
        return 1;
    }
    return gti2ConnectionSendData(connection, message, len, 0, 0) != 0;
}
