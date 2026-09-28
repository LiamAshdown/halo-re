// GOAHashInit  (GameSpy SDK in halo.exe; no C existed)
// address 0x622f20, size 61 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622f20..0x622f5c: rotor 1, ratchet 3, avalanche 5, last plain 7, last cipher
//   11; cards 255 down to 0.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void GOAHashInit(GOACryptState *state)
{
    int i;

    state->rotor = 1;
    state->ratchet = 3;
    state->avalanche = 5;
    state->last_plain = 7;
    state->last_cipher = 11;
    for (i = 0; i < 0x100; i++) {
        state->cards[i] = (unsigned char)(0xff - i);
    }
}
