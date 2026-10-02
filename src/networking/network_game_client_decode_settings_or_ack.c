// network_game_client_decode_settings_or_ack  (Ghidra: FUN_004dbe50; renamed, no prior name)
// address 0x4dbe50, size 218 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes an incoming game-settings/
// map-info message during the join handshake and applies it, sending a one-time acknowledgement
// when acting purely as a client"). Forwards to network_game_settings_packet_receive.c
// (0x4d9800) and network_game_settings_ack_send.c (0x4d9f50), both this task's batch.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `network_game_settings_ack_send` is called here with no visible arguments;
// reconstructed as (client, 0) matching network_game_settings_packet_receive.c's own
// reconstruction of the same call.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the third argument is the record length (an int);
// the original reduces it by 2 in its own slot and passes its address (EAX) as data_packet_group_decode_packet's
// remaining length -- that argument was missing from the declaration, so every other argument was shifted by one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int16_t network_game_mode; // 0x00719720
extern char network_game_settings_ack_send(uint8_t *client, int16_t template_row); // 0x4d9f50
extern int32_t network_game_settings_packet_receive(network_client_globals *client,
    const uint32_t *request); // 0x4d9800

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_settings_or_ack(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const int32_t *expected_sequence)
{
    network_resolved_address sender;
    uint8_t decoded_body[944];
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence) {
        if (network_game_mode == 2) {
            if (client->state == 2 && *(uint8_t *)&client->pad_ee2 == 0) { // UNSURE: mode field
                network_game_settings_ack_send((uint8_t *)client, 0); // UNSURE argument
                *(uint8_t *)&client->pad_ee2 = 1;
            }
        } else if (client->state == 2 || client->state == 3) {
            if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                                 decoded_body, buffer + 2, &out_a, (uint16_t *)&out_b, 2) != 0) {
                return network_game_settings_packet_receive(client, (const uint32_t *)decoded_body);
            }
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dbe50):

undefined4 FUN_004dbe50(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  undefined4 uVar2;
  int unaff_ESI;
  undefined1 local_3d0 [4];
  undefined1 local_3cc [4];
  int local_3c8;
  undefined1 local_3b0 [944];

  FUN_004dd390();
  if (local_3c8 == *param_3) {
    if (DAT_00719720 == 2) {
      if ((*(short *)(unaff_ESI + 0xeda) == 2) && (*(char *)(unaff_ESI + 0xee2) == '\0')) {
        FUN_004d9f50();
        *(undefined1 *)(unaff_ESI + 0xee2) = 1;
      }
    }
    else if ((*(short *)(unaff_ESI + 0xeda) == 2) || (*(short *)(unaff_ESI + 0xeda) == 3)) {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_3b0,param_1 + 2,local_3d0
                         ,local_3cc,2);
      if (cVar1 != '\0') {
        uVar2 = FUN_004d9800(local_3b0);
        return uVar2;
      }
      return 0;
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
