exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())

Q = '''
typedef struct qr2_buffer_s {
    char buffer[0x800];               // 0x000
    int len;                          // 0x800
} qr2_buffer_s;

typedef struct qr2_keybuffer_s {
    unsigned char keys[0x100];        // 0x000
    int numkeys;                      // 0x100
} qr2_keybuffer_s;

typedef struct qr2_implementation_s {
    SOCKET hbsock;                    // 0x000
    char gamename[0x40];              // 0x004
    char secret_key[0x40];            // 0x044
    unsigned char instance_key[4];    // 0x084
    void (*server_key_callback)(int keyid, qr2_buffer_s *outbuf, void *userdata);             // 0x088
    void (*player_key_callback)(int keyid, int index, qr2_buffer_s *outbuf, void *userdata);  // 0x08c
    void (*team_key_callback)(int keyid, int index, qr2_buffer_s *outbuf, void *userdata);    // 0x090
    void (*key_list_callback)(int keytype, qr2_keybuffer_s *keybuffer, void *userdata);       // 0x094
    int (*playerteam_count_callback)(int keytype, void *userdata);                          // 0x098
    void (*adderror_callback)(int error, char *errmsg, void *userdata);                     // 0x09c
    void (*nn_callback)(int cookie, void *userdata);                                        // 0x0a0
    void (*cm_callback)(char *data, int len, void *userdata);                               // 0x0a4
    unsigned long lastheartbeat;      // 0x0a8
    unsigned long lastka;             // 0x0ac
    int userstatechangerequested;     // 0x0b0
    int listed_state;                 // 0x0b4
    int qport;                        // 0x0b8
    int read_socket;                  // 0x0bc
    int nat_negotiate;                // 0x0c0
    struct sockaddr_in hbaddr;        // 0x0c4
    void (*cdkeyprocess)(char *buf, int len, struct sockaddr *fromaddr);                    // 0x0d4
    int client_message_keys[10];      // 0x0d8
    int cur_message_key;              // 0x100
    void *udata;                      // 0x104
} qr2_implementation_s;               // size 0x108

extern qr2_implementation_s *current_rec;         // 0x00683940
extern qr2_implementation_s static_rec;           // 0x00683838
extern char *qr2_registered_key_list[0x100];      // 0x00683990

void qr2_buffer_add(qr2_buffer_s *outbuf, const char *value);
void qr2_parse_queryA(qr2_implementation_s *qrec, char *query, int len, struct sockaddr *sender);
'''

def e(addr, size, name, note, code, extra='', cc='cdecl'):
    emit(addr, size, name, note, Q + extra + '\n' + code, cc=cc)

