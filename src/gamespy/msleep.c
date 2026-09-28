// msleep  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d210, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d210..0x61d21b: Sleep(msec).
// blam-cc: cdecl

#include "gamespy.h"

void msleep(unsigned long msec)
{
    Sleep(msec);
}
