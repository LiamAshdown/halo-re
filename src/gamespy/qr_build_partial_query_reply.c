// qr_build_partial_query_reply  (GameSpy SDK in halo.exe; no C existed)
// address 0x6158c0, size 695 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6158c0..0x615b76: EBX record, stack buffer, key type, key count, keys: for
//   player/team types the (network order) count from the count callback first; 0xff means every key of the type (from
//   the key list callback), whose names (qr2_registered_key_list, else "unknown") are listed and NUL-terminated --
//   server keys with their values inline; then the values of every key for each of the count rows (server / player /
//   team key callbacks), an empty string for a key that wrote nothing.
// blam-cc: EBX -> qrec, stack -> outbuf, keytype, numkeys, keys

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
