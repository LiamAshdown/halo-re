// network_game_server_handle_join_confirm  (Ghidra: FUN_004e2400)
// address 0x4e2400, size 200 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e2400..0x4e24c7: EAX machine, ECX server, EDX buffer, stack length: while
//   the host is not in a game, the decoded body is the player to add; failure sends reason 3, success broadcasts the
//   player set and sends a type 0xa accept. Returns 1.
// blam-cc: EAX -> machine, ECX -> server, EDX -> buffer, stack -> length

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, blam-cc: EAX type, EDX payload
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit, void *data,
    uint32_t bits, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine_id, ESI server
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body,
    uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class); // 0x4d09d0, blam-cc: EAX remaining_length
extern uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server,
    network_machine *machine); // 0x4df840, blam-cc: EAX entry, ECX server, EDX machine
extern uint32_t network_game_broadcast_player_set_changed(uint8_t *param_1); // 0x4e1bf0, one stack argument (the session)
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0

char network_game_server_handle_join_confirm(network_machine *machine, network_server_globals *server, uint8_t *buffer,
    int32_t length)
{
    uint16_t out_version;
    int16_t out_type;
    uint8_t body[0x20];
    int16_t remaining;

    if (server->state != 0 && server->state != 1) {
        return 1;
    }
    remaining = (int16_t)(length - 2);
    if (data_packet_group_decode_packet(&remaining, &network_game_messages_group, body, buffer + 2, &out_type, &out_version, 3) == 0) {
        return 1;
    }
    if (network_game_session_finalize_and_add_player((network_player_entry *)body, server, machine) == 0) {
        network_server_notify_or_resend_challenge(3, machine, server);
        return 1;
    }
    if (network_game_broadcast_player_set_changed((uint8_t *)server) != 0) {
        uint32_t payload = 0;
        uint16_t *packet = network_prepare_challenge_packet(0xa, &payload);

        if (packet != 0 && machine->machine_id != -1) {
            network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3, 1, 0, 1, 3);
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2400), from tools/pack.py 0x4e2400:

undefined4 FUN_004e2400(void)

{
  char cVar1;
  int in_EAX;
  ushort *puVar2;
  int in_ECX;
  int in_EDX;
  undefined1 local_28 [4];
  undefined4 local_24;
  undefined1 local_20 [32];

  if ((*(short *)(in_ECX + 4) == 0) || (*(short *)(in_ECX + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,&local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      cVar1 = FUN_004df840();
      if (cVar1 == '\0') {
        FUN_004e0af0();
      }
      else {
        cVar1 = FUN_004e1bf0();
        if (cVar1 != '\0') {
          local_24 = 0;
          puVar2 = (ushort *)network_prepare_challenge_packet();
          if ((puVar2 != (ushort *)0x0) && (*(short *)(in_EAX + 0xc) != -1)) {
            network_session_send_to_machine(0,puVar2,(uint)(*puVar2 >> 4) << 3,1,0,1,3);
            return 1;
          }
        }
      }
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
