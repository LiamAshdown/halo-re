// SBQueryEngineAddQueryKey  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e9a0, size 24 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e9a0..0x61e9b7: appends a key id while fewer than 20.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBQueryEngineAddQueryKey(SBQueryEngine *engine, unsigned char keyid)
{
    if (engine->numserverkeys < 0x14) {
        engine->serverkeys[engine->numserverkeys] = keyid;
        engine->numserverkeys++;
    }
}
