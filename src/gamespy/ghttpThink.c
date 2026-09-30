// ghttpThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c020, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c020..0x61c02b: processes every live connection.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"
#include "fn_gamespy.h"

void ghttpThink(void)
{
    ghiEnumConnections(ghiProcessConnection);
}
