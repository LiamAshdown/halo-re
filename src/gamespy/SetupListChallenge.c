// SetupListChallenge  (GameSpy SDK in halo.exe; no C existed)
// address 0x61eb90, size 140 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61eb90..0x61ec1b: the 8-byte list challenge at +0x6c: printable rand bytes
//   whose parities follow the running check ((i ^ prev ^ first) & 1) ^ (prev < first) ^ (first < 0x4f) with SIGNED
//   byte compares, like gti2GetChallenge.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SetupListChallenge(SBServerList *slist)
{
    unsigned char *challenge = slist->mychallenge;
    int parity = 0;
    int i;
    int r;

    r = rand();
    challenge[0] = (unsigned char)(r % 0x5d + 0x21);
    for (i = 1; i < 8; i++) {
        unsigned char c;

        parity ^= ((i ^ challenge[i - 1] ^ challenge[0]) & 1) ^ ((signed char)challenge[i - 1] < (signed char)challenge[0]) ^
                  ((signed char)challenge[0] < 0x4f);
        r = rand();
        c = (unsigned char)(r % 0x5d + 0x21);
        challenge[i] = c;
        if (parity != 0) {
            if ((c & 1) == 0) {
                challenge[i] = (unsigned char)(c + 1);
            }
        } else if ((c & 1) != 0) {
            challenge[i] = (unsigned char)(c + 1);
        }
    }
    return r / 0x5d;
}
