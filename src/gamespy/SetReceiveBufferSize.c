// SetReceiveBufferSize  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d2e0, size 38 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d2e0..0x61d305: setsockopt(SOL_SOCKET, SO_RCVBUF, size) (WS2_32 #21); whether
//   it did not fail.
// blam-cc: cdecl

#include "gamespy.h"

int SetReceiveBufferSize(SOCKET sock, int size)
{
    return setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(size)) != SOCKET_ERROR;
}
