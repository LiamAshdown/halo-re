// gti2HandleNack  (GameSpy SDK in halo.exe; no C existed)
// address 0x618d60, size 161 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618d60..0x618e00: EAX data, ECX len, EDX connection: a 2-byte nack names one
//   serial, a 4-byte one a range (anything else is a negotiation error); every outgoing message in the range (16-bit)
//   is resent.
// blam-cc: EDX -> connection, EAX -> data, ECX -> len

#include "gamespy.h"

#include "gt2.h"

int gti2HandleNack(GTI2Connection *connection, const unsigned char *data, int len)
{
    unsigned short from = (unsigned short)((data[0] << 8) | data[1]);
    unsigned short to;
    int count;
    int i;

    if (len == 2) {
        to = from;
    } else if (len == 4) {
        to = (unsigned short)((data[2] << 8) | data[3]);
    } else {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    count = ArrayLength(connection->outgoingBufferMessages);
    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

        if ((short)(message->serialNumber - from) >= 0 && (short)(message->serialNumber - to) <= 0 &&
            !gti2ResendMessage(connection, message)) {
            return 0;
        }
    }
    return 1;
}
