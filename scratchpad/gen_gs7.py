exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())

# gcdkeys.c (the CD key server side). The per-game table, clients and globals live at their original addresses.
G = '''
typedef struct gcd_client {
    int local_id;                 // 0x00
    char hash[0x24];              // 0x04 the first 32 characters of the response
    int skey;                     // 0x28
    unsigned int ip;              // 0x2c
    unsigned long sent_time;      // 0x30
    int tries;                    // 0x34
    int state;                    // 0x38 0 pending, 1 authorized, 2 rejected, 3 done
    void *instance;               // 0x3c
    void (*callback)(int game_id, int local_id, int authenticated, const char *message, void *instance); // 0x40
    char *message;                // 0x44
    char *request;                // 0x48
    int request_length;           // 0x4c
} gcd_client;                     // size 0x50

typedef struct gcd_client_node {
    gcd_client *client;           // 0x00
    struct gcd_client_node *next; // 0x04
    struct gcd_client_node *prev; // 0x08
} gcd_client_node;

typedef struct gcd_game {
    int game_id;                  // 0x00
    gcd_client_node sentinel;     // 0x04 only .next (0x08, the list head) is used; nodes' prev reach it
} gcd_game;                       // size 0x10

extern gcd_game gcd_games[4];            // 0x00723200
extern int gcd_game_count;               // 0x006a2e64
extern int gcd_no_network;               // 0x006a2e68 1 when initialized without the qr2 socket
extern unsigned short gcd_own_socket;    // 0x006a2e60 0xffff: the qr2 socket is shared
extern SOCKET gcd_socket;                // 0x00683988
extern struct sockaddr_in gcd_master_address; // 0x006a2c10
extern char gcd_xor_key[8];              // 0x006a2800 "gamespy"

void gs_xcode_buf(char *buf, int len);
const char *gcd_value_for_key(const char *buf, const char *key);
gcd_client *gcd_find_client_by_hash(const char *hash, int skey);
void gcd_send_disconnect(gcd_client *client, gcd_game *game);
void gcd_send_auth_request(gcd_game *game, gcd_client *client, const char *challenge, const char *response);
'''

def e(addr, size, name, note, code, extra=''):
    emit(addr, size, name, note, G + extra + '\n' + code)

