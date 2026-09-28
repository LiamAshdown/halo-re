// gti2ConnectionThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d130, size 115 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d130..0x61d1a2: timeouts; a keep-alive after 30 s without sending; resends; a
//   pending ack older than 100 ms goes out. 0 when any of them fails.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2ConnectionThink(GTI2Connection *connection, unsigned long now)
{
    if (!gti2CheckTimeout(connection, now)) {
        return 0;
    }
    if (now - connection->lastSend > 30000 && !gti2SendKeepAlive(connection)) {
        return 0;
    }
    if (!gti2ResendMessages(connection, now)) {
        return 0;
    }
    if (connection->pendingAck != 0 && now - connection->pendingAckTime > 100 && !gti2SendAck(connection)) {
        return 0;
    }
    return 1;
}
