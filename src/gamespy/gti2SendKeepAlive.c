// gti2SendKeepAlive  (GameSpy SDK in halo.exe; no C existed)
// address 0x6194c0, size 69 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6194c0..0x619504: type 7, header only.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendKeepAlive(GTI2Connection *connection)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, 7, GTI2MsgKeepAlive, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    return gti2SendLastOutgoingMessage(connection) != 0;
}
