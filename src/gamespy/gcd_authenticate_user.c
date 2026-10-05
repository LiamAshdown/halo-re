// gcd_authenticate_user  (GameSpy SDK in halo.exe; no C existed)
// address 0x61b110, size 567 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61b110..0x61b346: for a known game: a response shorter than 72 characters is
//   "Bad CD Key" and a hash already in use is "CD Key in use"; the new 0x50 byte client (hash = first 32 characters)
//   is appended to the game's list; with a message it is rejected (state 2), otherwise the auth request goes out.
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


void gcd_authenticate_user(int game_id, int local_id, unsigned int user_ip, const char *challenge, const char *response,
    void *callback, void *instance)
{
    char hash[0x21];
    char *message;
    gcd_game *game = 0;
    gcd_client_node *node;
    gcd_client_node *last;
    gcd_client *client;
    int i;

    for (i = 0; i < gcd_game_count; i++) {
        if (gcd_games[i].game_id == game_id) {
            game = &gcd_games[i];
            break;
        }
    }
    if (game == 0) {
        return;
    }
    strncpy(hash, response, 0x20);
    hash[0x20] = 0;
    message = strlen(response) < 0x48 ? goastrdup("Bad CD Key") : 0;
    for (node = game->sentinel.next; node != 0; node = node->next) {
        if (strcmp(hash, node->client->hash) == 0) {
            message = goastrdup("CD Key in use");
            break;
        }
    }
    client = (gcd_client *)malloc(sizeof(gcd_client));
    client->local_id = local_id;
    client->instance = instance;
    client->ip = user_ip;
    client->callback = (void (*)(int, int, int, const char *, void *))callback;
    client->message = 0;
    client->request = 0;
    strcpy(client->hash, hash);
    node = (gcd_client_node *)malloc(sizeof(gcd_client_node));
    node->client = client;
    for (last = &game->sentinel; last->next != 0; last = last->next) {
    }
    last->next = node;
    node->prev = last;
    node->next = 0;
#if defined(__EMSCRIPTEN__)
    fprintf(stderr, "web: gcd auth local_id=%d response_length=%d result=%s\n", local_id, (int)strlen(response),
        message != 0 ? message : "sent to keymaster");
#endif
    if (message != 0) {
        client->message = message;
        client->state = 2;
        return;
    }
    gcd_send_auth_request(game, client, challenge, response);
}
