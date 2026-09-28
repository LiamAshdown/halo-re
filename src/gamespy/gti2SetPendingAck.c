// gti2SetPendingAck  (GameSpy SDK in halo.exe; no C existed)
// address 0x6187c0, size 32 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6187c0..0x6187df: ESI connection: an ack becomes pending (from now) unless one
//   already is.
// blam-cc: ESI -> connection

#include "gamespy.h"

#include "gt2.h"

void gti2SetPendingAck(GTI2Connection *connection)
{
    if (connection->pendingAck == 0) {
        connection->pendingAck = 1;
        connection->pendingAckTime = current_time();
    }
}
