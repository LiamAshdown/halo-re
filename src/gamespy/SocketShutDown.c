// SocketShutDown  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d260, size 5 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d260..0x61d264: jmp to the WSACleanup delay-import thunk (WSOCK32 #116).
// blam-cc: cdecl

#include "gamespy.h"

void SocketShutDown(void)
{
    WSACleanup();
}
