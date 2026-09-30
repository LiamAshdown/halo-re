// gti2GetResponse  (GameSpy SDK in halo.exe; no C existed)
// address 0x620750, size 259 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620750..0x620852: the 32-byte response to a challenge: bytes 0 and 13, and all
//   of them when the challenge fails gti2VerifyChallenge (EDI), are random printable; the rest mix the challenge with
//   the key string 0x683db0 (signed chars, % its length): idx = (key[(c[i] + i) % klen] + c[i] * i) & 31 (signed), v
//   = c[idx] ^ key[(prev * i * 0x4647) % klen] where prev is c[i] for i 1 and 14 else c[i-1]; byte = |v| % 0x5d +
//   0x21.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

static const char GT2ChallengeKey[] = "3b8dd8995f7c40a9a5c5b7dd5b481341"; // the string at 0x00683db0


unsigned char *gti2GetResponse(unsigned char *response, const unsigned char *challenge)
{
    int key_length = (int)strlen(GT2ChallengeKey);
    int valid = gti2VerifyChallenge(challenge);
    int i;

    for (i = 0; i < 0x20; i++) {
        if (valid == 0 || i == 0 || i == 0xd) {
            response[i] = (unsigned char)(rand() % 0x5d + 0x21);
        } else {
            char previous;
            int index;
            int value;

            if (i == 1 || i == 0xe) {
                previous = (char)challenge[i];
            } else {
                previous = (char)challenge[i - 1];
            }
            index = ((int)GT2ChallengeKey[(int)(challenge[i] + i) % key_length] + (int)challenge[i] * i) % 0x20;
            value = (int)challenge[index] ^ (int)GT2ChallengeKey[(previous * i * 0x4647) % key_length];
            if (value < 0) {
                value = -value;
            }
            response[i] = (unsigned char)(value % 0x5d + 0x21);
        }
    }
    return response;
}
