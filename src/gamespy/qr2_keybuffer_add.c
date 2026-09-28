// qr2_keybuffer_add  (GameSpy SDK in halo.exe; no C existed)
// address 0x615560, size 45 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615560..0x61558c: appends a key id (1..0xfe) to a key buffer (bytes, count at
//   +0x100) holding fewer than 0xfe keys.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void qr2_keybuffer_add(unsigned char *keybuffer, int key_id)
{
    int count = FIELD(keybuffer, 0x100, int);

    if (count >= 0xfe || key_id < 1 || key_id > 0xfe) {
        return;
    }
    keybuffer[count] = (unsigned char)key_id;
    FIELD(keybuffer, 0x100, int) = count + 1;
}
