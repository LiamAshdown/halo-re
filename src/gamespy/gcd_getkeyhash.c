// gcd_getkeyhash  (GameSpy SDK in halo.exe; no C existed)
// address 0x61aa50, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61aa50..0x61aaaa: the CD key hash (+4) of the client with this local id in the
//   game with this id (the 0x10 byte game table at 0x00723200, count 0x006a2e64, each with a client list at +8 of
//   {client, next} nodes), or "".
// blam-cc: cdecl

#include "gamespy.h"

typedef struct gcd_client_node {
    int *client;                  // 0x00 -> { local id, hash[] }
    struct gcd_client_node *next; // 0x04
} gcd_client_node;

typedef struct gcd_game {
    int game_id;                  // 0x00
    int unknown_04;               // 0x04
    gcd_client_node *clients;     // 0x08
    int unknown_0c;               // 0x0c
} gcd_game;

extern gcd_game gcd_games[];      // 0x00723200
extern int gcd_game_count;        // 0x006a2e64

const char *gcd_getkeyhash(int game_id, int local_id)
{
    int i;
    gcd_client_node *node;

    for (i = 0; i < gcd_game_count; i++) {
        if (gcd_games[i].game_id == game_id) {
            break;
        }
    }
    if (i >= gcd_game_count) {
        return "";
    }
    for (node = gcd_games[i].clients; node != 0; node = node->next) {
        if (node->client[0] == local_id) {
            return (const char *)(node->client + 1);
        }
    }
    return "";
}
