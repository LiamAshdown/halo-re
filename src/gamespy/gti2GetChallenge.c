// gti2GetChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x6206c0, size 137 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6206c0..0x620748: seeds rand with current_time, then 32 printable bytes (rand %
//   0x5d + 0x21), each after the first bumped by one when its parity does not match the running check
//   gti2VerifyChallenge applies.
// blam-cc: cdecl

#include "gamespy.h"

unsigned char *gti2GetChallenge(unsigned char *challenge)
{
    int parity = 0;
    int i;

    srand(current_time());
    challenge[0] = (unsigned char)(rand() % 0x5d + 0x21);
    for (i = 1; i < 0x20; i++) {
        unsigned char c;

        parity ^= ((challenge[i - 1] ^ challenge[0] ^ i) & 1) ^ (challenge[i - 1] < challenge[0]) ^ (challenge[0] < 0x4f);
        c = (unsigned char)(rand() % 0x5d + 0x21);
        challenge[i] = c;
        if (parity != 0) {
            if ((c & 1) == 0) {
                challenge[i] = (unsigned char)(c + 1);
            }
        } else if ((c & 1) != 0) {
            challenge[i] = (unsigned char)(c + 1);
        }
    }
    return challenge;
}
