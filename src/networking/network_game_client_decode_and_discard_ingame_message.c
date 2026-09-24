// network_game_client_decode_and_discard_ingame_message  (Ghidra: FUN_004dc020; renamed, no
// prior name)
// address 0x4dc020, size 99 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Decodes and discards an in-game
// message type (0xd) using a different message-group index than the join-handshake handlers").
// Structurally identical to network_game_client_decode_and_discard_join_message.c except for
// the mode check (4, in-game, instead of 2) and the message-class argument (6 instead of 2).
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_and_discard_ingame_message(network_client_globals *client,
    const uint8_t *buffer, void *capacity, int32_t *expected_sequence)
{
    network_resolved_address sender;
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state == 4) { // UNSURE: mode field
        // UNSURE: decode destination is `&expected_sequence`; see
        // network_game_client_decode_player_config_value.c for the same idiom.
        data_packet_group_decode_packet(&network_game_messages_group, &expected_sequence,
                                         buffer + 2, &out_a, &out_b, 6);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dc020):

undefined4 FUN_004dc020(int param_1,undefined4 param_2,int *param_3)

{
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 4)) {
    data_packet_group_decode_packet
              (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,&param_3,6)
    ;
  }
  return 1;
}
#endif
