// ServerBrowserSendNatNegotiateCookieToServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x616f50, size 44 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616f50..0x616f7b: (browser, dotted ip, port, cookie) ->
//   SBSendNatNegotiateCookieToServer with inet_addr / htons.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int ServerBrowserSendNatNegotiateCookieToServer(ServerBrowser *sb, const char *ip, unsigned short port, int cookie)
{
    unsigned short network_port = htons(port);

    return SBSendNatNegotiateCookieToServer(&sb->list, inet_addr(ip), network_port, cookie);
}
