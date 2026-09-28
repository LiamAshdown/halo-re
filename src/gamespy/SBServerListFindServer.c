// SBServerListFindServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f030, size 99 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f030..0x61f092: the index of the server with this public ip and port, or -1.
//   (The binary reads the ip through 0x6175f0 and the port through 0x6147d0 -- the linker folded
//   SBServerGetPublicInetAddress / the raw port getter into ArrayLength / gt2GetRemotePort; plain field reads here.)
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerListFindServer(void *slist, unsigned int ip, unsigned short port)
{
    int count = ArrayLength(FIELD(slist, 0x04, DArray));
    int i;

    for (i = 0; i < count; i++) {
        void *server = *(void **)ArrayNth(FIELD(slist, 0x04, DArray), i);

        if (FIELD(server, 0x00, unsigned int) == ip && FIELD(server, 0x04, unsigned short) == port) {
            return i;
        }
    }
    return -1;
}
