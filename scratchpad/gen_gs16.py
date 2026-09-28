exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
H = '#include "sb.h"\n\n'
NULLSRV = 'extern SBServer *SBNullServer; // 0x006a27f8\n'


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, H + code.lstrip('\n'), cc=cc)


e(0x61ea20, 298, 'ServerListConnect', 'ESI list: the master is SBOverrideMasterServer when set, else "s1.ms01.hosthpc.com" (used as the sprintf format), port 28910; an unresolvable name gives 2. A TCP socket is made when there is none (1 on failure); a failed connect closes it again (3); else 0.', '''
extern char *SBOverrideMasterServer; // 0x006a3278

int ServerListConnect(SBServerList *slist)
{
    char hostname[0x80];
    struct sockaddr_in address;

    if (SBOverrideMasterServer != 0) {
        strcpy(hostname, SBOverrideMasterServer);
    } else {
        sprintf(hostname, "s1.ms01.hosthpc.com");
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(28910);
    address.sin_addr.s_addr = inet_addr(hostname);
    if (address.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *host = gethostbyname(hostname);

        if (host == 0) {
            return 2;
        }
        address.sin_addr.s_addr = *(unsigned int *)host->h_addr_list[0];
    }
    if (slist->slsocket == INVALID_SOCKET) {
        slist->slsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (slist->slsocket == INVALID_SOCKET) {
            return 1;
        }
    }
    if (connect(slist->slsocket, (const struct sockaddr *)&address, 0x10) != 0) {
        closesocket(slist->slsocket);
        slist->slsocket = INVALID_SOCKET;
        return 3;
    }
    return 0;
}
''', cc='ESI -> slist')
e(0x61eb50, 59, 'BufferAddNTS', 'EAX string (NULL: ""), EDX write pointer, EBX length: appends it with its NUL and advances both.', '''
void BufferAddNTS(char **buffer, int *len, const char *str)
{
    int n;

    if (str == 0) {
        str = "";
    }
    n = (int)strlen(str) + 1;
    memcpy(*buffer, str, n);
    *len += n;
    *buffer += n;
}
''', cc='EAX -> str, EDX -> buffer, EBX -> len')
e(0x61eb90, 140, 'SetupListChallenge', 'the 8-byte list challenge at +0x6c: printable rand bytes whose parities follow the running check ((i ^ prev ^ first) & 1) ^ (prev < first) ^ (first < 0x4f) with SIGNED byte compares, like gti2GetChallenge.', '''
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
''')
e(0x61ec20, 113, 'InitCryptKey', 'EAX list, EBX key, stack length: folds the key into the challenge -- challenge[(signed)(secret[i %% len] * i) %% 8] ^= challenge[i & 7] ^ key[i] (a negative index is kept) -- and keys the list cipher with the 8 challenge bytes.'.replace('%%', '%'), '''
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
''', cc='EAX -> slist, EBX -> key, stack -> keylen')
e(0x61eca0, 87, 'FullRulesPresent', 'ECX length, EDX data: whether the buffer holds whole key/value string pairs up to an empty key.', '''
int FullRulesPresent(const char *data, int len)
{
    int i;

    if (len > 0) {
        do {
            if (*data == 0) {
                return 1;
            }
            for (i = 0; i < len && data[i] != 0; i++) {
            }
            if (i >= len) {
                return 0;
            }
            i++;
            data += i;
            len -= i;
            if (len <= 0) {
                return 0;
            }
            for (i = 0; i < len && data[i] != 0; i++) {
            }
            if (i >= len) {
                return 0;
            }
            i++;
            len -= i;
            data += i;
        } while (len > 0);
    }
    if (len == 0) {
        return 0;
    }
    return *data == 0;
}
''', cc='ECX -> len, EDX -> data')
e(0x61ed00, 145, 'AllKeysPresent', 'EAX length, ECX data, stack list: whether a value for every key of the key list fits: a string is a popular-value index byte, or 0xff and a NUL-terminated string; a byte 1; a short 2; other types fail.', '''
int AllKeysPresent(SBServerList *slist, const unsigned char *data, int len)
{
    int count = ArrayLength(slist->keylist);
    int k;

    for (k = 0; k < count; k++) {
        int type = ((SBKeyInfo *)ArrayNth(slist->keylist, k))->type;

        if (type == 0) {
            unsigned char index;

            if (len < 1) {
                return 0;
            }
            index = *data++;
            len--;
            if (index == 0xff) {
                int i = 0;

                if (len <= 0) {
                    return 0;
                }
                do {
                    if (data[i++] == 0) {
                        break;
                    }
                    if (i >= len) {
                        return 0;
                    }
                } while (1);
                if (i == -1) {
                    return 0;
                }
                data += i;
                len -= i;
            }
        } else if (type == 1) {
            data++;
            len--;
        } else if (type == 2) {
            data += 2;
            len -= 2;
        } else {
            return 0;
        }
        if (len < 0) {
            return 0;
        }
    }
    return 1;
}
''', cc='EAX -> len, ECX -> data, stack -> slist')
e(0x61eda0, 50, 'ParseServerIPPort', 'EAX data, EDX length, EBX ip out, ESI port out, stack list: with 5 bytes, the ip after the flags byte; the port that follows when flag 0x10 says so (and 2 more bytes are there), else the list  default port.', '''
void ParseServerIPPort(SBServerList *slist, const unsigned char *data, int len, unsigned int *ip,
    unsigned short *port)
{
    unsigned char flags;

    if (len < 5) {
        return;
    }
    flags = data[0];
    *ip = *(const unsigned int *)(data + 1);
    if (flags & 0x10) {
        if (len - 5 >= 2) {
            *port = *(const unsigned short *)(data + 5);
        }
    } else {
        *port = slist->defaultport;
    }
}
''', cc='EAX -> data, EDX -> len, EBX -> ip, ESI -> port, stack -> slist')
e(0x61ede0, 518, 'ParseServer', 'EAX length, ECX data, EBX server, stack list, use popular values: the flags byte goes to the server; past the address (and port with 0x10) come the private ip (2) and port (0x20, else the default), the ICMP ip (8); with 0x40 one value per key-list key (short keys are network order; strings may be popular-value indices when allowed, 0xff meaning an inline string) and the basic-keys state; with 0x80 inline key/value pairs to an empty key and the full-keys state. The bytes consumed.', '''
int ParseServer(SBServerList *slist, SBServer *server, unsigned char *data, int len, int usepopularlist)
{
    unsigned char flags = data[0];
    int original = len;
    unsigned int privateip;
    unsigned short privateport;

    SBServerSetFlags(server, flags);
    data += 5;
    len -= 5;
    if (flags & 0x10) {
        data += 2;
        len -= 2;
    }
    if (flags & 2) {
        privateip = *(unsigned int *)data;
        data += 4;
        len -= 4;
    } else {
        privateip = 0;
    }
    if (flags & 0x20) {
        privateport = *(unsigned short *)data;
        data += 2;
        len -= 2;
    } else {
        privateport = slist->defaultport;
    }
    SBServerSetPrivateAddr(server, privateip, privateport);
    if (flags & 8) {
        unsigned int icmpip = *(unsigned int *)data;

        data += 4;
        len -= 4;
        SBServerSetICMPIP(server, icmpip);
    }
    if (flags & 0x40) {
        int count = ArrayLength(slist->keylist);
        int k;

        for (k = 0; k < count; k++) {
            SBKeyInfo *key = (SBKeyInfo *)ArrayNth(slist->keylist, k);

            if (key->type == 2) {
                SBServerAddIntKeyValue(server, key->name, ntohs(*(unsigned short *)data));
                data += 2;
                len -= 2;
            } else if (key->type == 1) {
                SBServerAddIntKeyValue(server, key->name, *data);
                data++;
                len--;
            } else if (key->type == 0) {
                int n;

                if (usepopularlist != 0) {
                    unsigned char index = *data++;

                    len--;
                    if (index != 0xff) {
                        SBServerAddKeyValue(server, key->name, slist->popularvalues[index]);
                        continue;
                    }
                }
                SBServerAddKeyValue(server, key->name, (const char *)data);
                n = (int)strlen((const char *)data) + 1;
                data += n;
                len -= n;
            }
        }
        SBServerSetState(server, (unsigned char)(SBServerGetState(server) | 1));
    }
    if (flags & 0x80) {
        while (*data != 0 && len > 0) {
            const char *keyname = (const char *)data;
            int n = (int)strlen(keyname) + 1;

            data += n;
            len -= n;
            SBServerAddKeyValue(server, keyname, (const char *)data);
            n = (int)strlen((const char *)data) + 1;
            data += n;
            len -= n;
        }
        len--;
        SBServerSetState(server, (unsigned char)(SBServerGetState(server) | 2));
    }
    return original - len;
}
''', cc='EAX -> len, ECX -> data, EBX -> server, stack -> slist, usepopularlist')
REL = '''        key.str = %s;
        ref = (SBRefString *)TableLookup(SBRefStrHash(slist), &key);
        if (ref != 0 && --ref->refcount == 0) {
            TableRemove(SBRefStrHash(slist), &key);
        }
'''
e(0x61f3d0, 135, 'FreePopularValues', 'ESI list: releases every popular value string (the ref-string count, removed at 0) and forgets them.', '''
void FreePopularValues(SBServerList *slist)
{
    SBRefString key;
    SBRefString *ref;
    int i;

    for (i = 0; i < slist->numpopularvalues; i++) {
''' + REL % 'slist->popularvalues[i]' + '''    }
    slist->numpopularvalues = 0;
}
''', cc='ESI -> slist')
e(0x61f460, 147, 'FreeKeyList', 'ESI list: releases every key name string and frees the key list.', '''
void FreeKeyList(SBServerList *slist)
{
    SBRefString key;
    SBRefString *ref;
    int i;

    if (slist->keylist == 0) {
        return;
    }
    for (i = 0; i < ArrayLength(slist->keylist); i++) {
''' + REL % '((SBKeyInfo *)ArrayNth(slist->keylist, i))->name' + '''    }
    ArrayFree(slist->keylist);
    slist->keylist = 0;
}
''', cc='ESI -> slist')
e(0x61f500, 90, 'SBServerListDisconnect', 'frees the input buffer, closes the master socket, back to disconnected (1), key list and popular values released.', '''
void SBServerListDisconnect(SBServerList *slist)
{
    if (slist->inbuffer != 0) {
        free(slist->inbuffer);
    }
    slist->inbuffer = 0;
    slist->inbufferlen = 0;
    if (slist->slsocket != INVALID_SOCKET) {
        closesocket(slist->slsocket);
    }
    slist->slsocket = INVALID_SOCKET;
    slist->state = 1;
    FreeKeyList(slist);
    slist->expectedelements = -1;
    FreePopularValues(slist);
}
''')
e(0x61f560, 51, 'SBServerListCleanup', 'disconnect, clear, drop the ref-string table, free the server array.', '''
void SBServerListCleanup(SBServerList *slist)
{
    SBServerListDisconnect(slist);
    SBServerListClear(slist);
    SBRefStrHashCleanup();
    if (slist->servers != 0) {
        ArrayFree(slist->servers);
    }
    slist->servers = 0;
}
''')
e(0x61f5a0, 244, 'ProcessServerRecord', 'EAX data, stack length, list: 0 until the whole record (header by flags, key values, rules) is there; -1 for the all-ones end-of-list address; otherwise the server is allocated (-2 when that fails), parsed with popular values and appended; the bytes used.', NULLSRV + '''
int ProcessServerRecord(SBServerList *slist, unsigned char *data, int len)
{
    unsigned char flags;
    int header;
    unsigned int ip;
    unsigned short port = (unsigned short)len;
    SBServer *server;
    int used;

    if (len < 1) {
        return 0;
    }
    flags = data[0];
    header = (flags & 2) ? 9 : 5;
    if (flags & 8) {
        header += 4;
    }
    if (flags & 0x10) {
        header += 2;
    }
    if (flags & 0x20) {
        header += 2;
    }
    if (len < header) {
        return 0;
    }
    if ((flags & 0x40) && !AllKeysPresent(slist, data + header, len - header)) {
        return 0;
    }
    if ((flags & 0x80) && !FullRulesPresent((const char *)data + header, len - header)) {
        return 0;
    }
    if (*(unsigned int *)(data + 1) == 0xffffffff) {
        return -1;
    }
    ParseServerIPPort(slist, data, len, &ip, &port);
    server = SBAllocServer(slist, ip, port);
    if (SBIsNullServer(server)) {
        return -2;
    }
    used = ParseServer(slist, server, data, len, 1);
    SBServerListAppendServer(slist, server);
    return used;
}
''', cc='EAX -> data, stack -> len, slist')
e(0x61f6a0, 860, 'ProcessMainListData', 'ESI list: the master reply state machine over the input buffer. 0: the crypt header (lengths xor 0xec / 0xea), the key folds into the challenge and the rest is decrypted. 1: our public ip (callback 6) and the default port; 0xffff is an error string (callback 5). Without servers wanted (option 2) or after an error it is connected (2). 2: the key list (type byte, name). 3: the popular values. 4: server records until the end marker (connected, callback 3 initial list complete). Out of memory: 5. Whatever is left moves to the buffer start; 0.', NULLSRV + '''
extern int NTSLengthSB(const char *buf, int len);

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
''', cc='ESI -> slist')
e(0x61fa10, 203, 'ProcessPushKeyList', 'EAX data, ECX list, stack length: a pushed key list replaces the old one (count, then type byte and name per key): 5 out of memory, 4 when truncated, else 0.', '''
int ProcessPushKeyList(SBServerList *slist, unsigned char *data, int len)
{
    int count = data[0];
    int k;

    data++;
    len--;
    if (slist->keylist != 0) {
        FreeKeyList(slist);
    }
    slist->keylist = ArrayNew(sizeof(SBKeyInfo), count, 0);
    if (slist->keylist == 0) {
        return 5;
    }
    for (k = 0; k < count; k++) {
        SBKeyInfo key;
        int i;
        int n;

        if (len < 2) {
            return 4;
        }
        for (i = 0; i < len - 1 && data[1 + i] != 0; i++) {
        }
        if (i >= len - 1) {
            return 4;
        }
        n = i + 1;
        if (n == -1) {
            return 4;
        }
        key.type = data[0];
        key.name = SBRefStr(slist, (const char *)data + 1);
        ArrayAppend(slist->keylist, &key);
        len += -1 - n;
        data += n + 1;
    }
    return 0;
}
''', cc='EAX -> data, ECX -> slist, stack -> len')
e(0x61fae0, 197, 'ProcessPushServer', 'stack data, length; ESI list: a pushed server record (4 when under 5 bytes or unparsable) updates the known server at that address or a new one (5 when it cannot be allocated), appended when new; the callback hears updated (1); 0.', '''
int ProcessPushServer(SBServerList *slist, unsigned char *data, int len)
{
    unsigned char flags;
    unsigned int ip;
    unsigned short port;
    int index;
    SBServer *server;

    if (len < 5) {
        return 4;
    }
    flags = data[0];
    ip = *(unsigned int *)(data + 1);
    if (flags & 0x10) {
        if (len - 5 < 2) {
            port = (unsigned short)len;
        } else {
            port = *(unsigned short *)(data + 5);
        }
    } else {
        port = slist->defaultport;
    }
    index = SBServerListFindServer(slist, ip, port);
    if (index == -1) {
        server = SBAllocServer(slist, ip, port);
        if (SBIsNullServer(server)) {
            return 5;
        }
    } else {
        server = *(SBServer **)ArrayNth(slist->servers, index);
    }
    if (ParseServer(slist, server, data, len, 0) < 0) {
        return 4;
    }
    if (index == -1) {
        SBServerListAppendServer(slist, server);
    }
    slist->ListCallback(slist, 1, server, slist->instance);
    return 0;
}
''', cc='stack -> data, len; ESI -> slist')
e(0x61fbb0, 330, 'ProcessLanData', 'every LAN reply (up to 0x1f3 bytes) from an unknown address adds a server (flags 0x11) and the callback hears added (0); 5 when one cannot be allocated. After 2 s the LAN socket closes and the list is disconnected (1).', '''
int ProcessLanData(SBServerList *slist)
{
    char buffer[0x1f8];
    struct sockaddr_in from;
    int fromlen = 0x10;

    while (CanReceiveOnSocket(slist->slsocket)) {
        if (recvfrom(slist->slsocket, buffer, 0x1f3, 0, (struct sockaddr *)&from, &fromlen) != SOCKET_ERROR &&
            SBServerListFindServer(slist, from.sin_addr.s_addr, from.sin_port) == -1) {
            SBServer *server = SBAllocServer(slist, from.sin_addr.s_addr, from.sin_port);

            if (SBIsNullServer(server)) {
                return 5;
            }
            SBServerSetFlags(server, 0x11);
            ArrayAppend(slist->servers, &server);
            slist->ListCallback(slist, 0, server, slist->instance);
        }
    }
    if (current_time() - slist->lanstarttime > 2000) {
        closesocket(slist->slsocket);
        slist->slsocket = INVALID_SOCKET;
        slist->state = 1;
    }
    return 0;
}
''')
e(0x61fd00, 537, 'SBServerListConnectAndQuery', '(list, field list, filter, options, max servers): 6 when either string is over 256 chars; connects (its error); sends the list request: length (network order), 0 1 3, the from-game version, the two game names, the 8-byte challenge, the filter and fields, the options (network order), the source ip for option 8 and the max for option 0x80. A failed send disconnects (3). Then the main list state with a 4 KB input buffer (5 when out of memory); 0.', '''
int SBServerListConnectAndQuery(SBServerList *slist, const char *fieldList, const char *serverFilter,
    int updateOptions, int maxServers)
{
    char request[0x2f0];
    char *write;
    int len;
    int error;

    if (fieldList == 0) {
        fieldList = "";
    }
    if (serverFilter == 0) {
        serverFilter = "";
    }
    if (strlen(fieldList) > 0x100 || strlen(serverFilter) > 0x100) {
        return 6;
    }
    error = ServerListConnect(slist);
    if (error != 0) {
        return error;
    }
    slist->queryoptions = updateOptions;
    SetupListChallenge(slist);
    *(int *)(request + 5) = slist->fromgamever;
    request[2] = 0;
    request[3] = 1;
    request[4] = 3;
    len = 9;
    write = request + 9;
    BufferAddNTS(&write, &len, slist->queryforgamename);
    BufferAddNTS(&write, &len, slist->queryfromgamename);
    memcpy(write, slist->mychallenge, 8);
    len += 8;
    write += 8;
    BufferAddNTS(&write, &len, serverFilter);
    BufferAddNTS(&write, &len, fieldList);
    *(unsigned int *)write = htonl((unsigned int)updateOptions);
    len += 4;
    write += 4;
    if (slist->queryoptions & 8) {
        *(unsigned int *)write = slist->srcip;
        len += 4;
        write += 4;
    }
    if (slist->queryoptions & 0x80) {
        *(int *)write = maxServers;
        len += 4;
    }
    *(unsigned short *)request = htons((unsigned short)len);
    if (send(slist->slsocket, request, len, 0) <= 0) {
        SBServerListDisconnect(slist);
        return 3;
    }
    slist->state = 3;
    slist->pstate = 0;
    if (slist->inbuffer == 0) {
        slist->inbuffer = (unsigned char *)malloc(0x1000);
        if (slist->inbuffer == 0) {
            return 5;
        }
        slist->inbufferlen = 0;
    }
    return 0;
}
''')
e(0x61ff20, 310, 'SBServerListGetLANList', '(list, start port, end port, query version): disconnects a connected list, opens a broadcast UDP socket (1 on failure) and to every port from start to end (at most 500 more) broadcasts fe fd 02 00 00 00 00 00 (version 1) or "\\\\echo\\\\test"; the list is LAN browsing (0) from now.', '''
int SBServerListGetLANList(SBServerList *slist, unsigned short startSearchPort, unsigned short endSearchPort,
    int queryversion)
{
    unsigned char query[8];
    BOOL broadcast = 1;
    struct sockaddr_in address;
    unsigned short port;

    query[0] = 0xfe;
    query[1] = 0xfd;
    query[2] = 2;
    query[3] = 0;
    query[4] = 0;
    query[5] = 0;
    query[6] = 0;
    query[7] = 0;
    if (slist->state != 1) {
        SBServerListDisconnect(slist);
    }
    slist->slsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (slist->slsocket == INVALID_SOCKET ||
        setsockopt(slist->slsocket, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, 4) != 0) {
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = 0xffffffff;
    if ((int)endSearchPort - (int)startSearchPort > 500) {
        endSearchPort = (unsigned short)(startSearchPort + 500);
    }
    if (startSearchPort <= endSearchPort) {
        port = startSearchPort;
        do {
            address.sin_port = htons(port);
            if (queryversion == 1) {
                sendto(slist->slsocket, (const char *)query, 8, 0, (const struct sockaddr *)&address, 0x10);
            } else {
                sendto(slist->slsocket, "\\\\echo\\\\test", 10, 0, (const struct sockaddr *)&address, 0x10);
            }
            port++;
        } while (port <= endSearchPort);
    }
    slist->state = 0;
    slist->lanstarttime = current_time();
    return 0;
}
''')
e(0x620060, 297, 'ProcessAdHocData', 'EAX list: whole messages from a connected master (length, type, data; over 4 KB is an error 4): 1 a pushed key list, 2 a pushed server, 3 echoed straight back (3 when that fails), 4 a server deleted by address (under 6 bytes: 4). Each is cut from the buffer; an error disconnects with callback 4 (disconnected).', NULLSRV + '''
int ProcessAdHocData(SBServerList *slist)
{
    int error = 0;

    if (slist->inbufferlen < 3) {
        return 0;
    }
    do {
        unsigned short msglen = ntohs(*(unsigned short *)slist->inbuffer);
        unsigned char *data = slist->inbuffer;

        if (msglen > 0x1000) {
            error = 4;
            break;
        }
        if (slist->inbufferlen < (int)msglen) {
            return 0;
        }
        switch ((signed char)data[2]) {
        case 1:
            error = ProcessPushKeyList(slist, data + 3, msglen - 3);
            break;
        case 2:
            error = ProcessPushServer(slist, data + 3, msglen - 3);
            break;
        case 3:
            if (send(slist->slsocket, (const char *)data, msglen, 0) <= 0) {
                return 3;
            }
            break;
        case 4:
            if (msglen - 3 < 6) {
                error = 4;
            } else {
                int index = SBServerListFindServer(slist, *(unsigned int *)(data + 3), *(unsigned short *)(data + 7));

                if (index != -1) {
                    SBServerListRemoveAt(slist, index);
                }
                error = 0;
            }
            break;
        }
        slist->inbufferlen -= msglen;
        if (slist->inbufferlen != 0 && slist->inbuffer != 0) {
            memmove(slist->inbuffer, slist->inbuffer + msglen, slist->inbufferlen);
        }
        if (error != 0) {
            break;
        }
    } while (slist->inbufferlen >= 3);
    if (error == 0) {
        return 0;
    }
    slist->ListCallback(slist, 4, SBNullServer, slist->instance);
    SBServerListDisconnect(slist);
    return error;
}
''', cc='EAX -> slist')
e(0x6201a0, 196, 'SBListThinkConnected', 'EAX list: with data waiting, recv into the rest of the 4 KB buffer (closed or failed: callback 4 and disconnect, 3); once past the crypt header (connected, or a main-list state past 0) the new bytes are decrypted; the main list is processed (its error returned), a connected list with data handles ad-hoc messages.', NULLSRV + '''
int SBListThinkConnected(SBServerList *slist)
{
    int oldlen;
    int received;

    if (!CanReceiveOnSocket(slist->slsocket)) {
        return 0;
    }
    oldlen = slist->inbufferlen;
    received = recv(slist->slsocket, (char *)slist->inbuffer + oldlen, 0x1000 - oldlen, 0);
    if (received == SOCKET_ERROR || received == 0) {
        slist->ListCallback(slist, 4, SBNullServer, slist->instance);
        SBServerListDisconnect(slist);
        return 3;
    }
    slist->inbufferlen += received;
    if (slist->state == 2 || slist->pstate > 0) {
        GOADecrypt(&slist->cryptkey, slist->inbuffer + oldlen, slist->inbufferlen - oldlen);
    }
    if (slist->state == 3) {
        int error = ProcessMainListData(slist);

        if (error != 0) {
            return error;
        }
    }
    if (slist->state == 2 && slist->inbufferlen > 0) {
        return ProcessAdHocData(slist);
    }
    return 0;
}
''', cc='EAX -> slist')
e(0x620270, 53, 'SBListThink', 'frees the dead servers; LAN browsing reads LAN replies, connected or main-list states read the master, others do nothing.', '''
int SBListThink(SBServerList *slist)
{
    SBFreeDeadList(slist);
    if (slist->state == 0) {
        return ProcessLanData(slist);
    }
    if (slist->state > 1 && slist->state <= 3) {
        return SBListThinkConnected(slist);
    }
    return 0;
}
''')
e(0x6202b0, 205, 'SendWithRetry', 'EAX list, stack data, length: sends to the master; after a failed first try the list is dropped (as SBServerListDisconnect) and reconnected with option 2 (no servers) -- a failed reconnect reports disconnected (4), disconnects and returns its error -- and sent once more (3 if that fails too).', NULLSRV + '''
int SendWithRetry(SBServerList *slist, const char *data, int len)
{
    int retry = 1;

    for (;;) {
        int error;

        retry--;
        if (send(slist->slsocket, data, len, 0) > 0) {
            return 0;
        }
        if (retry < 0) {
            return 3;
        }
        if (slist->inbuffer != 0) {
            free(slist->inbuffer);
        }
        slist->inbuffer = 0;
        slist->inbufferlen = 0;
        if (slist->slsocket != INVALID_SOCKET) {
            closesocket(slist->slsocket);
        }
        slist->slsocket = INVALID_SOCKET;
        slist->state = 1;
        FreeKeyList(slist);
        slist->expectedelements = -1;
        FreePopularValues(slist);
        error = SBServerListConnectAndQuery(slist, 0, 0, 2, 0);
        if (error != 0) {
            slist->ListCallback(slist, 4, SBNullServer, slist->instance);
            SBServerListDisconnect(slist);
            return error;
        }
    }
}
''', cc='EAX -> slist, stack -> data, len')
CONN = '''    if (slist->state == 1) {
        SBServerListConnectAndQuery(slist, 0, 0, 2, 0);
        if (slist->state == 1) {
            return 3;
        }
    }
'''
e(0x620380, 131, 'SBRequestServerUpdate', 'connects (option 2) when disconnected (3 when that fails), then asks the master to have the server at (ip, port) report: length 9, type 1, ip, port.', '''
int SBRequestServerUpdate(SBServerList *slist, unsigned int ip, unsigned short port)
{
    unsigned char message[9];

''' + CONN + '''    *(unsigned short *)message = htons(9);
    message[2] = 1;
    *(unsigned int *)(message + 3) = ip;
    *(unsigned short *)(message + 7) = port;
    return SendWithRetry(slist, (const char *)message, 9);
}
''')
e(0x620410, 175, 'SBSendMessageToServer', 'connects (option 2) when disconnected (3 when that fails), then forwards data to the server through the master: length 9 + len, type 2, ip, port, then the data (3 when that send fails).', '''
int SBSendMessageToServer(SBServerList *slist, unsigned int ip, unsigned short port, const char *data, int len)
{
    unsigned char header[9];
    int error;

''' + CONN + '''    *(unsigned short *)header = htons((unsigned short)(len + 9));
    header[2] = 2;
    *(unsigned int *)(header + 3) = ip;
    *(unsigned short *)(header + 7) = port;
    error = SendWithRetry(slist, (const char *)header, 9);
    if (error != 0) {
        return error;
    }
    return send(slist->slsocket, data, len, 0) >= 0 ? 0 : 3;
}
''')
e(0x6204c0, 96, 'SBSendNatNegotiateCookieToServer', 'the NAT negotiation magic fd fc 1e 66 6a b2 and the cookie (network order) through SBSendMessageToServer.', '''
int SBSendNatNegotiateCookieToServer(SBServerList *slist, unsigned int ip, unsigned short port, int cookie)
{
    unsigned char message[10];

    message[0] = 0xfd;
    message[1] = 0xfc;
    message[2] = 0x1e;
    message[3] = 0x66;
    message[4] = 0x6a;
    message[5] = 0xb2;
    *(unsigned int *)(message + 6) = htonl((unsigned int)cookie);
    return SBSendMessageToServer(slist, ip, port, (const char *)message, 10);
}
''')
print('ok')
