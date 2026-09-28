// SBSendNatNegotiateCookieToServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x6204c0, size 96 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6204c0..0x62051f: the NAT negotiation magic fd fc 1e 66 6a b2 and the cookie
//   (network order) through SBSendMessageToServer.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBSendNatNegotiateCookieToServer(SBServerList *slist, unsigned int ip, unsigned short port, int cookie)
{
    unsigned char message[10];

    message[0] = 0xfd;
    message[1] = 0xfc;
    message[2] = 0x1e;
    message[3] = 0x66;
    message[4] = 0x6a;
    message[5] = 0xb2;
    *(unsigned int *)(message + 6) = htonl((unsigned int)cookie);
    return SBSendMessageToServer(slist, ip, port, (const char *)message, 10);
}
