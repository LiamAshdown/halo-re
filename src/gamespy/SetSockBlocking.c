// SetSockBlocking  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d2b0, size 41 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d2b0..0x61d2d8: ioctlsocket(FIONBIO, !is_blocking) (WSOCK32 #12); whether it
//   succeeded.
// blam-cc: cdecl

#include "gamespy.h"

int SetSockBlocking(SOCKET sock, int is_blocking)
{
    u_long argp = is_blocking == 0;

    return ioctlsocket(sock, FIONBIO, &argp) == 0;
}
