// gti2SocketFindConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c360, size 118 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c360..0x61c3d5: looks the (ip, port) up in the connection table through a
//   stack connection record holding just those two; the connection or NULL.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

GTI2Connection *gti2SocketFindConnection(GTI2Socket *socket, unsigned int ip, unsigned short port)
{
    GTI2Connection key;
    GTI2Connection *key_pointer = &key;
    GTI2Connection **found;

    key.ip = ip;
    key.port = port;
    found = (GTI2Connection **)TableLookup(socket->connections, &key_pointer);
    if (found != 0) {
        return *found;
    }
    return 0;
}
