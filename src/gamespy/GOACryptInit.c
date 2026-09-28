// GOACryptInit  (GameSpy SDK in halo.exe; no C existed)
// address 0x622f60, size 181 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622f60..0x623014: no key: GOAHashInit. Otherwise the cards in order, each from
//   255 down to 0 swapped with keyrand(i); then rotor = cards[1], ratchet = cards[3], avalanche = cards[5], last
//   plain = cards[7], last cipher = cards[rsum].
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

extern unsigned char keyrand(GOACryptState *state, unsigned int limit, const unsigned char *key, unsigned char keysize,
    unsigned char *rsum, unsigned int *keypos);

void GOACryptInit(GOACryptState *state, const unsigned char *key, unsigned char keysize)
{
    unsigned int keypos = 0;
    unsigned char rsum = 0;
    int i;

    if (keysize < 1) {
        GOAHashInit(state);
        return;
    }
    for (i = 0; i < 0x100; i++) {
        state->cards[i] = (unsigned char)i;
    }
    for (i = 0xff; i >= 0; i--) {
        unsigned char toswap = keyrand(state, (unsigned int)i, key, keysize, &rsum, &keypos);
        unsigned char swaptemp = state->cards[i];

        state->cards[i] = state->cards[toswap];
        state->cards[toswap] = swaptemp;
    }
    state->ratchet = state->cards[3];
    state->rotor = state->cards[1];
    state->avalanche = state->cards[5];
    state->last_plain = state->cards[7];
    state->last_cipher = state->cards[rsum];
}
