// GOADecryptByte  (GameSpy SDK in halo.exe; no C existed)
// address 0x623020, size 239 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x623020..0x62310e: the GameSpy card-shuffle stream cipher, one byte.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

unsigned char GOADecryptByte(GOACryptState *state, unsigned char b)
{
    unsigned char swaptemp;

    state->ratchet = (unsigned char)(state->ratchet + state->cards[state->rotor]);
    state->rotor++;
    swaptemp = state->cards[state->last_cipher];
    state->cards[state->last_cipher] = state->cards[state->ratchet];
    state->cards[state->ratchet] = state->cards[state->last_plain];
    state->cards[state->last_plain] = state->cards[state->rotor];
    state->cards[state->rotor] = swaptemp;
    state->avalanche = (unsigned char)(state->avalanche + state->cards[swaptemp]);
    state->last_plain = (unsigned char)(b ^
        state->cards[(state->cards[state->avalanche] + state->cards[state->rotor]) & 0xff] ^
        state->cards[state->cards[(state->cards[state->last_plain] + state->cards[state->ratchet] +
                                   state->cards[state->last_cipher]) & 0xff]]);
    state->last_cipher = b;
    return state->last_plain;
}
