// qr2_init_socketA  (GameSpy SDK in halo.exe; no C existed)
// address 0x616340, size 619 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616340..0x6165aa: fills the record (the static one for NULL, else a
//   malloc(0x108) stored through the pointer): socket, port, game name, secret key, public flag, natneg flag, the six
//   callbacks, user data, random instance key, empty message key ring; caches up to five local addresses once
//   (0x006a26d8, count 0x006a27f0); a public server resolves its master (the name at 0x00723640, else
//   "s1.master.hosthpc.com") on port 27900 -- 3 when that fails. 0 otherwise.
// blam-cc: cdecl

#include "gamespy.h"

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
