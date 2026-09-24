// network_game_client_decode_join_finalize_ack  (Ghidra: FUN_004dc120; renamed, no prior name)
// address 0x4dc120, size 112 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Decodes a join-handshake message (type
// 0xb) and forwards the payload to a follow-up handler"). Forwards to
// network_client_timer_default_or_disconnect.c (network_client_timer_default_or_disconnect, 0x4d9ce0, this task's batch).
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `network_client_timer_default_or_disconnect` is called here with no visible argument;
// reconstructed as taking `client`, matching that function's own established signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8
extern void network_client_timer_default_or_disconnect(network_client_globals *client); // 0x4d9ce0, this batch

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_join_finalize_ack(network_client_globals *client, const uint8_t *buffer,
    void *capacity, int32_t *expected_sequence)
{
    network_resolved_address sender;
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state == 2) { // UNSURE: mode field
        // UNSURE: decode destination is `&expected_sequence`; see
        // network_game_client_decode_player_config_value.c for the same idiom.
        if (data_packet_group_decode_packet(&network_game_messages_group, &expected_sequence,
                                             buffer + 2, &out_a, &out_b, 2) != 0) {
            network_client_timer_default_or_disconnect(client); // UNSURE argument
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dc120):

undefined4 FUN_004dc120(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 2)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                       &param_3,2);
    if (cVar1 != '\0') {
      FUN_004d9ce0();
    }
  }
  return 1;
}
#endif