e(0x61a7b0, 123, 'gcd_init_common', 'at most four games: the next table entry gets the game id and no clients; srand(GetTickCount()) and the xor key "gamespy". -1 when full, else 0.', '''
int gcd_init_common(int game_id)
{
    int index = gcd_game_count;

    if (gcd_game_count > 3) {
        return -1;
    }
    gcd_game_count++;
    gcd_games[index].game_id = game_id;
    gcd_games[index].sentinel.next = 0;
    gcd_games[index].sentinel.prev = 0;
    gcd_games[index].sentinel.client = 0;
    srand(GetTickCount());
    memcpy(gcd_xor_key, "gamespy", 8);
    return 0;
}
''')
e(0x61a830, 171, 'gcd_find_client_by_hash', 'EDX hash, stack skey: the first client of any game with this hash and (skey -1 or) this skey, or NULL.', '''
gcd_client *gcd_find_client_by_hash(const char *hash, int skey)
{
    int i;

    for (i = 0; i < gcd_game_count; i++) {
        gcd_client_node *node;

        for (node = gcd_games[i].sentinel.next; node != 0; node = node->next) {
            if (strcmp(hash, node->client->hash) == 0 && (skey == -1 || node->client->skey == skey)) {
                return node->client;
            }
        }
    }
    return 0;
}
''', extra='// blam-cc of gcd_find_client_by_hash: EDX hash, stack skey\n')
e(0x61a8e0, 308, 'gcd_value_for_key', 'the value after "\\\\key\\\\" in the buffer, at most 200 characters up to the next backslash, in one of two alternating static 0x100 byte buffers (0x006a2c20, index 0x006a280c; statics here); "" when the key is missing.', '''
static char value_buffers[2][0x100]; // 0x006a2c20
static int value_index;              // 0x006a280c

const char *gcd_value_for_key(const char *buf, const char *key)
{
    char search[0x100];
    const char *found;
    char *out;
    int count = 0;

    value_index ^= 1;
    strcpy(search, "\\\\");
    strcat(search, key);
    strcat(search, "\\\\");
    found = strstr(buf, search);
    if (found == 0) {
        return "";
    }
    found += strlen(search);
    out = value_buffers[value_index];
    while (*found != 0 && *found != '\\\\' && count < 200) {
        *out++ = *found++;
        count++;
    }
    *out = 0;
    return value_buffers[value_index];
}
''')
e(0x61aa20, 44, 'gs_xcode_buf', 'ESI buffer, EDI length: xors the buffer with the key "gamespy", cycling.', '''
void gs_xcode_buf(char *buf, int len)
{
    const char *key = gcd_xor_key;
    int i;

    for (i = 0; i < len; i++) {
        buf[i] ^= *key++;
        if (*key == 0) {
            key = gcd_xor_key;
        }
    }
}
''', extra='// blam-cc of gs_xcode_buf: ESI buf, EDI len\n')
e(0x61aab0, 254, 'gcd_process_auth_reply', 'an "uok" (authorized) / "unok" (rejected) reply: the client with the reply\'s "cd" hash and "skey" that is not finished becomes authorized (1), or rejected (2) with the reply\'s "errmsg" kept.', '''
void gcd_process_auth_reply(const char *buf, int authorized)
{
    int skey = atol(gcd_value_for_key(buf, "skey"));
    char hash[0x21];
    gcd_client *client;

    strncpy(hash, gcd_value_for_key(buf, "cd"), 0x20);
    hash[0x20] = 0;
    client = gcd_find_client_by_hash(hash, skey);
    if (client == 0 || client->skey != skey || client->state == 3) {
        return;
    }
    if (authorized != 0) {
        client->state = 1;
        return;
    }
    client->state = 2;
    client->message = goastrdup(gcd_value_for_key(buf, "errmsg"));
}
''')
e(0x61abb0, 305, 'gcd_send_user_count', 'the "\\\\ucount\\\\%d" reply (xored) to the asker: the client count of the game named by "pid", or of the first game when "pid" is empty.', '''
void gcd_send_user_count(const char *buf, const struct sockaddr *to)
{
    char out[0x40];
    const char *pid = gcd_value_for_key(buf, "pid");
    gcd_game *game = 0;
    int count = 0;
    int length;

    if (pid[0] == 0 && gcd_game_count > 0) {
        game = &gcd_games[0];
    } else {
        int game_id = atol(pid);
        int i;

        for (i = 0; i < gcd_game_count; i++) {
            if (gcd_games[i].game_id == game_id) {
                game = &gcd_games[i];
                break;
            }
        }
    }
    if (game != 0) {
        gcd_client_node *node;

        for (node = game->sentinel.next; node != 0; node = node->next) {
            count++;
        }
    }
    length = sprintf(out, "\\\\ucount\\\\%d", count);
    gs_xcode_buf(out, length);
    sendto(gcd_socket, out, length, 0, to, 0x10);
}
''')
e(0x61acf0, 369, 'gcd_send_is_online', 'ECX buffer, stack address: "\\\\uon\\\\\\\\skey\\\\%d" when the client with the buffer\'s "cd" hash is finished (state 3), else "\\\\uoff\\\\\\\\skey\\\\%d", with the buffer\'s skey, xored and sent back.', '''
void gcd_send_is_online(const char *buf, const struct sockaddr *to)
{
    char out[0x40];
    int skey = atol(gcd_value_for_key(buf, "skey"));
    gcd_client *client = gcd_find_client_by_hash(gcd_value_for_key(buf, "cd"), -1);
    int length;

    if (client != 0 && client->state == 3) {
        length = sprintf(out, "\\\\uon\\\\\\\\skey\\\\%d", skey);
    } else {
        length = sprintf(out, "\\\\uoff\\\\\\\\skey\\\\%d", skey);
    }
    gs_xcode_buf(out, length);
    sendto(gcd_socket, out, length, 0, to, 0x10);
}
''', extra='// blam-cc of gcd_send_is_online: ECX buf, stack to\n')
e(0x61ae70, 243, 'gcd_send_disconnect', 'ECX client, stack game: unless without network, "\\\\disc\\\\\\\\pid\\\\%d\\\\cd\\\\%s\\\\ip\\\\%d" (game id, hash, ip), xored, to the master.', '''
void gcd_send_disconnect(gcd_client *client, gcd_game *game)
{
    char out[0x200];
    int length;

    if (gcd_no_network == 1) {
        return;
    }
    length = sprintf(out, "\\\\disc\\\\\\\\pid\\\\%d\\\\cd\\\\%s\\\\ip\\\\%d", game->game_id, client->hash, client->ip);
    gs_xcode_buf(out, length);
    sendto(gcd_socket, out, length, 0, (const struct sockaddr *)&gcd_master_address, 0x10);
}
''', extra='// blam-cc of gcd_send_disconnect: ECX client, stack game\n')
e(0x61af70, 409, 'gcd_send_auth_request', 'the client goes pending with a new skey ((GetTickCount() ^ rand()) & 0x3fff), the send time and one try; unless without network "\\\\auth\\\\\\\\pid\\\\%d\\\\ch\\\\%s\\\\resp\\\\%s\\\\ip\\\\%d\\\\skey\\\\%d", xored, goes to the master and a copy is kept for resends.', '''
void gcd_send_auth_request(gcd_game *game, gcd_client *client, const char *challenge, const char *response)
{
    char out[0x200];
    int length;

    client->state = 0;
    client->skey = (int)((GetTickCount() ^ (unsigned int)rand()) & 0x3fff);
    client->sent_time = GetTickCount();
    client->tries = 1;
    if (gcd_no_network == 1) {
        return;
    }
    length = sprintf(out, "\\\\auth\\\\\\\\pid\\\\%d\\\\ch\\\\%s\\\\resp\\\\%s\\\\ip\\\\%d\\\\skey\\\\%d", game->game_id, challenge, response,
        client->ip, client->skey);
    gs_xcode_buf(out, length);
    sendto(gcd_socket, out, length, 0, (const struct sockaddr *)&gcd_master_address, 0x10);
    client->request = (char *)malloc(length);
    memmove(client->request, out, length);
    client->request_length = length;
}
''')
e(0x61b110, 567, 'gcd_authenticate_user', 'for a known game: a response shorter than 72 characters is "Bad CD Key" and a hash already in use is "CD Key in use"; the new 0x50 byte client (hash = first 32 characters) is appended to the game\'s list; with a message it is rejected (state 2), otherwise the auth request goes out.', '''
void gcd_authenticate_user(int game_id, int local_id, unsigned int user_ip, const char *challenge, const char *response,
    void *callback, void *instance)
{
    char hash[0x21];
    char *message;
    gcd_game *game = 0;
    gcd_client_node *node;
    gcd_client_node *last;
    gcd_client *client;
    int i;

    for (i = 0; i < gcd_game_count; i++) {
        if (gcd_games[i].game_id == game_id) {
            game = &gcd_games[i];
            break;
        }
    }
    if (game == 0) {
        return;
    }
    strncpy(hash, response, 0x20);
    hash[0x20] = 0;
    message = strlen(response) < 0x48 ? goastrdup("Bad CD Key") : 0;
    for (node = game->sentinel.next; node != 0; node = node->next) {
        if (strcmp(hash, node->client->hash) == 0) {
            message = goastrdup("CD Key in use");
            break;
        }
    }
    client = (gcd_client *)malloc(sizeof(gcd_client));
    client->local_id = local_id;
    client->instance = instance;
    client->ip = user_ip;
    client->callback = (void (*)(int, int, int, const char *, void *))callback;
    client->message = 0;
    client->request = 0;
    strcpy(client->hash, hash);
    node = (gcd_client_node *)malloc(sizeof(gcd_client_node));
    node->client = client;
    for (last = &game->sentinel; last->next != 0; last = last->next) {
    }
    last->next = node;
    node->prev = last;
    node->next = 0;
    if (message != 0) {
        client->message = message;
        client->state = 2;
        return;
    }
    gcd_send_auth_request(game, client, challenge, response);
}
''')
FREE_NODE = '''
// unlinks a client node and frees it with its client (the disconnect message goes out first)
static void remove_client(gcd_game *game, gcd_client_node *node)
{
    gcd_send_disconnect(node->client, game);
    node->prev->next = node->next;
    if (node->next != 0) {
        node->next->prev = node->prev;
    }
    if (node->client->request != 0) {
        free(node->client->request);
    }
    free(node->client);
    free(node);
}
'''
e(0x61b350, 154, 'gcd_disconnect_user', 'removes the game\'s client with this local id (the master is told).', FREE_NODE + '''
void gcd_disconnect_user(int game_id, int local_id)
{
    int i;

    for (i = 0; i < gcd_game_count; i++) {
        if (gcd_games[i].game_id == game_id) {
            gcd_client_node *node;

            for (node = gcd_games[i].sentinel.next; node != 0; node = node->next) {
                if (node->client->local_id == local_id) {
                    remove_client(&gcd_games[i], node);
                    return;
                }
            }
            return;
        }
    }
}
''')
e(0x61b3f0, 141, 'gcd_disconnect_all', 'removes every client of the game (the master is told for each).', FREE_NODE + '''
void gcd_disconnect_all(int game_id)
{
    int i;

    for (i = 0; i < gcd_game_count; i++) {
        if (gcd_games[i].game_id == game_id) {
            while (gcd_games[i].sentinel.next != 0) {
                remove_client(&gcd_games[i], gcd_games[i].sentinel.next);
            }
            return;
        }
    }
}
''')
e(0x61b480, 585, 'gcd_process', 'the qr2 cd-key packet handler (installed at qrec +0xd4): decodes the buffer, takes the command between the first two backslashes (at most 32 characters) and dispatches "uok" / "unok" (auth replies), "ison" (online query) and "ucount" (user count).', '''
extern void gcd_process_auth_reply(const char *buf, int authorized);
extern void gcd_send_user_count(const char *buf, const struct sockaddr *to);
extern void gcd_send_is_online(const char *buf, const struct sockaddr *to);

void gcd_process(char *buf, int len, const struct sockaddr *from)
{
    char command[0x21];
    char *end;

    gs_xcode_buf(buf, len);
    command[0] = 0;
    if (buf[0] != '\\\\') {
        return;
    }
    end = strchr(buf + 1, '\\\\');
    if (end != 0 && end - buf < 0x21) {
        strncpy(command, buf + 1, end - buf - 1);
        command[end - buf - 1] = 0;
    }
    if (command[0] == 0) {
        return;
    }
    if (strcmp(command, "uok") == 0) {
        gcd_process_auth_reply(buf, 1);
    } else if (strcmp(command, "unok") == 0) {
        gcd_process_auth_reply(buf, 0);
    } else if (strcmp(command, "ison") == 0) {
        gcd_send_is_online(buf, from);
    } else if (strcmp(command, "ucount") == 0) {
        gcd_send_user_count(buf, from);
    }
}
''')
e(0x61b6d0, 138, 'gcd_init_qr2', 'sharing a query/report record\'s socket (default record 0x00683838): without the flag there is no network (0x006a2e68 = 1); otherwise the record\'s socket, gcd_process as its cd-key callback (+0xd4) and the master address (AF_INET, port 29910, the record\'s master ip +0xc8). Then gcd_init_common.', '''
extern unsigned char qr2_default_record[]; // 0x00683838
extern void gcd_process(char *buf, int len, const struct sockaddr *from);
extern int gcd_init_common(int game_id);

void gcd_init_qr2(void *qrec, int game_id, int use_network)
{
    gcd_no_network = use_network == 0;
    if (qrec == 0) {
        qrec = qr2_default_record;
    }
    gcd_own_socket = 0xffff;
    if (gcd_no_network == 0) {
        gcd_socket = *(SOCKET *)qrec;
        *(void **)((char *)qrec + 0xd4) = (void *)gcd_process;
        memset(&gcd_master_address, 0, sizeof(gcd_master_address));
        gcd_master_address.sin_family = AF_INET;
        gcd_master_address.sin_port = htons(0x74d6);
        gcd_master_address.sin_addr.s_addr = *(unsigned int *)((char *)qrec + 0xc8);
    }
    gcd_init_common(game_id);
}
''')
e(0x61b760, 122, 'gcd_shutdown', 'disconnects every game\'s clients; with its own socket (never, as initialized: the marker is 0xffff) it would close it and WSACleanup; no games and no socket afterwards.', '''
extern void gcd_disconnect_all(int game_id);

void gcd_shutdown(void)
{
    int i;

    for (i = 0; i < gcd_game_count; i++) {
        gcd_disconnect_all(gcd_games[i].game_id);
    }
    if (gcd_no_network == 0 && gcd_own_socket != 0xffff && gcd_socket != INVALID_SOCKET) {
        closesocket(gcd_socket);
        WSACleanup();
    }
    gcd_socket = INVALID_SOCKET;
    gcd_game_count = 0;
}
''')
e(0x61b7e0, 842, 'gcd_think', 'with its own socket (never, see gcd_shutdown) it drains replies through gcd_process. Then per client: pending ones resend every 2 s (up to three tries, then they count as authorized with "Validation Timeout"); authorized ones call back (1, "Validated") and finish (3); rejected ones are unlinked, call back (0, their message or "") and are freed.', '''
extern void gcd_process(char *buf, int len, const struct sockaddr *from);
extern char gcd_receive_buffer[0x400]; // 0x006a2810

void gcd_think(void)
{
    int i;

    if (gcd_no_network == 0 && gcd_own_socket != 0xffff) {
        fd_set set;
        struct timeval timeout = { 0, 0 };
        struct sockaddr from;
        int from_length = 0x10;
        int result;

        set.fd_array[0] = gcd_socket;
        set.fd_count = 1;
        result = select(0x40, &set, 0, 0, &timeout);
        while (result != SOCKET_ERROR && result != 0) {
            result = recvfrom(gcd_socket, gcd_receive_buffer, 0x3ff, 0, &from, &from_length);
            if (result != SOCKET_ERROR) {
                gcd_receive_buffer[result] = 0;
                gcd_process(gcd_receive_buffer, result, &from);
            }
            result = select(0x40, &set, 0, 0, &timeout);
        }
    }
    for (i = 0; i < gcd_game_count; i++) {
        gcd_client_node *node;

        for (node = gcd_games[i].sentinel.next; node != 0; node = node->next) {
            gcd_client *client = node->client;

            if (client->state == 0) {
                if (GetTickCount() < client->sent_time + 2000) {
                    continue;
                }
                if (client->tries <= 2) {
                    client->sent_time = GetTickCount();
                    client->tries++;
                    if (gcd_no_network != 1) {
                        sendto(gcd_socket, client->request, client->request_length, 0,
                            (const struct sockaddr *)&gcd_master_address, 0x10);
                    }
                    continue;
                }
            }
            if (client->state == 0 || client->state == 1) {
                client->callback(gcd_games[i].game_id, client->local_id, 1,
                    client->state == 1 ? "Validated" : "Validation Timeout", client->instance);
                node->client->state = 3;
                free(node->client->request);
                node->client->request = 0;
            } else if (client->state == 2) {
                gcd_client_node *prev = node->prev;

                node->prev->next = node->next;
                if (node->next != 0) {
                    node->next->prev = node->prev;
                }
                client->callback(gcd_games[i].game_id, client->local_id, 0,
                    client->message != 0 ? client->message : "", client->instance);
                free(node->client->request);
                if (node->client->message != 0) {
                    free(node->client->message);
                }
                free(node->client);
                free(node);
                node = prev;
            }
        }
    }
}
''')
print('ok')
