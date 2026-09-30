// gcd_init_qr2  (GameSpy SDK in halo.exe; no C existed)
// address 0x61b6d0, size 138 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61b6d0..0x61b759: sharing a query/report record's socket (default record
//   0x00683838): without the flag there is no network (0x006a2e68 = 1); otherwise the record's socket, gcd_process as
//   its cd-key callback (+0xd4) and the master address (AF_INET, port 29910, the record's master ip +0xc8). Then
//   gcd_init_common.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

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


extern unsigned char static_rec[]; // 0x00683838


void gcd_init_qr2(void *qrec, int game_id, int use_network)
{
    gcd_no_network = use_network == 0;
    if (qrec == 0) {
        qrec = static_rec;
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
