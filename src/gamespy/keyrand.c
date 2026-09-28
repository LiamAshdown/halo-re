// keyrand  (GameSpy SDK in halo.exe; no C existed)
// address 0x622ea0, size 127 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622ea0..0x622f1e: stack state, limit, key, keysize (byte); ECX keypos, ESI
//   rsum: 0 for limit 0; otherwise the smallest all-ones mask covering limit, then rsum = cards[rsum] + key[keypos++]
//   (the key recycles with rsum += keysize), u = rsum & mask -- after 11 tries reduced modulo limit -- until u <=
//   limit.
// blam-cc: stack -> state, limit, key, keysize; ECX -> keypos, ESI -> rsum

#include "gamespy.h"

#include "sb.h"

unsigned char keyrand(GOACryptState *state, unsigned int limit, const unsigned char *key, unsigned char keysize,
    unsigned char *rsum, unsigned int *keypos)
{
    unsigned int mask = 1;
    unsigned int retry = 0;
    unsigned int u;

    if (limit == 0) {
        return 0;
    }
    while (mask < limit) {
        mask = mask * 2 + 1;
    }
    do {
        *rsum = (unsigned char)(state->cards[*rsum] + key[*keypos]);
        (*keypos)++;
        if (*keypos >= keysize) {
            *keypos = 0;
            *rsum = (unsigned char)(*rsum + keysize);
        }
        u = *rsum & mask;
        if (++retry > 11) {
            u %= limit;
        }
    } while (u > limit);
    return (unsigned char)u;
}
