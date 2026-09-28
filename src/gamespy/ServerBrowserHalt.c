// ServerBrowserHalt  (GameSpy SDK in halo.exe; no C existed)
// address 0x616fa0, size 25 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616fa0..0x616fb8: disconnects the list and halts the engine.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void ServerBrowserHalt(ServerBrowser *sb)
{
    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
}
