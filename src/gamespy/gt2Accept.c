// gt2Accept  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ce80, size 85 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ce80..0x61ced4: clears freeAtAcceptReject; only a connection that was not so
//   marked and awaits accept/reject is accepted: the accept (with our public key) goes out, it is connected (5) and
//   takes the callbacks when given; 1, else 0.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gt2Accept(GTI2Connection *connection, const GT2ConnectionCallbacks *callbacks)
{
    int was_marked = connection->freeAtAcceptReject;

    connection->freeAtAcceptReject = 0;
    if (was_marked != 0 || connection->state != GTI2AwaitingAcceptReject) {
        return 0;
    }
    gti2SendAccept(connection);
    connection->state = GTI2Connected;
    if (callbacks != 0) {
        connection->callbacks = *callbacks;
    }
    return 1;
}
