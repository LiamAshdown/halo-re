// gti2SendClosed  (GameSpy SDK in halo.exe; no C existed)
// address 0x618b50, size 93 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618b50..0x618bac: fe fe 68 straight to the address (formatted with
//   gt2AddressToString first, unused).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendClosed(GTI2Socket *socket, unsigned int ip, unsigned short port)
{
    unsigned char packet[3];
    char address[0x18];

    gt2AddressToString(ip, port, address);
    packet[0] = 0xfe;
    packet[1] = 0xfe;
    packet[2] = GTI2MsgClosed;
    return gti2SocketSend(socket, ip, port, packet, 3, 0, 0) != 0;
}
