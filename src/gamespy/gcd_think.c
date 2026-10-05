// gcd_think  (GameSpy SDK in halo.exe; no C existed)
// address 0x61b7e0, size 842 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61b7e0..0x61bb29: with its own socket (never, see gcd_shutdown) it drains
//   replies through gcd_process. Then per client: pending ones resend every 2 s (up to three tries, then they count
//   as authorized with "Validation Timeout"); authorized ones call back (1, "Validated") and finish (3); rejected
//   ones are unlinked, call back (0, their message or "") and are freed.
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


extern void gcd_process(char *buf, int len, const struct sockaddr *from);
extern char gcd_receive_buffer[0x400]; // 0x006a2810

void gcd_think(void)
{
    int i;

    if (gcd_no_network == 0 && gcd_own_socket != 0xffff) {
        fd_set set;
        struct timeval timeout = { 0, 0 };
        struct sockaddr from;
        int from_length = 0x10;
        int result;

        FD_ZERO(&set);
        FD_SET(gcd_socket, &set);
        result = select(FD_SETSIZE, &set, 0, 0, &timeout);
        while (result != SOCKET_ERROR && result != 0) {
            result = recvfrom(gcd_socket, gcd_receive_buffer, 0x3ff, 0, &from, &from_length);
            if (result != SOCKET_ERROR) {
                gcd_receive_buffer[result] = 0;
                gcd_process(gcd_receive_buffer, result, &from);
            }
            result = select(FD_SETSIZE, &set, 0, 0, &timeout);
        }
    }
    for (i = 0; i < gcd_game_count; i++) {
        gcd_client_node *node;

        for (node = gcd_games[i].sentinel.next; node != 0; node = node->next) {
            gcd_client *client = node->client;

            if (client->state == 0) {
                if (GetTickCount() < client->sent_time + 2000) {
                    continue;
                }
                if (client->tries <= 2) {
                    client->sent_time = GetTickCount();
                    client->tries++;
                    if (gcd_no_network != 1) {
                        sendto(gcd_socket, client->request, client->request_length, 0,
                            (const struct sockaddr *)&gcd_master_address, 0x10);
                    }
                    continue;
                }
            }
            if (client->state == 0 || client->state == 1) {
                client->callback(gcd_games[i].game_id, client->local_id, 1,
                    client->state == 1 ? "Validated" : "Validation Timeout", client->instance);
                node->client->state = 3;
                free(node->client->request);
                node->client->request = 0;
            } else if (client->state == 2) {
                gcd_client_node *prev = node->prev;

                node->prev->next = node->next;
                if (node->next != 0) {
                    node->next->prev = node->prev;
                }
                client->callback(gcd_games[i].game_id, client->local_id, 0,
                    client->message != 0 ? client->message : "", client->instance);
                free(node->client->request);
                if (node->client->message != 0) {
                    free(node->client->message);
                }
                free(node->client);
                free(node);
                node = prev;
            }
        }
    }
}
