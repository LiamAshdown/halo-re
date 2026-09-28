// send_heartbeat  (GameSpy SDK in halo.exe; no C existed)
// address 0x616680, size 1226 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616680..0x616b49: a heartbeat (type 3, instance key) to the master:
//   "localip<n>" per cached local address, "localport" (the query port), "natneg" ("1"/"0"), "statechanged" (when
//   non-zero), "gamename"; a state change of 2 (exiting) ends there, otherwise every server, player and team key
//   follows (qr_build_partial_query_reply with 0xff, EBX the record). Remembers the heartbeat and keep-alive time.
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

extern unsigned int qr2_local_ips[5];    // 0x006a26d8
extern int qr2_local_ip_count;           // 0x006a27f0
void send_heartbeat(qr2_implementation_s *qrec, int statechanged);
void qr2_check_send_heartbeat(qr2_implementation_s *qrec);
void qr2_check_queries(qr2_implementation_s *qrec);
void send_keepalive(qr2_implementation_s *qrec);


extern void qr_build_partial_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype, int numkeys,
    unsigned char *keys);

void send_heartbeat(qr2_implementation_s *qrec, int statechanged)
{
    qr2_buffer_s buffer;
    char text[0x14];
    int i;

    buffer.buffer[0] = 3;
    memcpy(buffer.buffer + 1, qrec->instance_key, 4);
    buffer.len = 5;
    for (i = 0; i < qr2_local_ip_count; i++) {
        struct in_addr address;

        sprintf(text, "localip%d", i);
        qr2_buffer_add(&buffer, text);
        address.s_addr = qr2_local_ips[i];
        qr2_buffer_add(&buffer, inet_ntoa(address));
    }
    qr2_buffer_add(&buffer, "localport");
    sprintf(text, "%d", qrec->qport);
    qr2_buffer_add(&buffer, text);
    qr2_buffer_add(&buffer, "natneg");
    qr2_buffer_add(&buffer, qrec->nat_negotiate != 0 ? "1" : "0");
    if (statechanged != 0) {
        qr2_buffer_add(&buffer, "statechanged");
        sprintf(text, "%d", statechanged);
        qr2_buffer_add(&buffer, text);
    }
    qr2_buffer_add(&buffer, "gamename");
    qr2_buffer_add(&buffer, qrec->gamename);
    if (statechanged == 2) {
        if (0x800 - buffer.len >= 1) {
            buffer.buffer[buffer.len] = 0;
            buffer.len++;
        }
    } else {
        qr_build_partial_query_reply(qrec, &buffer, 0, 0xff, 0);
        qr_build_partial_query_reply(qrec, &buffer, 1, 0xff, 0);
        qr_build_partial_query_reply(qrec, &buffer, 2, 0xff, 0);
    }
    sendto(qrec->hbsock, buffer.buffer, buffer.len, 0, (const struct sockaddr *)&qrec->hbaddr, 0x10);
    qrec->lastheartbeat = current_time();
    qrec->lastka = qrec->lastheartbeat;
}
