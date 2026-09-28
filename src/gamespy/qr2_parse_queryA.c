// qr2_parse_queryA  (GameSpy SDK in halo.exe; no C existed)
// address 0x616050, size 611 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616050..0x6162b2: a query for the record (the current one for NULL): ";" goes
//   to the cd key handler; "\\" gets the old-style reply; 0xfe 0xfd packets of at least 7 bytes (type, instance key,
//   data) -- query (0) builds the reply, challenge (1) the challenge response, echo (2) echoes up to 32 bytes as type
//   5, add-error (4) with our instance key reports once to the add-error callback without a reply, client message (6)
//   with our instance key is acknowledged (type 7 with its 4 byte key) and handled once per key; the reply (type
//   byte, instance key, data) goes back to the sender.
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
    if (query[0] == '\\') {
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