e(0x615600, 189, 'B64Encode', 'EAX output, EDX input, stack length: 3 bytes (zero past the end) to 4 characters of A-Z a-z 0-9 + /, no padding characters, NUL-terminated.', '''
void B64Encode(char *output, const unsigned char *input, int len)
{
    int consumed = 0;

    while (consumed < len) {
        unsigned char in[3];
        unsigned char out[4];
        int i;

        for (i = 0; i < 3; i++, consumed++) {
            in[i] = consumed < len ? *input++ : 0;
        }
        out[0] = in[0] >> 2;
        out[1] = (unsigned char)(((in[0] & 3) << 4) + (in[1] >> 4));
        out[2] = (unsigned char)(((in[1] & 0xf) << 2) + (in[2] >> 6));
        out[3] = in[2] & 0x3f;
        for (i = 0; i < 4; i++) {
            unsigned char c = out[i];

            if (c < 26) {
                c += 'A';
            } else if (c < 52) {
                c += 'a' - 26;
            } else if (c < 62) {
                c -= 4;
            } else if (c == 62) {
                c = '+';
            } else {
                c = c == 63 ? '/' : 0;
            }
            *output++ = (char)c;
        }
    }
    *output = 0;
}
''', cc='EAX -> output, EDX -> input, stack -> len')
e(0x6156c0, 327, 'gs_encrypt', 'ECX key length, stack key, buffer, length: the GameSpy RC4 variant -- the usual key schedule, then per byte x += data + 1 (not x += 1), y += s[x], swap, data ^= s[s[x] + s[y]].', '''
void gs_encrypt(const unsigned char *key, int key_len, unsigned char *buffer, int len)
{
    unsigned char state[0x100];
    unsigned char x = 0;
    unsigned char y = 0;
    int k = 0;
    short n;
    int i;

    for (i = 0; i < 0x100; i++) {
        state[i] = (unsigned char)i;
    }
    for (i = 0; i < 0x100; i++) {
        unsigned char t = state[i];

        y = (unsigned char)(key[k] + y + t);
        k = (unsigned char)((k + 1) % key_len);
        state[i] = state[y];
        state[y] = t;
    }
    y = 0;
    for (n = 0; n < len; n++) {
        unsigned char c = buffer[n];
        unsigned char t;

        x = (unsigned char)(c + x + 1);
        y = (unsigned char)(state[x] + y);
        t = state[x];
        state[x] = state[y];
        state[y] = t;
        buffer[n] = state[(unsigned char)(state[x] + state[y])] ^ c;
    }
}
''', cc='ECX -> key_len, stack -> key, buffer, len')
e(0x615810, 158, 'compute_challenge_response', 'EBX challenge length, EDI output buffer, stack record, challenge: a NUL-terminated challenge of 1..0x41 bytes is copied, encrypted with the secret key (gs_encrypt) and appended base64-encoded with its NUL.', '''
extern void gs_encrypt(const unsigned char *key, int key_len, unsigned char *buffer, int len);
extern void B64Encode(char *output, const unsigned char *input, int len);

void compute_challenge_response(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, const char *challenge, int len)
{
    char encrypted[0x44];

    if (len < 1 || len > 0x41 || challenge[len - 1] != 0) {
        return;
    }
    strcpy(encrypted, challenge);
    gs_encrypt((const unsigned char *)qrec->secret_key, (int)strlen(qrec->secret_key), (unsigned char *)encrypted, len - 1);
    B64Encode(outbuf->buffer + outbuf->len, (const unsigned char *)encrypted, len - 1);
    outbuf->len += (int)strlen(outbuf->buffer + outbuf->len) + 1;
}
''', cc='EBX -> len, EDI -> outbuf, stack -> qrec, challenge')
e(0x6158c0, 695, 'qr_build_partial_query_reply', 'EBX record, stack buffer, key type, key count, keys: for player/team types the (network order) count from the count callback first; 0xff means every key of the type (from the key list callback), whose names (qr2_registered_key_list, else "unknown") are listed and NUL-terminated -- server keys with their values inline; then the values of every key for each of the count rows (server / player / team key callbacks), an empty string for a key that wrote nothing.', '''
void qr_build_partial_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype, int numkeys,
    unsigned char *keys)
{
    qr2_keybuffer_s keybuffer;
    int count;
    int i;
    int k;

    keybuffer.numkeys = 0;
    if (numkeys == 0) {
        return;
    }
    if (keytype == 1 || keytype == 2) {
        if ((unsigned int)(0x800 - outbuf->len) < 2) {
            return;
        }
        count = qrec->playerteam_count_callback(keytype, qrec->udata);
        *(unsigned short *)(outbuf->buffer + outbuf->len) = htons((unsigned short)count);
        outbuf->len += 2;
    } else {
        count = 1;
    }
    if (numkeys == 0xff) {
        qrec->key_list_callback(keytype, &keybuffer, qrec->udata);
        for (k = 0; k < keybuffer.numkeys; k++) {
            const char *name = qr2_registered_key_list[keybuffer.keys[k]];

            qr2_buffer_add(outbuf, name != 0 ? name : "unknown");
            if (keytype == 0) {
                int before = outbuf->len;

                qrec->server_key_callback(keybuffer.keys[k], outbuf, qrec->udata);
                if (before == outbuf->len) {
                    qr2_buffer_add(outbuf, "");
                }
            }
        }
        if (0x800 - outbuf->len < 1) {
            return;
        }
        outbuf->buffer[outbuf->len] = 0;
        outbuf->len++;
        keys = keybuffer.keys;
        numkeys = keybuffer.numkeys;
        if (keytype == 0) {
            return;
        }
    }
    for (i = 0; i < count; i++) {
        for (k = 0; k < numkeys; k++) {
            int before = outbuf->len;

            if (keytype == 0) {
                qrec->server_key_callback(keys[k], outbuf, qrec->udata);
            } else if (keytype == 1) {
                qrec->player_key_callback(keys[k], i, outbuf, qrec->udata);
            } else if (keytype == 2) {
                qrec->team_key_callback(keys[k], i, outbuf, qrec->udata);
            }
            if (before == outbuf->len) {
                qr2_buffer_add(outbuf, "");
            }
        }
    }
}
''', cc='EBX -> qrec, stack -> outbuf, keytype, numkeys, keys')
e(0x615b80, 54, 'qr_build_query_reply', 'EDX record, ESI buffer, ECX / EAX server key count and keys, stack player and team counts and keys: the server, player and team sections.', '''
extern void qr_build_partial_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype, int numkeys,
    unsigned char *keys);

void qr_build_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int serverkeycount, unsigned char *serverkeys,
    int playerkeycount, unsigned char *playerkeys, int teamkeycount, unsigned char *teamkeys)
{
    qr_build_partial_query_reply(qrec, outbuf, 0, serverkeycount, serverkeys);
    qr_build_partial_query_reply(qrec, outbuf, 1, playerkeycount, playerkeys);
    qr_build_partial_query_reply(qrec, outbuf, 2, teamkeycount, teamkeys);
}
''', cc='EDX -> qrec, ESI -> outbuf, ECX -> serverkeycount, EAX -> serverkeys, stack -> the rest')
e(0x615bc0, 147, 'parse_query', 'ECX data, EDX length, stack record, buffer: three (count, keys) lists -- a count of 0 or 0xff has no key bytes -- then the reply; too short a query is ignored.', '''
extern void qr_build_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int serverkeycount,
    unsigned char *serverkeys, int playerkeycount, unsigned char *playerkeys, int teamkeycount, unsigned char *teamkeys);

void parse_query(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, unsigned char *data, int len)
{
    unsigned char serverkeycount;
    unsigned char playerkeycount;
    unsigned char teamkeycount;
    unsigned char *serverkeys = 0;
    unsigned char *playerkeys = 0;
    unsigned char *teamkeys = 0;

    if (len < 3) {
        return;
    }
    serverkeycount = *data++;
    len--;
    if (serverkeycount != 0 && serverkeycount != 0xff) {
        serverkeys = data;
        data += serverkeycount;
        len -= serverkeycount;
    }
    if (len < 2) {
        return;
    }
    playerkeycount = *data++;
    len--;
    if (playerkeycount != 0 && playerkeycount != 0xff) {
        playerkeys = data;
        data += playerkeycount;
        len -= playerkeycount;
    }
    if (len < 1) {
        return;
    }
    teamkeycount = *data;
    len--;
    if (teamkeycount != 0 && teamkeycount != 0xff) {
        teamkeys = data + 1;
        len -= teamkeycount;
    }
    if (len < 0) {
        return;
    }
    qr_build_query_reply(qrec, outbuf, serverkeycount, serverkeys, playerkeycount, playerkeys, teamkeycount, teamkeys);
}
''', cc='ECX -> data, EDX -> len, stack -> qrec, outbuf')
e(0x615c60, 687, 'qr_build_partial_old_query_reply', 'the old "\\\\key\\\\value" protocol: every key of the type (key list callback) as name\\\\value\\\\, player / team keys once per row as "<name><row>" ("%s%d"); a key that wrote nothing gets an empty value.', '''
static void set_trailing_backslash(qr2_buffer_s *outbuf)
{
    outbuf->buffer[outbuf->len - 1] = '\\\\';
}

void qr_build_partial_old_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype)
{
    qr2_keybuffer_s keybuffer;
    int count;
    int k;

    keybuffer.numkeys = 0;
    count = (keytype == 1 || keytype == 2) ? qrec->playerteam_count_callback(keytype, qrec->udata) : 1;
    qrec->key_list_callback(keytype, &keybuffer, qrec->udata);
    for (k = 0; k < keybuffer.numkeys; k++) {
        const char *name = qr2_registered_key_list[keybuffer.keys[k]];

        if (name == 0) {
            name = "unknown";
        }
        if (keytype == 0) {
            int before;

            qr2_buffer_add(outbuf, name);
            set_trailing_backslash(outbuf);
            before = outbuf->len;
            qrec->server_key_callback(keybuffer.keys[k], outbuf, qrec->udata);
            if (before == outbuf->len) {
                qr2_buffer_add(outbuf, "");
            }
            set_trailing_backslash(outbuf);
        } else {
            int i;

            for (i = 0; i < count; i++) {
                char indexed[0x80];
                int before;

                sprintf(indexed, "%s%d", name, i);
                qr2_buffer_add(outbuf, indexed);
                set_trailing_backslash(outbuf);
                before = outbuf->len;
                if (keytype == 1) {
                    qrec->player_key_callback(keybuffer.keys[k], i, outbuf, qrec->udata);
                } else if (keytype == 2) {
                    qrec->team_key_callback(keybuffer.keys[k], i, outbuf, qrec->udata);
                }
                if (before == outbuf->len) {
                    qr2_buffer_add(outbuf, "");
                }
                set_trailing_backslash(outbuf);
            }
        }
    }
}
''')
e(0x615f10, 68, 'qr_build_old_query_reply', 'EDI record, ESI buffer: "\\\\" then the server, player and team keys old-style, then "final\\\\\\\\queryid\\\\1.1" (0x0064e3e4) without its NUL.', '''
extern void qr_build_partial_old_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype);

void qr_build_old_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf)
{
    outbuf->len = 1;
    outbuf->buffer[0] = '\\\\';
    qr_build_partial_old_query_reply(qrec, outbuf, 0);
    qr_build_partial_old_query_reply(qrec, outbuf, 1);
    qr_build_partial_old_query_reply(qrec, outbuf, 2);
    qr2_buffer_add(outbuf, "final\\\\\\\\queryid\\\\1.1");
    outbuf->len--;
}
''', cc='EDI -> qrec, ESI -> outbuf')
e(0x615f60, 173, 'handle_client_message', 'EDX length, EDI record, stack data: a message of at least 10 bytes starting with the natneg magic goes to the natneg callback (its network order cookie at +6), anything else to the client message callback.', '''
void handle_client_message(qr2_implementation_s *qrec, char *data, int len)
{
    static const unsigned char magic[6] = { 0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2 };

    if (len >= 10 && memcmp(data, magic, 6) == 0) {
        if (qrec->nn_callback != 0) {
            qrec->nn_callback((int)ntohl(*(unsigned int *)(data + 6)), qrec->udata);
        }
        return;
    }
    if (qrec->cm_callback != 0) {
        qrec->cm_callback(data, len, qrec->udata);
    }
}
''', cc='EDX -> len, EDI -> qrec, stack -> data')
e(0x616010, 58, 'qr2_got_message_key', 'ESI record, EDI key: 1 when the key is one of the last ten client message keys, else it is remembered (ring of ten) and 0.', '''
int qr2_got_message_key(qr2_implementation_s *qrec, int key)
{
    int i;

    for (i = 0; i < 10; i++) {
        if (qrec->client_message_keys[i] == key) {
            return 1;
        }
    }
    qrec->cur_message_key = (qrec->cur_message_key + 1) % 10;
    qrec->client_message_keys[qrec->cur_message_key] = key;
    return 0;
}
''', cc='ESI -> qrec, EDI -> key')
e(0x616050, 611, 'qr2_parse_queryA', 'a query for the record (the current one for NULL): ";" goes to the cd key handler; "\\\\" gets the old-style reply; 0xfe 0xfd packets of at least 7 bytes (type, instance key, data) -- query (0) builds the reply, challenge (1) the challenge response, echo (2) echoes up to 32 bytes as type 5, add-error (4) with our instance key reports once to the add-error callback without a reply, client message (6) with our instance key is acknowledged (type 7 with its 4 byte key) and handled once per key; the reply (type byte, instance key, data) goes back to the sender.', '''
extern void parse_query(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, unsigned char *data, int len);
extern void compute_challenge_response(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, const char *challenge, int len);
extern void qr_build_old_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf);
extern int qr2_got_message_key(qr2_implementation_s *qrec, int key);
extern void handle_client_message(qr2_implementation_s *qrec, char *data, int len);

void qr2_parse_queryA(qr2_implementation_s *qrec, char *query, int len, struct sockaddr *sender)
{
    qr2_buffer_s reply;
    char *data;
    int i;

    reply.len = 0;
    if (qrec == 0) {
        qrec = current_rec;
    }
    if (query[0] == ';') {
        if (qrec->cdkeyprocess != 0) {
            qrec->cdkeyprocess(query, len, sender);
        }
        return;
    }
    if (query[0] == '\\\\') {
        qr_build_old_query_reply(qrec, &reply);
        sendto(qrec->hbsock, reply.buffer, reply.len, 0, sender, 0x10);
        return;
    }
    if (len < 7 || (unsigned char)query[0] != 0xfe || (unsigned char)query[1] != 0xfd) {
        return;
    }
    if (qrec->userstatechangerequested > 0) {
        qrec->userstatechangerequested = 0;
    }
    reply.buffer[0] = query[2];
    memcpy(reply.buffer + 1, query + 3, 4);
    len -= 7;
    data = query + 7;
    reply.len = 5;
    switch ((signed char)query[2]) {
    case 0:
        parse_query(qrec, &reply, (unsigned char *)data, len);
        break;
    case 1:
        compute_challenge_response(qrec, &reply, data, len);
        break;
    case 2:
        if (len > 0x20) {
            len = 0x20;
        }
        reply.buffer[0] = 5;
        memcpy(reply.buffer + 5, data, len);
        reply.len += len;
        break;
    case 4:
        if (qrec->userstatechangerequested == -1) {
            return;
        }
        for (i = 0; i < 4; i++) {
            if (query[i + 3] != (char)qrec->instance_key[i]) {
                return;
            }
        }
        if (len < 2) {
            return;
        }
        qrec->userstatechangerequested = -1;
        qrec->adderror_callback(data[0], data + 1, qrec->udata);
        return;
    case 6:
        for (i = 0; i < 4; i++) {
            if (query[i + 3] != (char)qrec->instance_key[i]) {
                return;
            }
        }
        if (len < 4) {
            return;
        }
        memcpy(reply.buffer + 5, data, 4);
        reply.buffer[0] = 7;
        reply.len = 9;
        if (!qr2_got_message_key(qrec, *(int *)data)) {
            handle_client_message(qrec, data + 4, len - 4);
        }
        break;
    default:
        return;
    }
    sendto(qrec->hbsock, reply.buffer, reply.len, 0, sender, 0x10);
}
''')
e(0x6162d0, 100, 'send_keepalive', 'ESI record: a keep-alive (type 8, instance key) to the master; remembers the time.', '''
void send_keepalive(qr2_implementation_s *qrec)
{
    qr2_buffer_s packet;

    packet.buffer[0] = 8;
    memcpy(packet.buffer + 1, qrec->instance_key, 4);
    packet.len = 5;
    sendto(qrec->hbsock, packet.buffer, 5, 0, (const struct sockaddr *)&qrec->hbaddr, 0x10);
    qrec->lastka = current_time();
}
''', cc='ESI -> qrec')
e(0x616340, 619, 'qr2_init_socketA', 'fills the record (the static one for NULL, else a malloc(0x108) stored through the pointer): socket, port, game name, secret key, public flag, natneg flag, the six callbacks, user data, random instance key, empty message key ring; caches up to five local addresses once (0x006a26d8, count 0x006a27f0); a public server resolves its master (the name at 0x00723640, else "s1.master.hosthpc.com") on port 27900 -- 3 when that fails. 0 otherwise.', '''
extern char qr2_hostname[];              // 0x00723640
extern unsigned int qr2_local_ips[5];    // 0x006a26d8
extern int qr2_local_ip_count;           // 0x006a27f0
extern struct hostent *getlocalhost(void);

int qr2_init_socketA(qr2_implementation_s **qrec_out, SOCKET s, int boundport, const char *gamename, const char *secret_key,
    int ispublic, int natnegotiate, void *server_key_callback, void *player_key_callback, void *team_key_callback,
    void *key_list_callback, void *playerteam_count_callback, void *adderror_callback, void *userdata)
{
    qr2_implementation_s *qrec;
    int i;

    if (qrec_out == 0) {
        qrec = &static_rec;
    } else {
        qrec = (qr2_implementation_s *)malloc(sizeof(qr2_implementation_s));
        *qrec_out = qrec;
    }
    srand(current_time());
    strcpy(qrec->gamename, gamename);
    strcpy(qrec->secret_key, secret_key);
    qrec->qport = boundport;
    qrec->hbsock = s;
    qrec->udata = userdata;
    qrec->server_key_callback = (void (*)(int, qr2_buffer_s *, void *))server_key_callback;
    qrec->player_key_callback = (void (*)(int, int, qr2_buffer_s *, void *))player_key_callback;
    qrec->team_key_callback = (void (*)(int, int, qr2_buffer_s *, void *))team_key_callback;
    qrec->key_list_callback = (void (*)(int, qr2_keybuffer_s *, void *))key_list_callback;
    qrec->lastheartbeat = 0;
    qrec->lastka = 0;
    qrec->userstatechangerequested = 1;
    qrec->playerteam_count_callback = (int (*)(int, void *))playerteam_count_callback;
    qrec->adderror_callback = (void (*)(int, char *, void *))adderror_callback;
    qrec->nn_callback = 0;
    qrec->cm_callback = 0;
    qrec->cdkeyprocess = 0;
    qrec->listed_state = ispublic;
    qrec->read_socket = 0;
    qrec->nat_negotiate = natnegotiate;
    for (i = 0; i < 4; i++) {
        qrec->instance_key[i] = (unsigned char)(rand() % 0xff);
    }
    for (i = 0; i < 10; i++) {
        qrec->client_message_keys[i] = -1;
    }
    qrec->cur_message_key = 0;
    if (qr2_local_ip_count == 0) {
        struct hostent *host = getlocalhost();

        if (host != 0) {
            for (qr2_local_ip_count = 0; qr2_local_ip_count < 5; qr2_local_ip_count++) {
                unsigned int *address = (unsigned int *)host->h_addr_list[qr2_local_ip_count];

                if (address == 0) {
                    break;
                }
                qr2_local_ips[qr2_local_ip_count] = *address;
            }
        }
    }
    if (ispublic != 0) {
        char default_name[0x40];
        const char *name;

        if (qr2_hostname[0] == 0) {
            sprintf(default_name, "s1.master.hosthpc.com");
            name = default_name;
        } else {
            name = qr2_hostname;
        }
        qrec->hbaddr.sin_family = AF_INET;
        qrec->hbaddr.sin_port = htons(0x6cfc);
        qrec->hbaddr.sin_addr.s_addr = name != 0 ? inet_addr(name) : 0;
        if (qrec->hbaddr.sin_addr.s_addr == INADDR_NONE && memcmp(name, "255.255.255.255", 0x10) != 0) {
            struct hostent *host = gethostbyname(name);

            if (host == 0) {
                return 3;
            }
            qrec->hbaddr.sin_addr.s_addr = *(unsigned int *)host->h_addr_list[0];
        }
    }
    return 0;
}
''')
e(0x6165b0, 137, 'qr2_check_queries', 'ESI record: with read_socket set, drains the socket (0xff byte datagrams into 0x006a26f0, NUL-terminated) through qr2_parse_queryA.', '''
extern char qr2_receive_buffer[0x100]; // 0x006a26f0

void qr2_check_queries(qr2_implementation_s *qrec)
{
    struct sockaddr sender;
    int sender_length = 0x10;

    if (qrec->read_socket == 0) {
        return;
    }
    while (CanReceiveOnSocket(qrec->hbsock)) {
        int length = recvfrom(qrec->hbsock, qr2_receive_buffer, 0xff, 0, &sender, &sender_length);

        if (length != SOCKET_ERROR) {
            qr2_receive_buffer[length] = 0;
            qr2_parse_queryA(qrec, qr2_receive_buffer, length, &sender);
        }
    }
}
''', cc='ESI -> qrec')
print('ok')
