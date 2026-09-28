// WaitForTriggerUpdate  (GameSpy SDK in halo.exe; no C existed)
// address 0x617240, size 66 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617240..0x617281: EDI browser, EBX via master: while the trigger server is
//   pending and nothing failed: sleep 10 ms, think; via the master it also stops once the list is back to LAN (0)
//   state.
// blam-cc: EDI -> sb, EBX -> viaMaster

#include "gamespy.h"

#include "sb.h"

int WaitForTriggerUpdate(ServerBrowser *sb, int viaMaster)
{
    int error = 0;

    while (sb->triggerIP != 0 && error == 0) {
        msleep(10);
        SBQueryEngineThink(&sb->engine);
        error = SBListThink(&sb->list);
        if (viaMaster != 0 && sb->list.state == 0) {
            break;
        }
    }
    return error;
}
