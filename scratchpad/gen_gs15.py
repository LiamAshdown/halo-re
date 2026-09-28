exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
H = '#include "sb.h"\n\n'
NULLSRV = 'extern SBServer *SBNullServer; // 0x006a27f8\n'
KEYS = 'extern char *qr2_registered_key_list[0x100]; // 0x00683990\n'


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, H + code.lstrip('\n'), cc=cc)


# ------------------------------------------------------------------ sb_crypt.c
e(0x622ea0, 127, 'keyrand', 'stack state, limit, key, keysize (byte); ECX keypos, ESI rsum: 0 for limit 0; otherwise the smallest all-ones mask covering limit, then rsum = cards[rsum] + key[keypos++] (the key recycles with rsum += keysize), u = rsum & mask -- after 11 tries reduced modulo limit -- until u <= limit.', '''
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
''', cc='stack -> state, limit, key, keysize; ECX -> keypos, ESI -> rsum')
e(0x622f20, 61, 'GOAHashInit', 'rotor 1, ratchet 3, avalanche 5, last plain 7, last cipher 11; cards 255 down to 0.', '''
void GOAHashInit(GOACryptState *state)
{
    int i;

    state->rotor = 1;
    state->ratchet = 3;
    state->avalanche = 5;
    state->last_plain = 7;
    state->last_cipher = 11;
    for (i = 0; i < 0x100; i++) {
        state->cards[i] = (unsigned char)(0xff - i);
    }
}
''')
e(0x622f60, 181, 'GOACryptInit', 'no key: GOAHashInit. Otherwise the cards in order, each from 255 down to 0 swapped with keyrand(i); then rotor = cards[1], ratchet = cards[3], avalanche = cards[5], last plain = cards[7], last cipher = cards[rsum].', '''
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
''')
e(0x623020, 239, 'GOADecryptByte', 'the GameSpy card-shuffle stream cipher, one byte.', '''
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
''')
e(0x623110, 50, 'GOADecrypt', 'GOADecryptByte over the buffer in place.', '''
void GOADecrypt(GOACryptState *state, unsigned char *data, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        data[i] = GOADecryptByte(state, data[i]);
    }
}
''')

