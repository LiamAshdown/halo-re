// gti2ResendMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618bb0, size 125 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618bb0..0x618c2c: rewrites the ack field (bytes 5-6) of the buffered message
//   with the expected serial and sends it again (dump args 0, 1); then it counts a resend (the connection keeps the
//   most), is stamped with the send time, and a server challenge (type 2) restarts the challenge clock.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ResendMessage(GTI2Connection *connection, GTI2OutgoingBufferMessage *message)
{
    unsigned char *data = connection->outgoingBuffer.buffer + message->start;

    data[5] = (unsigned char)(connection->expectedSerialNumber >> 8);
    data[6] = (unsigned char)connection->expectedSerialNumber;
    if (!gti2ConnectionSendData(connection, connection->outgoingBuffer.buffer + message->start, message->len, 0, 1)) {
        return 0;
    }
    message->lastSend = connection->lastSend;
    message->resends++;
    if (message->resends > connection->maxResends) {
        connection->maxResends = message->resends;
    }
    if (connection->outgoingBuffer.buffer[message->start + 2] == GTI2MsgServerChallenge) {
        connection->challengeTime = connection->lastSend;
    }
    return 1;
}
