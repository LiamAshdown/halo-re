// ProcessMainListData  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f6a0, size 860 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f6a0..0x61f9fb: ESI list: the master reply state machine over the input
//   buffer. 0: the crypt header (lengths xor 0xec / 0xea), the key folds into the challenge and the rest is
//   decrypted. 1: our public ip (callback 6) and the default port; 0xffff is an error string (callback 5). Without
//   servers wanted (option 2) or after an error it is connected (2). 2: the key list (type byte, name). 3: the
//   popular values. 4: server records until the end marker (connected, callback 3 initial list complete). Out of
//   memory: 5. Whatever is left moves to the buffer start; 0.
// blam-cc: ESI -> slist

#include "gamespy.h"

#include "sb.h"
#include "fn_gamespy.h"

extern SBServer *SBNullServer; // 0x006a27f8


int ProcessMainListData(SBServerList *slist)
{
    unsigned char *data = slist->inbuffer;
    int len = slist->inbufferlen;
    int i;
    int n;

    switch (slist->pstate) {
    case 0: {
        int cryptlen;
        int keylen;

        if (len < 1) {
            break;
        }
        cryptlen = (data[0] ^ 0xec) + 2;
        if (len < cryptlen) {
            break;
        }
        keylen = data[cryptlen - 1] ^ 0xea;
        if (len < cryptlen + keylen) {
            break;
        }
        InitCryptKey(slist, data + cryptlen, keylen);
        len -= cryptlen + keylen;
        data += cryptlen + keylen;
        slist->pstate = 1;
        GOADecrypt(&slist->cryptkey, data, len);
    }
    /* fall through */
    case 1:
        if (len < 6) {
            break;
        }
        slist->mypublicip = *(unsigned int *)data;
        slist->ListCallback(slist, 6, SBNullServer, slist->instance);
        slist->defaultport = *(unsigned short *)(data + 4);
        if (slist->defaultport == 0xffff) {
            if (NTSLengthSB((const char *)data + 6, len - 6) == -1) {
                break;
            }
            slist->lasterror = (const char *)data + 6;
            slist->ListCallback(slist, 5, SBNullServer, slist->instance);
            if (slist->inbuffer == 0) {
                break;
            }
        }
        data += 6;
        len -= 6;
        if ((slist->queryoptions & 2) || slist->defaultport == 0xffff) {
            slist->pstate = 5;
            slist->state = 2;
            break;
        }
        slist->pstate = 2;
        slist->expectedelements = -1;
    /* fall through */
    case 2:
        if (slist->expectedelements == -1) {
            if (len < 1) {
                break;
            }
            slist->expectedelements = data[0];
            slist->keylist = ArrayNew(sizeof(SBKeyInfo), data[0], 0);
            if (slist->keylist == 0) {
                return 5;
            }
            data++;
            len--;
        }
        while (slist->expectedelements > ArrayLength(slist->keylist) && len >= 2) {
            SBKeyInfo key;

            for (i = 0; i < len - 1 && data[1 + i] != 0; i++) {
            }
            if (i >= len - 1) {
                break;
            }
            n = i + 1;
            if (n == -1) {
                break;
            }
            key.type = data[0];
            key.name = SBRefStr(slist, (const char *)data + 1);
            ArrayAppend(slist->keylist, &key);
            data += n + 1;
            len -= n + 1;
        }
        if (slist->expectedelements > ArrayLength(slist->keylist)) {
            break;
        }
        slist->pstate = 3;
        slist->expectedelements = -1;
    /* fall through */
    case 3:
        if (slist->expectedelements == -1) {
            if (len < 1) {
                break;
            }
            slist->expectedelements = data[0];
            data++;
            slist->numpopularvalues = 0;
            len--;
        }
        while (slist->expectedelements > slist->numpopularvalues && len > 0) {
            for (i = 0; i < len && data[i] != 0; i++) {
            }
            if (i >= len) {
                break;
            }
            n = i + 1;
            if (n == -1) {
                break;
            }
            slist->popularvalues[slist->numpopularvalues] = SBRefStr(slist, (const char *)data);
            slist->numpopularvalues++;
            data += n;
            len -= n;
        }
        if (slist->expectedelements > slist->numpopularvalues) {
            break;
        }
        slist->pstate = 4;
    /* fall through */
    case 4:
        if (len < 5) {
            break;
        }
        do {
            n = ProcessServerRecord(slist, data, len);
            if (n == -2) {
                return 5;
            }
            if (n == -1) {
                slist->pstate = 5;
                slist->state = 2;
                len -= 5;
                data += 5;
                slist->ListCallback(slist, 3, SBNullServer, slist->instance);
                break;
            }
            data += n;
            len -= n;
            if (slist->inbuffer == 0) {
                break;
            }
        } while (n != 0);
        break;
    }
    if (slist->inbuffer != 0) {
        if (len != 0) {
            memmove(slist->inbuffer, data, len);
        }
        slist->inbufferlen = len;
    }
    return 0;
}
