// SBServerListConnectAndQuery  (GameSpy SDK in halo.exe; no C existed)
// address 0x61fd00, size 537 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61fd00..0x61ff18: (list, field list, filter, options, max servers): 6 when
//   either string is over 256 chars; connects (its error); sends the list request: length (network order), 0 1 3, the
//   from-game version, the two game names, the 8-byte challenge, the filter and fields, the options (network order),
//   the source ip for option 8 and the max for option 0x80. A failed send disconnects (3). Then the main list state
//   with a 4 KB input buffer (5 when out of memory); 0.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBServerListConnectAndQuery(SBServerList *slist, const char *fieldList, const char *serverFilter,
    int updateOptions, int maxServers)
{
    char request[0x2f0];
    char *write;
    int len;
    int error;

    if (fieldList == 0) {
        fieldList = "";
    }
    if (serverFilter == 0) {
        serverFilter = "";
    }
    if (strlen(fieldList) > 0x100 || strlen(serverFilter) > 0x100) {
        return 6;
    }
    error = ServerListConnect(slist);
    if (error != 0) {
        return error;
    }
    slist->queryoptions = updateOptions;
    SetupListChallenge(slist);
    *(int *)(request + 5) = slist->fromgamever;
    request[2] = 0;
    request[3] = 1;
    request[4] = 3;
    len = 9;
    write = request + 9;
    BufferAddNTS(&write, &len, slist->queryforgamename);
    BufferAddNTS(&write, &len, slist->queryfromgamename);
    memcpy(write, slist->mychallenge, 8);
    len += 8;
    write += 8;
    BufferAddNTS(&write, &len, serverFilter);
    BufferAddNTS(&write, &len, fieldList);
    *(unsigned int *)write = htonl((unsigned int)updateOptions);
    len += 4;
    write += 4;
    if (slist->queryoptions & 8) {
        *(unsigned int *)write = slist->srcip;
        len += 4;
        write += 4;
    }
    if (slist->queryoptions & 0x80) {
        *(int *)write = maxServers;
        len += 4;
    }
    *(unsigned short *)request = htons((unsigned short)len);
    if (send(slist->slsocket, request, len, 0) <= 0) {
        SBServerListDisconnect(slist);
        return 3;
    }
    slist->state = 3;
    slist->pstate = 0;
    if (slist->inbuffer == 0) {
        slist->inbuffer = (unsigned char *)malloc(0x1000);
        if (slist->inbuffer == 0) {
            return 5;
        }
        slist->inbufferlen = 0;
    }
    return 0;
}
