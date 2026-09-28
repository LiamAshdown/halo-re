// SBAllocServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x617ab0, size 112 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617ab0..0x617b1f: a 0x24 byte server with a key table TableNew2(8, 8, 4,
//   KeyValHashKeyA, KeyValCompareKeyA, SBServerKeyValFree), the public ip and port, everything else 0; NULL when
//   either allocation fails. The server list argument is not used.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void *SBAllocServer(void *slist, unsigned int public_ip, unsigned short public_port)
{
    void *server = malloc(0x24);

    (void)slist;
    if (server == 0) {
        return 0;
    }
    FIELD(server, 0x18, HashTable) = TableNew2(sizeof(SBKeyValuePair), 8, 4, KeyValHashKeyA, KeyValCompareKeyA, SBServerKeyValFree);
    if (FIELD(server, 0x18, HashTable) == 0) {
        free(server);
        return 0;
    }
    FIELD(server, 0x00, unsigned int) = public_ip;
    FIELD(server, 0x14, unsigned char) = 0;
    FIELD(server, 0x15, unsigned char) = 0;
    FIELD(server, 0x20, void *) = 0;
    FIELD(server, 0x1c, int) = 0;
    FIELD(server, 0x10, unsigned int) = 0;
    FIELD(server, 0x08, unsigned int) = 0;
    FIELD(server, 0x0c, unsigned short) = 0;
    FIELD(server, 0x04, unsigned short) = public_port;
    return server;
}
