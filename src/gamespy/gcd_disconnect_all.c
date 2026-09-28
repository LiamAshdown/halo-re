// gcd_disconnect_all  (GameSpy SDK in halo.exe; no C existed)
// address 0x61b3f0, size 141 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61b3f0..0x61b47c: removes every client of the game (the master is told for
//   each).
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
