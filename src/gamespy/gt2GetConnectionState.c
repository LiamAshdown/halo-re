// gt2GetConnectionState  (GameSpy SDK in halo.exe; no C existed)
// address 0x6147a0, size 48 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6147a0..0x6147cf: the connection state (+0x0c) as GT2ConnectionState: below 5
//   connecting (0), 5 connected (1), 6 closing (2), above closed (3).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int gt2GetConnectionState(void *connection)
{
    int state = FIELD(connection, 0x0c, int);

    if (state < 5) {
        return 0;
    }
    if (state == 5) {
        return 1;
    }
    return state != 6 ? 3 : 2;
}
