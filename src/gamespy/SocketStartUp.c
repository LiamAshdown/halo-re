// SocketStartUp  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d220, size 51 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d220..0x61d252: WSAStartup(1.1) into a stack WSADATA (delay import WS2_32
//   #115).
// blam-cc: cdecl

#include "gamespy.h"

void SocketStartUp(void)
{
    WSADATA data;

    WSAStartup(0x101, &data);
}
