// gs_encrypt  (GameSpy SDK in halo.exe; no C existed)
// address 0x6156c0, size 327 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6156c0..0x615806: ECX key length, stack key, buffer, length: the GameSpy RC4
//   variant -- the usual key schedule, then per byte x += data + 1 (not x += 1), y += s[x], swap, data ^= s[s[x] +
//   s[y]].
// blam-cc: ECX -> key_len, stack -> key, buffer, len

#include "gamespy.h"
#include "fn_gamespy.h"

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
