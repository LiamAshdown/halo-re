// current_time  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d200, size 6 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d200..0x61d205: the GetTickCount import thunk (jmp [0x63a0d0]).
// blam-cc: cdecl

#include "gamespy.h"

unsigned long current_time(void)
{
    return GetTickCount();
}