# ------------------------------------------------------------------ sb_queryengine.c
e(0x61e340, 70, 'FIFORemove', 'EAX server, EDX fifo: unlinks the server when it is there (1), else 0.', '''
int FIFORemove(SBServer *server, SBServerFIFO *fifo)
{
    SBServer *hold = 0;
    SBServer *it;

    for (it = fifo->first; it != 0; it = it->next) {
        if (it == server) {
            break;
        }
        hold = it;
    }
    if (it == 0) {
        return 0;
    }
    if (hold != 0) {
        hold->next = it->next;
    }
    if (fifo->first == it) {
        fifo->first = it->next;
    }
    if (fifo->last == it) {
        fifo->last = hold;
    }
    fifo->count--;
    return 1;
}
''', cc='EAX -> server, EDX -> fifo')
e(0x61e390, 348, 'QEStartQuery', 'EBX engine, stack server: onto the query list, stamped now, then the query: version 1 -- fe fd 00, the time as request key, then the engine key list with two 0 bytes for a basic query (state 4) or ff ff ff for everything; version 0 -- "\\\\basic\\\\\\\\info\\\\" or "\\\\status\\\\". Sent to the private address when the server shares our public ip and has one (flag 2), else to the public one.', '''
void QEStartQuery(SBQueryEngine *engine, SBServer *server)
{
    unsigned char query[0x100];
    struct sockaddr_in address;
    int len;

    if (engine->querylist.last != 0) {
        engine->querylist.last->next = server;
    }
    engine->querylist.last = server;
    server->next = 0;
    if (engine->querylist.first == 0) {
        engine->querylist.first = server;
    }
    engine->querylist.count++;
    server->updatetime = current_time();
    if (engine->queryversion == 1) {
        *(unsigned long *)(query + 3) = server->updatetime;
        query[0] = 0xfe;
        query[1] = 0xfd;
        query[2] = 0;
        if (server->state & 4) {
            query[7] = (unsigned char)engine->numserverkeys;
            if (engine->numserverkeys > 0) {
                memcpy(query + 8, engine->serverkeys, engine->numserverkeys);
            }
            query[8 + engine->numserverkeys] = 0;
            query[9 + engine->numserverkeys] = 0;
            len = engine->numserverkeys + 10;
        } else {
            query[7] = 0xff;
            query[8] = 0xff;
            query[9] = 0xff;
            len = 10;
        }
    } else if (server->state & 4) {
        memcpy(query, "\\\\basic\\\\\\\\info\\\\", 13);
        len = 13;
    } else {
        memcpy(query, "\\\\status\\\\", 8);
        len = 8;
    }
    address.sin_family = AF_INET;
    if (server->publicip == engine->mypublicip && (server->flags & 2)) {
        address.sin_addr.s_addr = server->privateip;
        address.sin_port = server->privateport;
    } else {
        address.sin_addr.s_addr = server->publicip;
        address.sin_port = server->publicport;
    }
    sendto(engine->querysock, (const char *)query, len, 0, (const struct sockaddr *)&address, 0x10);
}
''', cc='EBX -> engine, stack -> server')
e(0x61e4f0, 81, 'SBQueryEngineInit', 'SocketStartUp; version, max updates, no keys, callback and instance, no public ip, a UDP query socket, empty lists.', '''
void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, SBEngineCallbackFn callback,
    void *instance)
{
    SocketStartUp();
    engine->queryversion = queryversion;
    engine->maxupdates = maxupdates;
    engine->numserverkeys = 0;
    engine->ListCallback = callback;
    engine->instance = instance;
    engine->mypublicip = 0;
    engine->querysock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    engine->pendinglist.last = 0;
    engine->pendinglist.first = 0;
    engine->pendinglist.count = 0;
    engine->querylist.last = 0;
    engine->querylist.first = 0;
    engine->querylist.count = 0;
}
''')
e(0x61e560, 25, 'SBEngineHaltUpdates', 'empties both lists.', '''
void SBEngineHaltUpdates(SBQueryEngine *engine)
{
    engine->pendinglist.last = 0;
    engine->pendinglist.first = 0;
    engine->pendinglist.count = 0;
    engine->querylist.last = 0;
    engine->querylist.first = 0;
    engine->querylist.count = 0;
}
''')
e(0x61e580, 43, 'SBEngineCleanup', 'closes the query socket (-1) and empties both lists.', '''
void SBEngineCleanup(SBQueryEngine *engine)
{
    closesocket(engine->querysock);
    engine->querysock = INVALID_SOCKET;
    engine->pendinglist.last = 0;
    engine->pendinglist.first = 0;
    engine->pendinglist.count = 0;
    engine->querylist.last = 0;
    engine->querylist.first = 0;
    engine->querylist.count = 0;
}
''')
e(0x61e5b0, 128, 'SBQueryEngineUpdateServer', 'clears the pending bits (keeping 0x1c clear) and marks a basic (type 0: 4) or full (1: 8) query; queried at once while below the update limit, else queued at the front or back.', '''
void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer *server, int addfront, int querytype)
{
    server->state &= 0xe3;
    if (querytype == 0) {
        server->state |= 4;
    } else if (querytype == 1) {
        server->state |= 8;
    }
    if (engine->querylist.count < engine->maxupdates) {
        QEStartQuery(engine, server);
        return;
    }
    if (addfront != 0) {
        server->next = engine->pendinglist.first;
        engine->pendinglist.first = server;
        if (engine->pendinglist.last == 0) {
            engine->pendinglist.last = server;
        }
    } else {
        if (engine->pendinglist.last != 0) {
            engine->pendinglist.last->next = server;
        }
        engine->pendinglist.last = server;
        server->next = 0;
        if (engine->pendinglist.first == 0) {
            engine->pendinglist.first = server;
        }
    }
    engine->pendinglist.count++;
}
''')
e(0x61e630, 273, 'ParseSingleQR2Reply', 'EAX data, EBX server, stack engine, len: only a reply of type 0; past the 5-byte header a basic query takes one string per engine key (qr2 key names), a full one parses all keys (then basic and full are set). The pending bits clear, the ping is taken, it leaves the query list and the callback hears success (0).', KEYS + '''
extern int NTSLengthSB(const char *buf, int len);

void ParseSingleQR2Reply(SBQueryEngine *engine, SBServer *server, char *data, int len)
{
    if (data[0] != 0) {
        return;
    }
    len -= 5;
    data += 5;
    if (server->state & 4) {
        int i;

        for (i = 0; i < engine->numserverkeys; i++) {
            int n = NTSLengthSB(data, len);

            if (n < 0) {
                break;
            }
            SBServerAddKeyValue(server, qr2_registered_key_list[engine->serverkeys[i]], data);
            len -= n;
            data += n;
        }
        server->state |= 1;
    } else {
        SBServerParseQR2FullKeysSingle(server, data, len);
        server->state |= 3;
    }
    server->state &= 0xf3;
    server->updatetime = current_time() - server->updatetime;
    FIFORemove(server, &engine->querylist);
    engine->ListCallback(engine, 0, server, engine->instance);
}
''', cc='EAX -> data, EBX -> server, stack -> engine, len')
e(0x61e750, 101, 'ParseSingleGOAReply', 'EBX data, EDI server, stack engine: the key/value pairs are parsed; once the reply carries "\\\\final\\\\" the query is complete (basic, or full), the ping taken, it leaves the query list and the callback hears success.', '''
void ParseSingleGOAReply(SBQueryEngine *engine, SBServer *server, char *data)
{
    int isfinal = strstr(data, "\\\\final\\\\") != 0;

    SBServerParseKeyVals(server, data);
    if (!isfinal) {
        return;
    }
    if (server->state & 4) {
        server->state |= 1;
    } else {
        server->state |= 2;
    }
    server->state &= 0xf3;
    server->updatetime = current_time() - server->updatetime;
    FIFORemove(server, &engine->querylist);
    engine->ListCallback(engine, 0, server, engine->instance);
}
''', cc='EBX -> data, EDI -> server, stack -> engine')
e(0x61e7c0, 232, 'ProcessIncomingReplies', 'ESI engine: every waiting datagram (up to 0x833 bytes, NUL-terminated) goes to the queried server it came from (public address, or the private one for a server behind our own public ip), parsed by query version.', '''
void ProcessIncomingReplies(SBQueryEngine *engine)
{
    char buffer[0x834];
    struct sockaddr_in from;
    int fromlen = 0x10;
    int len;

    while (CanReceiveOnSocket(engine->querysock)) {
        SBServer *server;

        len = recvfrom(engine->querysock, buffer, 0x833, 0, (struct sockaddr *)&from, &fromlen);
        if (len == SOCKET_ERROR) {
            return;
        }
        buffer[len] = 0;
        for (server = engine->querylist.first; server != 0; server = server->next) {
            if ((server->publicip == from.sin_addr.s_addr && server->publicport == from.sin_port) ||
                (server->publicip == engine->mypublicip && (server->flags & 2) &&
                 server->privateip == from.sin_addr.s_addr && server->privateport == from.sin_port)) {
                if (engine->queryversion == 1) {
                    ParseSingleQR2Reply(engine, server, buffer, len);
                } else {
                    ParseSingleGOAReply(engine, server, buffer);
                }
                break;
            }
        }
    }
}
''', cc='ESI -> engine')
e(0x61e8b0, 91, 'TimeoutOldQueries', 'ESI engine: queries older than 2.5 s fail from the front: flag 0x10 set and 0x0c cleared in the flags byte (+0x15), the callback hears failure (1), and it leaves the list.', '''
void TimeoutOldQueries(SBQueryEngine *engine)
{
    unsigned long now = current_time();

    while (engine->querylist.first != 0 && now > engine->querylist.first->updatetime + 2500) {
        engine->querylist.first->flags |= 0x10;
        engine->querylist.first->flags &= 0xf3;
        engine->ListCallback(engine, 1, engine->querylist.first, engine->instance);
        if (engine->querylist.first != 0) {
            engine->querylist.first = engine->querylist.first->next;
            if (engine->querylist.first == 0) {
                engine->querylist.last = 0;
            }
            engine->querylist.count--;
        }
    }
}
''', cc='ESI -> engine')
e(0x61e910, 67, 'QueueNextQueries', 'EAX engine: while below the update limit and anything is pending, the front pending server is queried.', '''
void QueueNextQueries(SBQueryEngine *engine)
{
    while (engine->querylist.count < engine->maxupdates && engine->pendinglist.count > 0) {
        SBServer *server = engine->pendinglist.first;

        if (server != 0) {
            engine->pendinglist.first = server->next;
            if (engine->pendinglist.first == 0) {
                engine->pendinglist.last = 0;
            }
            engine->pendinglist.count--;
        }
        QEStartQuery(engine, server);
    }
}
''', cc='EAX -> engine')
e(0x61e960, 60, 'SBQueryEngineThink', 'with queries out: replies, timeouts, then pending queries; when none are left the callback hears idle (2).', '''
void SBQueryEngineThink(SBQueryEngine *engine)
{
    if (engine->querylist.count == 0) {
        return;
    }
    ProcessIncomingReplies(engine);
    TimeoutOldQueries(engine);
    if (engine->pendinglist.count > 0) {
        QueueNextQueries(engine);
    }
    if (engine->querylist.count == 0) {
        engine->ListCallback(engine, 2, 0, engine->instance);
    }
}
''')
e(0x61e9a0, 24, 'SBQueryEngineAddQueryKey', 'appends a key id while fewer than 20.', '''
void SBQueryEngineAddQueryKey(SBQueryEngine *engine, unsigned char keyid)
{
    if (engine->numserverkeys < 0x14) {
        engine->serverkeys[engine->numserverkeys] = keyid;
        engine->numserverkeys++;
    }
}
''')
e(0x61e9c0, 39, 'SBQueryEngineRemoveServerFromFIFOs', 'out of the query list, else the pending one.', '''
int SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer *server)
{
    if (FIFORemove(server, &engine->querylist)) {
        return 1;
    }
    return FIFORemove(server, &engine->pendinglist);
}
''')
print('ok')
