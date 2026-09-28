// gti2SendClose  (GameSpy SDK in halo.exe; no C existed)
// address 0x619470, size 69 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619470..0x6194b4: type 6, header only.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendClose(GTI2Connection *connection)
{
    int overflow;

    if (!gti2BeginReliableMessage(connection, 7, GTI2MsgClose, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
    return gti2SendLastOutgoingMessage(connection) != 0;
}
