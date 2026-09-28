// gt2Send  (GameSpy SDK in halo.exe; no C existed)
// address 0x6146b0, size 84 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6146b0..0x614703: only on a connected connection: the message is checked (NULL
//   / -1 length), then goes through send filter 0 when any are registered, else straight to gti2Send (which appends a
//   CRC and encrypts IN PLACE, so the buffer needs 4 spare bytes). Returns what that returned; 0 when not connected
//   (the binary leaves EAX as it was).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gt2Send(GTI2Connection *connection, const unsigned char *message, int len, int reliable)
{
    if (connection->state != GTI2Connected) {
        return 0;
    }
    gti2MessageCheck((const char **)&message, &len);
    if (ArrayLength(connection->sendFilters) != 0) {
        return gti2SendFilterCallback(connection, 0, message, len, reliable);
    }
    return gti2Send(connection, (unsigned char *)message, len, reliable);
}
