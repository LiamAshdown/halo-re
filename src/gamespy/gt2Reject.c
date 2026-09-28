// gt2Reject  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cee0, size 61 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cee0..0x61cf1c: clears freeAtAcceptReject; a connection awaiting
//   accept/reject is sent the reject message and closes (6).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gt2Reject(GTI2Connection *connection, const unsigned char *message, int len)
{
    connection->freeAtAcceptReject = 0;
    if (connection->state != GTI2AwaitingAcceptReject) {
        return;
    }
    gti2MessageCheck((const char **)&message, &len);
    gti2SendReject(connection, message, len);
    connection->state = GTI2Closing;
}
