// gcd_send_is_online  (GameSpy SDK in halo.exe; no C existed)
// address 0x61acf0, size 369 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61acf0..0x61ae60: ECX buffer, stack address: "\\uon\\\\skey\\%d" when the
//   client with the buffer's "cd" hash is finished (state 3), else "\\uoff\\\\skey\\%d", with the buffer's skey,
//   xored and sent back.
// blam-cc: cdecl

#include "gamespy.h"

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
// blam-cc of gcd_send_is_online: ECX buf, stack to


void gcd_send_is_online(const char *buf, const struct sockaddr *to)
{
    char out[0x40];
    int skey = atol(gcd_value_for_key(buf, "skey"));
    gcd_client *client = gcd_find_client_by_hash(gcd_value_for_key(buf, "cd"), -1);
    int length;

    if (client != 0 && client->state == 3) {
        length = sprintf(out, "\\uon\\\\skey\\%d", skey);
    } else {
        length = sprintf(out, "\\uoff\\\\skey\\%d", skey);
    }
    gs_xcode_buf(out, length);
    sendto(gcd_socket, out, length, 0, to, 0x10);
}
