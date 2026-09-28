// compute_challenge_response  (GameSpy SDK in halo.exe; no C existed)
// address 0x615810, size 158 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615810..0x6158ad: EBX challenge length, EDI output buffer, stack record,
//   challenge: a NUL-terminated challenge of 1..0x41 bytes is copied, encrypted with the secret key (gs_encrypt) and
//   appended base64-encoded with its NUL.
// blam-cc: EBX -> len, EDI -> outbuf, stack -> qrec, challenge

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
