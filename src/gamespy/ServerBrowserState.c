// ServerBrowserState  (GameSpy SDK in halo.exe; no C existed)
// address 0x616ff0, size 44 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616ff0..0x61701b: querying (2) while the engine has queries outstanding
//   (+0x10); otherwise from the list state (+0x48): 3 (connected) for list state 1, 0 for 0 or 3, 1 otherwise.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int ServerBrowserState(void *sb)
{
    int state;

    if (FIELD(sb, 0x10, int) > 0) {
        return 2;
    }
    state = FIELD(sb, 0x48, int);
    if (state == 3 || state == 0) {
        return 1;
    }
    return state == 1 ? 0 : 3;
}
