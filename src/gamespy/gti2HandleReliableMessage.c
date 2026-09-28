// gti2HandleReliableMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x6198b0, size 253 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6198b0..0x6199ac: EAX len, ECX message, EDX connection, stack type: fewer than
//   7 bytes is a negotiation error. The header ack (bytes 5-6) is handled first. The expected serial (bytes 3-4) is
//   delivered (an ack becomes pending) followed by any queued ones; an older one only makes an ack pending; a newer
//   one is buffered (running out of memory there would close the connection).
// blam-cc: EDX -> connection, ECX -> message, EAX -> len, stack -> type

#include "gamespy.h"

#include "gt2.h"

int gti2HandleReliableMessage(GTI2Connection *connection, unsigned char *message, int len, int type)
{
    unsigned short serial;
    unsigned short ack;
    unsigned char *data;
    int overflow;

    if (len < 7) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    serial = (unsigned short)((message[3] << 8) | message[4]);
    ack = (unsigned short)((message[5] << 8) | message[6]);
    data = message + 7;
    len -= 7;
    if (!gti2HandleAck(connection, ack)) {
        return 0;
    }
    if (serial == connection->expectedSerialNumber) {
        if (connection->pendingAck == 0) {
            connection->pendingAck = 1;
            connection->pendingAckTime = current_time();
        }
        if (!gti2DeliverReliableMessage(connection, type, data, len)) {
            return 0;
        }
        return gti2DeliverQueuedMessages(connection) != 0;
    }
    if ((short)(serial - connection->expectedSerialNumber) < 0) {
        gti2SetPendingAck(connection);
        return 1;
    }
    if (!gti2BufferIncomingMessage(connection, len, type, serial, data, &overflow)) {
        return 0;
    }
    if (overflow != 0 && !gti2HandleOutOfMemory(connection)) {
        return 0;
    }
    return 1;
}
