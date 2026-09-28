// SBIsNullServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x617b80, size 18 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617b80..0x617b91: whether the server is the shared null server (0x006a27f8).
// blam-cc: cdecl

#include "gamespy.h"

extern unsigned char SBNullServer[]; // 0x006a27f8

int SBIsNullServer(void *server)
{
    return server == (void *)SBNullServer;
}
