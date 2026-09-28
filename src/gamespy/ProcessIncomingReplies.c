// ProcessIncomingReplies  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e7c0, size 232 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e7c0..0x61e8a7: ESI engine: every waiting datagram (up to 0x833 bytes, NUL-
//   terminated) goes to the queried server it came from (public address, or the private one for a server behind our
//   own public ip), parsed by query version.
// blam-cc: ESI -> engine

#include "gamespy.h"

#include "sb.h"

void ProcessIncomingReplies(SBQueryEngine *engine)
{
    char buffer[0x834];
    struct sockaddr_in from;
    int fromlen = 0x10;
    int len;

    while (CanReceiveOnSocket(engine->querysock)) {
        SBServer *server;

        len = recvfrom(engine->querysock, buffer, 0x833, 0, (struct sockaddr *)&from, &fromlen);
        if (len == SOCKET_ERROR) {
            return;
        }
        buffer[len] = 0;
        for (server = engine->querylist.first; server != 0; server = server->next) {
            if ((server->publicip == from.sin_addr.s_addr && server->publicport == from.sin_port) ||
                (server->publicip == engine->mypublicip && (server->flags & 2) &&
                 server->privateip == from.sin_addr.s_addr && server->privateport == from.sin_port)) {
                if (engine->queryversion == 1) {
                    ParseSingleQR2Reply(engine, server, buffer, len);
                } else {
                    ParseSingleGOAReply(engine, server, buffer);
                }
                break;
            }
        }
    }
}
