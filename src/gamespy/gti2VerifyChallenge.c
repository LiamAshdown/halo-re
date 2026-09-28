// gti2VerifyChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x620650, size 105 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620650..0x6206b8: EDI challenge (32 bytes): byte i (1..31) must have the parity
//   of the running XOR of ((c[i-1] ^ c[0] ^ i) & 1) ^ (c[i-1] < c[0]) ^ (c[0] < 0x4f).
// blam-cc: EDI -> challenge

#include "gamespy.h"

int gti2VerifyChallenge(const unsigned char *challenge)
{
    unsigned char first = challenge[0];
    int parity = 0;
    int i;

    for (i = 1; i < 0x20; i++) {
        parity ^= ((challenge[i - 1] ^ first ^ i) & 1) ^ (challenge[i - 1] < first) ^ (first < 0x4f);
        if (parity != 0) {
            if ((challenge[i] & 1) == 0) {
                return 0;
            }
        } else if ((challenge[i] & 1) != 0) {
            return 0;
        }
    }
    return 1;
}
