// network_session_host_reject_or_cleanup_client  (Ghidra: FUN_00575ff0)
// address 0x575ff0, size 162 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x575ff0..0x576091: the host's CD key check for a joining machine:
//   gcd_authenticate_user(game id 0x0069fdfc, local id, ip, challenge, response,
//   network_session_host_cd_key_callback, 0), then the ban list check on the key hash (gcd_getkeyhash, EDI). Not
//   banned: 1. Banned: reason 6 to the machine with that local id (or NULL), the key is disconnected from gcd (every
//   key for local id -1), 0. (Name kept; it authenticates.)
// blam-cc: EAX -> response, ECX -> challenge, EDX -> ip, ESI -> local_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t network_cd_key_game_id; // 0x0069fdfc
extern void gcd_authenticate_user(int32_t game_id, int32_t local_id, uint32_t ip, const char *challenge, const char *response,
    void *callback, void *instance); // 0x61b110 gcd_authenticate_user
extern const char *gcd_getkeyhash(int32_t game_id, int32_t local_id); // 0x61aa50 gcd_getkeyhash
extern void gcd_disconnect_user(int32_t game_id, int32_t local_id); // 0x61b350 gcd_disconnect_user
extern void gcd_disconnect_all(int32_t game_id); // 0x61b3f0 gcd_disconnect_all
extern uint8_t ban_list_check_and_reject_player(char *key); // 0x4e3820, blam-cc: EDI key
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0
extern void network_session_host_cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated,
    const char *message, void *instance); // 0x5760a0

uint8_t network_session_host_reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip, int32_t local_id)
{
    network_server_globals *server;
    network_machine *machine = 0;
    int32_t i;

    gcd_authenticate_user(network_cd_key_game_id, local_id, ip, challenge, response, (void *)network_session_host_cd_key_callback, 0);
    if (ban_list_check_and_reject_player((char *)gcd_getkeyhash(network_cd_key_game_id, local_id)) == 0) {
        return 1;
    }
    server = network_server;
    for (i = 0; i < 0x10; i++) {
        if (*(int32_t *)((uint8_t *)server + 0x414 + i * 0x60) == local_id) {
            machine = (network_machine *)((uint8_t *)server + 0x3b8 + i * 0x60);
            break;
        }
    }
    network_server_notify_or_resend_challenge(6, machine, server);
    if (local_id == -1) {
        gcd_disconnect_all(network_cd_key_game_id);
    } else {
        gcd_disconnect_user(network_cd_key_game_id, local_id);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x575ff0):

undefined4 FUN_00575ff0(void)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  int unaff_ESI;

  FUN_0061b110(DAT_0069fdfc);
  FUN_0061aa50(DAT_0069fdfc);
  cVar1 = ban_list_check_and_reject_player();
  if (cVar1 == '\0') {
    return 1;
  }
  iVar2 = 0;
  piVar3 = (int *)(DAT_0071c2d4 + 0x414);
  do {
    if (*piVar3 == unaff_ESI) break;
    iVar2 = iVar2 + 1;
    piVar3 = piVar3 + 0x18;
  } while (iVar2 < 0x10);
  FUN_004e0af0(DAT_0071c2d4);
  if (unaff_ESI != -1) {
    FUN_0061b350(DAT_0069fdfc);
    return 0;
  }
  FUN_0061b3f0(DAT_0069fdfc);
  return 0;
}
#endif
