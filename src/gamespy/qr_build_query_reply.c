// qr_build_query_reply  (GameSpy SDK in halo.exe; no C existed)
// address 0x615b80, size 54 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615b80..0x615bb5: EDX record, ESI buffer, ECX / EAX server key count and keys,
//   stack player and team counts and keys: the server, player and team sections.
// blam-cc: EDX -> qrec, ESI -> outbuf, ECX -> serverkeycount, EAX -> serverkeys, stack -> the rest

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


extern void qr_build_partial_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype, int numkeys,
    unsigned char *keys);

void qr_build_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int serverkeycount, unsigned char *serverkeys,
    int playerkeycount, unsigned char *playerkeys, int teamkeycount, unsigned char *teamkeys)
{
    qr_build_partial_query_reply(qrec, outbuf, 0, serverkeycount, serverkeys);
    qr_build_partial_query_reply(qrec, outbuf, 1, playerkeycount, playerkeys);
    qr_build_partial_query_reply(qrec, outbuf, 2, teamkeycount, teamkeys);
}
