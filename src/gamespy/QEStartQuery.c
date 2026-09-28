// QEStartQuery  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e390, size 348 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e390..0x61e4eb: EBX engine, stack server: onto the query list, stamped now,
//   then the query: version 1 -- fe fd 00, the time as request key, then the engine key list with two 0 bytes for a
//   basic query (state 4) or ff ff ff for everything; version 0 -- "\\basic\\\\info\\" or "\\status\\". Sent to the
//   private address when the server shares our public ip and has one (flag 2), else to the public one.
// blam-cc: EBX -> engine, stack -> server

#include "gamespy.h"

#include "sb.h"

void QEStartQuery(SBQueryEngine *engine, SBServer *server)
{
    unsigned char query[0x100];
    struct sockaddr_in address;
    int len;

    if (engine->querylist.last != 0) {
        engine->querylist.last->next = server;
    }
    engine->querylist.last = server;
    server->next = 0;
    if (engine->querylist.first == 0) {
        engine->querylist.first = server;
    }
    engine->querylist.count++;
    server->updatetime = current_time();
    if (engine->queryversion == 1) {
        *(unsigned long *)(query + 3) = server->updatetime;
        query[0] = 0xfe;
        query[1] = 0xfd;
        query[2] = 0;
        if (server->state & 4) {
            query[7] = (unsigned char)engine->numserverkeys;
            if (engine->numserverkeys > 0) {
                memcpy(query + 8, engine->serverkeys, engine->numserverkeys);
            }
            query[8 + engine->numserverkeys] = 0;
            query[9 + engine->numserverkeys] = 0;
            len = engine->numserverkeys + 10;
        } else {
            query[7] = 0xff;
            query[8] = 0xff;
            query[9] = 0xff;
            len = 10;
        }
    } else if (server->state & 4) {
        memcpy(query, "\\basic\\\\info\\", 13);
        len = 13;
    } else {
        memcpy(query, "\\status\\", 8);
        len = 8;
    }
    address.sin_family = AF_INET;
    if (server->publicip == engine->mypublicip && (server->flags & 2)) {
        address.sin_addr.s_addr = server->privateip;
        address.sin_port = server->privateport;
    } else {
        address.sin_addr.s_addr = server->publicip;
        address.sin_port = server->publicport;
    }
    sendto(engine->querysock, (const char *)query, len, 0, (const struct sockaddr *)&address, 0x10);
}
