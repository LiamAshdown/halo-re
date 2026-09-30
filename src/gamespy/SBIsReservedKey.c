// SBIsReservedKey  (GameSpy SDK in halo.exe; no C existed)
// address 0x617690, size 116 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617690..0x617703: returns 0 for the keys "queryid" and "final" (0x0064e498,
//   0x0064e490), else 1 (a key worth storing).
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

int SBIsReservedKey(const char *key)
{
    static const char *const reserved[2] = { "queryid", "final" };
    int i;

    for (i = 0; i < 2; i++) {
        if (strcmp(key, reserved[i]) == 0) {
            return 0;
        }
    }
    return 1;
}
