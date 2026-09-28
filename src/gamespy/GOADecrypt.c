// GOADecrypt  (GameSpy SDK in halo.exe; no C existed)
// address 0x623110, size 50 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x623110..0x623141: GOADecryptByte over the buffer in place.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void GOADecrypt(GOACryptState *state, unsigned char *data, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        data[i] = GOADecryptByte(state, data[i]);
    }
}
