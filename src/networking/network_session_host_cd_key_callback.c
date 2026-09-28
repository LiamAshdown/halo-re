// network_session_host_cd_key_callback  (not a Ghidra function; no C existed)
// address 0x5760a0, size 92 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5760a0..0x5760fb: the gcd_authenticate_user callback (game id, local id,
//   authenticated, message, instance): a rejected key sends reason 4 to the machine with that CD key local id (+0x5c
//   of the 0x60 byte machines at server +0x3b8; NULL when none).
// blam-cc: cdecl (a gcdkey callback)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0, blam-cc: CX reason, EDI machine

void network_session_host_cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated, const char *message,
    void *instance)
{
    network_server_globals *server = network_server;
    network_machine *machine = 0;
    int32_t i;

    (void)game_id;
    (void)message;
    (void)instance;
    if (authenticated != 0) {
        return;
    }
    for (i = 0; i < 0x10; i++) {
        if (*(int32_t *)((uint8_t *)server + 0x414 + i * 0x60) == local_id) {
            machine = (network_machine *)((uint8_t *)server + 0x3b8 + i * 0x60);
            break;
        }
    }
    network_server_notify_or_resend_challenge(4, machine, server);
}
