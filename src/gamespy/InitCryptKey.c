// InitCryptKey  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ec20, size 113 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ec20..0x61ec90: EAX list, EBX key, stack length: folds the key into the
//   challenge -- challenge[(signed)(secret[i % len] * i) % 8] ^= challenge[i & 7] ^ key[i] (a negative index is kept)
//   -- and keys the list cipher with the 8 challenge bytes.
// blam-cc: EAX -> slist, EBX -> key, stack -> keylen

#include "gamespy.h"

#include "sb.h"

void InitCryptKey(SBServerList *slist, const unsigned char *key, int keylen)
{
    char *secret = slist->queryfromkey;
    int secretlen = (int)strlen(secret);
    int i;

    for (i = 0; i < keylen; i++) {
        int index = ((int)secret[i % secretlen] * i) % 8;

        ((unsigned char *)slist->mychallenge)[index] ^= slist->mychallenge[i & 7] ^ key[i];
    }
    GOACryptInit(&slist->cryptkey, slist->mychallenge, 8);
}
