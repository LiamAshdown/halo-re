// gti2HandleClosed  (GameSpy SDK in halo.exe; no C existed)
// address 0x618830, size 54 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618830..0x618865: EAX connection: the peer says it is closed; unless already
//   closed, an error with result 2 (rejected) and reason local close when we were closing, else remote close.
// blam-cc: EAX -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2HandleClosed(GTI2Connection *connection)
{
    if (connection->state == GTI2Closed) {
        return 1;
    }
    return gti2ConnectionError(connection, 2, connection->state != GTI2Closing) != 0;
}
